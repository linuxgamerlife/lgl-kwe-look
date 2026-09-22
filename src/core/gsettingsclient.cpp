#include "gsettingsclient.h"

#include <QProcess>
#include <QStandardPaths>
#include <cmath>

namespace GSettings {

namespace {

constexpr int kTimeoutMs = 5000;

QString program()
{
    return QStandardPaths::findExecutable(QStringLiteral("gsettings"));
}

// Returns true when the process ran and exited 0.
bool run(const QStringList &args, QString *out, QString *err)
{
    const QString exe = program();
    if (exe.isEmpty()) {
        if (err)
            *err = QStringLiteral("gsettings is not installed");
        return false;
    }

    QProcess p;
    p.start(exe, args);
    if (!p.waitForFinished(kTimeoutMs)) {
        p.kill();
        p.waitForFinished(1000);
        if (err)
            *err = QStringLiteral("gsettings timed out");
        return false;
    }
    if (out)
        *out = QString::fromUtf8(p.readAllStandardOutput()).trimmed();
    if (p.exitStatus() != QProcess::NormalExit || p.exitCode() != 0) {
        if (err)
            *err = QString::fromUtf8(p.readAllStandardError()).trimmed();
        return false;
    }
    return true;
}

}  // namespace

bool available()
{
    return !program().isEmpty();
}

QString normalize(const QString &raw)
{
    QString v = raw.trimmed();
    if (v.size() >= 2 && v.startsWith(QLatin1Char('\'')) && v.endsWith(QLatin1Char('\'')))
        return v.mid(1, v.size() - 2);
    return v;
}

bool valuesEquivalent(const QString &wanted, const QString &got)
{
    const QString a = normalize(wanted);
    const QString b = normalize(got);
    if (a == b)
        return true;

    // 1.000000 (what we send) reads back as 1.0
    bool okA = false;
    bool okB = false;
    const double da = a.toDouble(&okA);
    const double db = b.toDouble(&okB);
    return okA && okB && std::fabs(da - db) < 1e-6;
}

QString get(const QString &schema, const QString &key, bool *ok)
{
    QString out;
    QString err;
    const bool good = run({QStringLiteral("get"), schema, key}, &out, &err);
    if (ok)
        *ok = good;
    return good ? normalize(out) : QString();
}

StepResult setVerified(const QString &schema, const QString &key, const QString &value)
{
    QString err;
    if (!run({QStringLiteral("set"), schema, key, value}, nullptr, &err))
        return StepResult::fail(QStringLiteral("gsettings set %1 failed: %2").arg(key, err));

    bool ok = false;
    const QString back = get(schema, key, &ok);
    if (!ok)
        return StepResult::warn(QStringLiteral("%1 written but could not be read back").arg(key));
    if (!valuesEquivalent(value, back)) {
        return StepResult::warn(
            QStringLiteral("%1 did not persist (wanted '%2', read '%3'). No session bus or dconf "
                           "service? gsettings then falls back to an in-memory backend.")
                .arg(key, value, back));
    }
    return StepResult::ok(QStringLiteral("%1 = %2").arg(key, value));
}

}  // namespace GSettings
