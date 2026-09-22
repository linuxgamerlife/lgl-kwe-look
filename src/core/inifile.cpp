#include "inifile.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QSaveFile>

namespace IniFile {

namespace {

QStringList splitLines(const QString &text)
{
    QStringList lines = text.split(QLatin1Char('\n'));
    if (!lines.isEmpty() && lines.last().isEmpty())
        lines.removeLast();
    for (QString &l : lines) {
        if (l.endsWith(QLatin1Char('\r')))
            l.chop(1);
    }
    return lines;
}

bool isHeader(const QString &line)
{
    const QString t = line.trimmed();
    return t.size() >= 2 && t.startsWith(QLatin1Char('[')) && t.endsWith(QLatin1Char(']'));
}

QString headerName(const QString &line)
{
    const QString t = line.trimmed();
    return t.mid(1, t.size() - 2);
}

bool isKeyLine(const QString &line)
{
    const QString t = line.trimmed();
    return !t.isEmpty() && !t.startsWith(QLatin1Char('#')) && !t.startsWith(QLatin1Char(';'))
        && t.contains(QLatin1Char('='));
}

QString keyOf(const QString &line)
{
    return line.section(QLatin1Char('='), 0, 0).trimmed();
}

bool applyOne(QStringList &lines, const Update &u)
{
    int start = -1;
    for (int i = 0; i < lines.size(); ++i) {
        if (isHeader(lines.at(i)) && headerName(lines.at(i)) == u.section) {
            start = i;
            break;
        }
    }

    if (start < 0) {
        if (u.remove)
            return false;
        if (!lines.isEmpty() && !lines.last().trimmed().isEmpty())
            lines << QString();
        lines << QLatin1Char('[') + u.section + QLatin1Char(']')
              << u.key + QLatin1Char('=') + u.value;
        return true;
    }

    int end = lines.size();
    for (int i = start + 1; i < lines.size(); ++i) {
        if (isHeader(lines.at(i))) {
            end = i;
            break;
        }
    }

    for (int i = start + 1; i < end; ++i) {
        if (!isKeyLine(lines.at(i)) || keyOf(lines.at(i)) != u.key)
            continue;
        if (u.remove) {
            lines.removeAt(i);
            return true;
        }
        if (lines.at(i).section(QLatin1Char('='), 1).trimmed() == u.value)
            return false;
        lines[i] = u.key + QLatin1Char('=') + u.value;
        return true;
    }

    if (u.remove)
        return false;

    // Insert after the section's last non-blank line, keeping trailing blanks in place.
    int pos = end;
    while (pos - 1 > start && lines.at(pos - 1).trimmed().isEmpty())
        --pos;
    lines.insert(pos, u.key + QLatin1Char('=') + u.value);
    return true;
}

}  // namespace

QString encodeValue(const QString &value)
{
    if (value.isEmpty())
        return QString();

    bool needsQuotes = value.front().isSpace() || value.back().isSpace();
    QString out;
    out.reserve(value.size() + 2);
    for (const QChar c : value) {
        switch (c.unicode()) {
        case '\\': out += QLatin1String("\\\\"); break;
        case '"':  out += QLatin1String("\\\""); needsQuotes = true; break;
        case '\n': out += QLatin1String("\\n"); break;
        case '\r': out += QLatin1String("\\r"); break;
        case '\t': out += QLatin1String("\\t"); break;
        case ';':
        case ',':
        case '=':
            needsQuotes = true;
            out += c;
            break;
        default: out += c; break;
        }
    }
    return needsQuotes ? QLatin1Char('"') + out + QLatin1Char('"') : out;
}

QString encodeList(const QStringList &list)
{
    if (list.isEmpty())
        return QStringLiteral("@Invalid()");
    QStringList parts;
    parts.reserve(list.size());
    for (const QString &item : list) {
        const QString enc = encodeValue(item);
        parts << (enc.isEmpty() ? QStringLiteral("\"\"") : enc);
    }
    return parts.join(QStringLiteral(", "));
}

QString decodeValue(const QString &raw)
{
    QString v = raw.trimmed();
    if (v.size() < 2 || !v.startsWith(QLatin1Char('"')) || !v.endsWith(QLatin1Char('"')))
        return v;

    v = v.mid(1, v.size() - 2);
    QString out;
    out.reserve(v.size());
    for (int i = 0; i < v.size(); ++i) {
        const QChar c = v.at(i);
        if (c != QLatin1Char('\\') || i + 1 >= v.size()) {
            out += c;
            continue;
        }
        const QChar n = v.at(++i);
        switch (n.unicode()) {
        case 'n': out += QLatin1Char('\n'); break;
        case 'r': out += QLatin1Char('\r'); break;
        case 't': out += QLatin1Char('\t'); break;
        default:  out += n; break;   // \\ \" \; and anything else: the char itself
        }
    }
    return out;
}

QString patchText(const QString &text, const QList<Update> &updates, bool *changed)
{
    QStringList lines = splitLines(text);
    bool any = false;
    for (const Update &u : updates)
        any = applyOne(lines, u) || any;
    if (changed)
        *changed = any;
    return lines.isEmpty() ? QString() : lines.join(QLatin1Char('\n')) + QLatin1Char('\n');
}

QString readValue(const QString &text, const QString &section, const QString &key,
                  const QString &fallback)
{
    bool inSection = false;
    for (const QString &line : splitLines(text)) {
        if (isHeader(line)) {
            inSection = headerName(line) == section;
            continue;
        }
        if (inSection && isKeyLine(line) && keyOf(line) == key)
            return decodeValue(line.section(QLatin1Char('='), 1));
    }
    return fallback;
}

bool readText(const QString &path, QString *out)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
        return false;
    *out = QString::fromUtf8(f.readAll());
    return true;
}

