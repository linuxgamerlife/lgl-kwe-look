#pragma once
#include <QList>
#include <QString>
#include <QStringList>

// Line-preserving INI patcher.
//
// A port of the session's Python patcher (start-kineticwe.sh step 4h). Only the
// lines that change are touched: comments, blank lines, ordering and unknown
// keys survive. That matters because qt5ct.conf / qt6ct.conf are also written
// by the login script and read live by every running Qt app.
//
// Values are written as given. For QSettings-format files (qt5ct.conf,
// qt6ct.conf) pass them through encodeValue() / encodeList() first so the
// plugin's QSettings reader decodes them to the same string.
namespace IniFile {

struct Update {
    QString section;
    QString key;
    QString value;      // already encoded
    bool remove = false;

    static Update set(const QString &section, const QString &key, const QString &value)
    {
        return {section, key, value, false};
    }
    static Update erase(const QString &section, const QString &key)
    {
        return {section, key, QString(), true};
    }
};

// ---- QSettings-compatible value encoding ----
QString encodeValue(const QString &value);
QString encodeList(const QStringList &list);   // empty list -> "@Invalid()" like QSettings
QString decodeValue(const QString &raw);

// ---- Pure text operations (unit tested) ----
QString patchText(const QString &text, const QList<Update> &updates, bool *changed = nullptr);
QString readValue(const QString &text, const QString &section, const QString &key,
                  const QString &fallback = QString());

// ---- File operations ----
bool readText(const QString &path, QString *out);
// Atomic (QSaveFile). Follows symlinks so a symlinked file (the KWE root's
// kdeglobals) is updated in place instead of being replaced by a regular file.
bool writeTextAtomic(const QString &path, const QString &text, QString *error);
// Read-modify-write. Creates the file when missing. Skips the write when
// nothing differs so unchanged files do not wake the plugin's file watcher.
bool applyUpdates(const QString &path, const QList<Update> &updates, QString *error,
                  bool *changed = nullptr);
QString readFileValue(const QString &path, const QString &section, const QString &key,
                      const QString &fallback = QString());

}  // namespace IniFile
