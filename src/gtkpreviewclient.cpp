#include "gtkpreviewclient.h"

#include <QCoreApplication>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QProcessEnvironment>
#include <QTimer>
#include <cmath>

GtkPreviewClient::GtkPreviewClient(QObject *parent) : QObject(parent) {}

GtkPreviewClient::~GtkPreviewClient()
{
    if (m_proc && m_proc->state() != QProcess::NotRunning) {
        m_proc->write("Q\n");
        m_proc->closeWriteChannel();
        if (!m_proc->waitForFinished(1000))
            m_proc->kill();
    }
}

QString GtkPreviewClient::helperPath()
{
    const QString beside = QCoreApplication::applicationDirPath() + QStringLiteral("/lgl-kwe-look-gtk-preview");
    if (QFileInfo::exists(beside))
        return beside;
    return QStringLiteral(LGLKWE_LIBEXECDIR "/lgl-kwe-look-gtk-preview");
}

bool GtkPreviewClient::isAvailable() const
{
    return QFileInfo::exists(helperPath());
}

void GtkPreviewClient::ensureStarted()
{
    if (m_proc)
        return;

    m_proc = new QProcess(this);
    m_proc->setProcessChannelMode(QProcess::SeparateChannels);

    // GTK renders at its own integer scale; hand it ours so the preview is sharp on HiDPI.
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    const qreal dpr = qApp->devicePixelRatio();
    m_scale = qMax(1, int(std::ceil(dpr)));
    if (m_scale > 1)
        env.insert(QStringLiteral("GDK_SCALE"), QString::number(m_scale));
    m_proc->setProcessEnvironment(env);

    connect(m_proc, &QProcess::readyReadStandardOutput, this, &GtkPreviewClient::onReadyRead);
    connect(m_proc, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
        m_ready = false;
        m_busy = false;
        if (error == QProcess::FailedToStart && m_proc) {
            // No finished() follows a failed start: drop the process so the next render retries.
            m_proc->deleteLater();
            m_proc = nullptr;
        }
        emit failed(tr("The GTK preview helper could not be started (%1).").arg(helperPath()));
    });
    connect(m_proc, &QProcess::finished, this, [this]() {
        const bool wasBusy = m_busy;
        m_ready = false;
        m_busy = false;
        m_proc->deleteLater();
        m_proc = nullptr;
        m_buffer.clear();
        if (wasBusy)
            emit failed(tr("The GTK preview helper exited unexpectedly."));
    });
    m_proc->start(helperPath(), {});
}

void GtkPreviewClient::render(const QString &theme, bool dark, const QString &font, const QSize &size)
{
    if (!isAvailable()) {
        emit failed(tr("The GTK preview helper is not installed."));
        return;
    }
    const Request r{theme, dark, font, size};
    ensureStarted();
    if (!m_ready || m_busy) {
        m_pending = r;
        m_hasPending = true;
        return;
    }
    sendRequest(r);
}

void GtkPreviewClient::restart()
{
    if (!m_proc)
        return;

    QProcess *old = m_proc;
    m_proc = nullptr;
    m_ready = false;
    m_busy = false;
    m_buffer.clear();

    // Detach it: nothing the old helper reports reaches this client any more.
    old->disconnect(this);
    old->write("Q\n");
    old->closeWriteChannel();
    connect(old, &QProcess::finished, old, &QObject::deleteLater);
    QTimer::singleShot(1500, old, &QProcess::kill);   // if it does not exit on its own
}

void GtkPreviewClient::sendRequest(const Request &r)
{
    m_busy = true;
    m_inFlight = m_nextId++;
    const QString out = m_tmp.path() + QStringLiteral("/preview-%1.png").arg(m_inFlight);
    const auto hex = [](const QString &s) { return s.toUtf8().toHex(); };

    QByteArray line = "R " + QByteArray::number(m_inFlight) + ' ' + (r.dark ? "1" : "0") + ' '
                    + QByteArray::number(r.size.width()) + ' ' + QByteArray::number(r.size.height()) + ' '
                    + hex(r.theme) + ' ' + hex(r.font) + ' ' + hex(out) + '\n';
    m_proc->write(line);
}

void GtkPreviewClient::onReadyRead()
{
    m_buffer += m_proc->readAllStandardOutput();
    int nl = -1;
    while ((nl = int(m_buffer.indexOf('\n'))) >= 0) {
        const QList<QByteArray> parts = m_buffer.left(nl).trimmed().split(' ');
        m_buffer.remove(0, nl + 1);
        if (parts.isEmpty())
            continue;

        const QByteArray &tag = parts.at(0);
        if (tag == "READY") {
            m_ready = true;
            if (m_hasPending) {
                m_hasPending = false;
                sendRequest(m_pending);
            }
        } else if (tag == "FATAL") {
            m_busy = false;
            emit failed(parts.size() > 1 ? QString::fromUtf8(QByteArray::fromHex(parts.at(1)))
                                         : tr("The GTK preview helper failed to start."));
        } else if (tag == "OK" || tag == "ERR") {
            m_busy = false;
            const QString file = m_tmp.path() + QStringLiteral("/preview-%1.png")
                                     .arg(parts.value(1).toULongLong());
            if (tag == "OK") {
                QImage img(file);
                if (img.isNull()) {
                    emit failed(tr("The GTK preview image could not be read."));
                } else {
                    img.setDevicePixelRatio(m_scale);
                    emit rendered(img);
                }
            } else {
                emit failed(parts.size() > 2 ? QString::fromUtf8(QByteArray::fromHex(parts.at(2)))
                                             : tr("The GTK preview failed."));
            }
            QFile::remove(file);
            if (m_hasPending && m_ready) {
                m_hasPending = false;
                sendRequest(m_pending);
            }
        }
    }
}