bool writeTextAtomic(const QString &path, const QString &text, QString *error)
{
    QString target = path;
    QFile::Permissions perms;
    bool havePerms = false;

    const QFileInfo fi(path);
    if (fi.exists()) {
        target = fi.canonicalFilePath();
        perms = fi.permissions();
        havePerms = true;
    } else if (fi.isSymLink()) {
        target = fi.symLinkTarget();   // dangling link: create what it points at
    }

    if (!QDir().mkpath(QFileInfo(target).absolutePath())) {
        if (error)
            *error = QStringLiteral("cannot create directory for %1").arg(target);
        return false;
    }

    QSaveFile file(target);
    if (!file.open(QIODevice::WriteOnly)) {
        if (error)
            *error = QStringLiteral("cannot write %1: %2").arg(target, file.errorString());
        return false;
    }
    file.write(text.toUtf8());
    if (havePerms)
        file.setPermissions(perms);
    if (!file.commit()) {
        if (error)
            *error = QStringLiteral("cannot commit %1: %2").arg(target, file.errorString());
        return false;
    }
    return true;
}

bool applyUpdates(const QString &path, const QList<Update> &updates, QString *error, bool *changed)
{
    for (const Update &u : updates) {
        if (u.value.contains(QLatin1Char('\n')) || u.value.contains(QLatin1Char('\r'))
            || u.key.contains(QLatin1Char('\n')) || u.section.contains(QLatin1Char('\n'))) {
            if (error)
                *error = QStringLiteral("refusing to write a multi-line value for %1").arg(u.key);
            return false;
        }
    }

    QString text;
    readText(path, &text);   // a missing file is fine: it is created below
    bool didChange = false;
    const QString patched = patchText(text, updates, &didChange);

    if (changed)
        *changed = didChange;
    if (!didChange)
        return true;
    return writeTextAtomic(path, patched, error);
}

QString readFileValue(const QString &path, const QString &section, const QString &key,
                      const QString &fallback)
{
    QString text;
    if (!readText(path, &text))
        return fallback;
    return readValue(text, section, key, fallback);
}

}  // namespace IniFile
