#pragma once
#include <QImage>
#include <QObject>
#include <QProcess>
#include <QSize>
#include <QTemporaryDir>

// Talks to the long-lived lgl-kwe-look-gtk-preview helper. Requests are coalesced:
// while one render is in flight only the newest pending request is kept.
class GtkPreviewClient : public QObject
{
    Q_OBJECT
public:
    explicit GtkPreviewClient(QObject *parent = nullptr);
    ~GtkPreviewClient() override;

    static QString helperPath();
    bool isAvailable() const;

    void render(const QString &theme, bool dark, const QString &font, const QSize &size);
    // GTK caches themes and the user's gtk.css for the life of a process, so a change on disk
    // only shows in a fresh helper. The next render() starts one.
    void restart();

signals:
    void rendered(const QImage &image);
    void failed(const QString &message);

private:
    struct Request {
        QString theme;
        bool dark = false;
        QString font;
        QSize size;
    };

    void ensureStarted();
    void sendRequest(const Request &r);
    void onReadyRead();

    QProcess *m_proc = nullptr;
    QTemporaryDir m_tmp;
    QByteArray m_buffer;
    bool m_ready = false;
    bool m_busy = false;
    bool m_hasPending = false;
    Request m_pending;
    quint64 m_nextId = 1;
    quint64 m_inFlight = 0;
    int m_scale = 1;
};
