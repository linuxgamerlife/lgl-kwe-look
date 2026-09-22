#pragma once
#include <QString>
#include <QStringList>

#include "stepresult.h"

// Thin wrapper around the gsettings CLI. Always an argument list, never a shell.
//
// gsettings can fail silently: with no session bus or dconf service the backend
// falls back to an in-memory store and `gsettings set` exits 0 while nothing is
// persisted. Every write is therefore read back in a fresh process.
namespace GSettings {

constexpr const char *kInterface = "org.gnome.desktop.interface";
constexpr const char *kSound     = "org.gnome.desktop.sound";

bool available();
// `gsettings get`, with the surrounding GVariant quotes removed. *ok is false
// when the key cannot be read.
QString get(const QString &schema, const QString &key, bool *ok = nullptr);
// `gsettings set` followed by a read-back comparison.
StepResult setVerified(const QString &schema, const QString &key, const QString &value);

// ---- pure helpers (unit tested) ----
QString normalize(const QString &raw);
bool valuesEquivalent(const QString &wanted, const QString &got);

}  // namespace GSettings
