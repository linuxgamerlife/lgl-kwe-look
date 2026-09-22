#pragma once
#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtGlobal>

// Points HOME and the XDG variables at a fresh temporary directory for one test object,
// so nothing ever touches the developer's real configuration.
class TestEnv
{
public:
    TestEnv()
    {
        const QString root = m_dir.path();
        qputenv("HOME", root.toUtf8());
        qputenv("XDG_CONFIG_HOME", (root + QStringLiteral("/.config")).toUtf8());
        qputenv("XDG_DATA_HOME", (root + QStringLiteral("/.local/share")).toUtf8());
        qputenv("XDG_DATA_DIRS", (root + QStringLiteral("/usr/share")).toUtf8());
        qunsetenv("KWE_CONFIG_HOME");
        qunsetenv("GTK2_RC_FILES");
        qunsetenv("XDG_CURRENT_DESKTOP");
        QDir().mkpath(root + QStringLiteral("/.config"));
        QDir().mkpath(root + QStringLiteral("/.local/share"));
        QDir().mkpath(root + QStringLiteral("/usr/share"));
    }

    QString root() const { return m_dir.path(); }
    QString path(const QString &rel) const { return m_dir.path() + QLatin1Char('/') + rel; }

    static void write(const QString &path, const QString &text)
    {
        QDir().mkpath(QFileInfo(path).absolutePath());
        QFile f(path);
        if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
            qFatal("TestEnv::write: cannot open %s", qPrintable(path));
        f.write(text.toUtf8());
    }

    static QString read(const QString &path)
    {
        QFile f(path);
        if (!f.open(QIODevice::ReadOnly))
            return QString();
        return QString::fromUtf8(f.readAll());
    }

private:
    QTemporaryDir m_dir;
};
