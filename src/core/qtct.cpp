#include "qtct.h"

#include "kwepaths.h"

#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonObject>
#include <QLibraryInfo>
#include <QPluginLoader>
#include <QRegularExpression>
#include <QSet>
#include <QSettings>
#include <QStyleFactory>

namespace QtCt {

namespace {

constexpr int kQt5Roles = 21;
constexpr int kQt6Roles = 22;
constexpr int kMinAcceptedRoles = 20;   // files written before PlaceholderText existed

QString numberSuffixFree(QString s)
{
    while (!s.isEmpty() && s.back().isDigit())
        s.chop(1);
    return s;
}

QStringList keysFromPluginDir(const QString &dir, bool isStyle)
{
    QStringList keys;
    const QDir d(dir);
    for (const QFileInfo &fi : d.entryInfoList(QStringList{QStringLiteral("*.so")}, QDir::Files)) {
        // Prefer the plugin's own metadata; a Qt5 plugin read from a Qt6 process may not
        // expose it, so fall back to the file name.
        QStringList found;
        QPluginLoader loader(fi.absoluteFilePath());
        const QJsonArray arr = loader.metaData().value(QStringLiteral("MetaData")).toObject()
                                   .value(QStringLiteral("Keys")).toArray();
        for (const QJsonValue &v : arr)
            found << v.toString();

        if (found.isEmpty()) {
            const QString k = isStyle ? styleKeyFromFileName(fi.fileName())
                                      : platformThemeKeyFromFileName(fi.fileName());
            if (!k.isEmpty())
                found << k;
        }
        for (const QString &k : std::as_const(found)) {
            // the proxy style / platform theme are the runtime plugins themselves
            if (k.contains(QLatin1String("qt5ct"), Qt::CaseInsensitive)
                || k.contains(QLatin1String("qt6ct"), Qt::CaseInsensitive))
                continue;
            if (!keys.contains(k, Qt::CaseInsensitive))
                keys << k;
        }
    }
    return keys;
}

QString capitalised(QString s)
{
    if (!s.isEmpty())
        s[0] = s.at(0).toUpper();
    return s;
}

}  // namespace

QString toolName(QtTarget t)  { return t == QtTarget::Qt5 ? QStringLiteral("qt5ct") : QStringLiteral("qt6ct"); }
QString majorName(QtTarget t) { return t == QtTarget::Qt5 ? QStringLiteral("Qt5") : QStringLiteral("Qt6"); }

QString configDir(QtTarget t)  { return KwePaths::realConfigHome() + QLatin1Char('/') + toolName(t); }
QString configFile(QtTarget t) { return configDir(t) + QLatin1Char('/') + toolName(t) + QStringLiteral(".conf"); }
QString userColorSchemesDir(QtTarget t) { return configDir(t) + QStringLiteral("/colors"); }
QString userStyleSheetsDir(QtTarget t)  { return configDir(t) + QStringLiteral("/qss"); }
QString styleColorsFile(QtTarget t)     { return configDir(t) + QStringLiteral("/style-colors.conf"); }

namespace {
QStringList sharedDirs(QtTarget t, const QString &sub)
{
    QStringList out;
    const QString tool = toolName(t);
    for (const QString &d : KwePaths::dataDirs())
        out << d + QLatin1Char('/') + tool + QLatin1Char('/') + sub;
    out << QStringLiteral("/usr/share/%1/%2").arg(tool, sub);
    out << QStringLiteral(LGLKWE_DATADIR "/") + sub;   // schemes and sheets shipped by this app
    out.removeDuplicates();
    return out;
}
}  // namespace

QStringList sharedColorSchemeDirs(QtTarget t) { return sharedDirs(t, QStringLiteral("colors")); }
QStringList sharedStyleSheetDirs(QtTarget t)  { return sharedDirs(t, QStringLiteral("qss")); }

int paletteRoleCount(QtTarget t) { return t == QtTarget::Qt5 ? kQt5Roles : kQt6Roles; }

QString pluginRoot(QtTarget t)
{
    if (t == QtTarget::Qt6)
        return QLibraryInfo::path(QLibraryInfo::PluginsPath);

    for (const char *c : {"/usr/lib64/qt5/plugins", "/usr/lib/qt5/plugins",
                          "/usr/lib/x86_64-linux-gnu/qt5/plugins", "/usr/lib/aarch64-linux-gnu/qt5/plugins"}) {
        if (QFileInfo(QLatin1String(c)).isDir())
            return QLatin1String(c);
    }
    return QString();
}

QString styleKeyFromFileName(const QString &fileName)
{
    QString s = fileName;
    if (s.endsWith(QLatin1String(".so")))
        s.chop(3);
    if (s.startsWith(QLatin1String("lib")))
        s = s.mid(3);
    return numberSuffixFree(s).toLower();
}

QString platformThemeKeyFromFileName(const QString &fileName)
{
    QString s = fileName;
    if (s.endsWith(QLatin1String(".so")))
        s.chop(3);
    if (s.startsWith(QLatin1String("KDEPlasma")))
        return QStringLiteral("kde");
    if (s.startsWith(QLatin1String("lib")))
        s = s.mid(3);
    // libqgtk3 -> gtk3, libqxdgdesktopportal -> xdgdesktopportal (but not qt5ct itself)
    if (s.startsWith(QLatin1Char('q')) && !s.startsWith(QLatin1String("qt")))
        s = s.mid(1);
    return s.toLower();
}

QStringList availableStyles(QtTarget t)
{
    QStringList styles;
    if (t == QtTarget::Qt6) {
        styles = QStyleFactory::keys();
        styles.removeAll(QStringLiteral("qt6ct-style"));
        return styles;
    }
    styles << QStringLiteral("Fusion") << QStringLiteral("Windows");
    const QString root = pluginRoot(t);
    if (!root.isEmpty()) {
        for (const QString &k : keysFromPluginDir(root + QStringLiteral("/styles"), true)) {
            const QString cap = capitalised(k);
            if (!styles.contains(cap, Qt::CaseInsensitive))
                styles << cap;
        }
    }
    return styles;
}

QStringList availablePlatformThemes(QtTarget t)
{
    const QString root = pluginRoot(t);
    if (root.isEmpty())
        return {};
    QStringList keys = keysFromPluginDir(root + QStringLiteral("/platformthemes"), false);
    for (QString &k : keys)
        k = k.toLower();
    keys.removeDuplicates();
    return keys;
}

// ---------------------------------------------------------------- schemes ----

namespace {

// [Colors:<set>] <key>=r,g,b. Invalid when absent.
QColor kdeColor(QSettings &s, const QString &set, const QString &key)
{
    const QStringList v = s.value(QStringLiteral("Colors:%1/%2").arg(set, key)).toStringList();
    if (v.size() < 3)
        return QColor();
    return QColor(v.at(0).trimmed().toInt(), v.at(1).trimmed().toInt(), v.at(2).trimmed().toInt());
}

QColor mixColors(const QColor &a, const QColor &b, qreal t)
{
    return QColor::fromRgbF(a.redF() * (1 - t) + b.redF() * t, a.greenF() * (1 - t) + b.greenF() * t,
                            a.blueF() * (1 - t) + b.blueF() * t);
}

bool loadKdeScheme(const QString &path, Scheme *out)
{
    QSettings s(path, QSettings::IniFormat);
    const QColor window = kdeColor(s, QStringLiteral("Window"), QStringLiteral("BackgroundNormal"));
    const QColor base = kdeColor(s, QStringLiteral("View"), QStringLiteral("BackgroundNormal"));
    if (!window.isValid() || !base.isValid())
        return false;   // not a KColorScheme

    const QColor defaultFg = window.lightness() < 128 ? QColor(Qt::white) : QColor(Qt::black);
    const auto pick = [&s](const char *set, const char *key, const QColor &fallback) {
        const QColor c = kdeColor(s, QLatin1String(set), QLatin1String(key));
        return c.isValid() ? c : fallback;
    };

    const QColor text = pick("View", "ForegroundNormal", defaultFg);
    const QColor button = pick("Button", "BackgroundNormal", window);
    const QColor highlight = pick("Selection", "BackgroundNormal", base);
    const QColor toolTipBase = pick("Tooltip", "BackgroundNormal", base);

    QList<QColor> a(kQt6Roles);
    a[QPalette::Window] = window;
    a[QPalette::WindowText] = pick("Window", "ForegroundNormal", text);
    a[QPalette::Base] = base;
    a[QPalette::AlternateBase] = pick("View", "BackgroundAlternate", base);
    a[QPalette::Text] = text;
    a[QPalette::Button] = button;
    a[QPalette::ButtonText] = pick("Button", "ForegroundNormal", text);
    a[QPalette::BrightText] = pick("Window", "ForegroundNegative", QColor(Qt::white));
    a[QPalette::Highlight] = highlight;
    a[QPalette::HighlightedText] = pick("Selection", "ForegroundNormal", defaultFg);
    a[QPalette::Link] = pick("View", "ForegroundLink", text);
    a[QPalette::LinkVisited] = pick("View", "ForegroundVisited", text);
    a[QPalette::ToolTipBase] = toolTipBase;
    a[QPalette::ToolTipText] = pick("Tooltip", "ForegroundNormal", text);
    a[QPalette::Accent] = pick("View", "DecorationFocus", highlight);
    a[QPalette::NoRole] = window;
    a[QPalette::PlaceholderText] = text;
    a[QPalette::PlaceholderText].setAlpha(128);
    // The bevel colours are not part of a KColorScheme: derive them from the window colour.
    a[QPalette::Light] = window.lighter(150);
    a[QPalette::Midlight] = window.lighter(125);
    a[QPalette::Mid] = window.darker(150);
    a[QPalette::Dark] = window.darker(200);
    a[QPalette::Shadow] = QColor(Qt::black);

    // Disabled: text fades towards its own background, like the "Fade" contrast effect.
    const qreal fade = s.value(QStringLiteral("ColorEffects:Disabled/ContrastAmount"), 0.65).toReal();
    QList<QColor> d = a;
    const auto fadeText = [&](QPalette::ColorRole role, const QColor &bg) { d[role] = mixColors(a[role], bg, fade); };
    fadeText(QPalette::WindowText, window);
    fadeText(QPalette::BrightText, window);
    fadeText(QPalette::Text, base);
    fadeText(QPalette::Link, base);
    fadeText(QPalette::LinkVisited, base);
    fadeText(QPalette::ButtonText, button);
    fadeText(QPalette::HighlightedText, highlight);
    fadeText(QPalette::ToolTipText, toolTipBase);

    Scheme sch;
    sch.active = a;
    sch.inactive = a;
    sch.disabled = d;
    *out = sch;
    return true;
}

}  // namespace

bool loadScheme(const QString &path, Scheme *out)
{
    if (isKdeSchemePath(path))
        return loadKdeScheme(path, out);

    QSettings s(path, QSettings::IniFormat);
    s.beginGroup(QStringLiteral("ColorScheme"));
    const QStringList a = s.value(QStringLiteral("active_colors")).toStringList();
    const QStringList i = s.value(QStringLiteral("inactive_colors")).toStringList();
    const QStringList d = s.value(QStringLiteral("disabled_colors")).toStringList();
    s.endGroup();

    const int n = int(qMin(a.size(), qMin(i.size(), d.size())));
    if (n < kMinAcceptedRoles)
        return false;

    Scheme sch;
    for (int k = 0; k < n; ++k) {
        sch.active << QColor(a.at(k).trimmed());
        sch.inactive << QColor(i.at(k).trimmed());
        sch.disabled << QColor(d.at(k).trimmed());
    }
    *out = sch;
    return true;
}

Scheme adaptRoleCount(const Scheme &s, int roleCount)
{
    if (!s.isValid() || s.size() == roleCount)
        return s;

    Scheme r;
    const auto pad = [roleCount](const QList<QColor> &in) {
        QList<QColor> o = in.mid(0, roleCount);
        while (o.size() < roleCount) {
            const int idx = int(o.size());
            QColor c;
            if (idx == 20 && in.size() > 6) {          // PlaceholderText
                c = in.at(6);                             // Text
                c.setAlpha(128);
            } else if (idx == 21 && in.size() > 12) {  // Accent
                c = in.at(12);                            // Highlight
            } else {
                c = in.isEmpty() ? QColor(Qt::black) : in.at(6 < in.size() ? 6 : 0);
            }
            o << c;
        }
        return o;
    };
    r.active = pad(s.active);
    r.inactive = pad(s.inactive);
    r.disabled = pad(s.disabled);
    return r;
}

Scheme fromPalette(const QPalette &p, int roleCount)
{
    Scheme s;
    const int n = qMin(roleCount, int(QPalette::NColorRoles));
    for (int i = 0; i < n; ++i) {
        const auto role = QPalette::ColorRole(i);
        s.active << p.color(QPalette::Active, role);
        s.inactive << p.color(QPalette::Inactive, role);
        s.disabled << p.color(QPalette::Disabled, role);
    }
    return adaptRoleCount(s, roleCount);
}

QPalette toPalette(const Scheme &s, const QPalette &base)
{
    QPalette p = base;
    const int n = qMin(s.size(), int(QPalette::NColorRoles));
    for (int i = 0; i < n; ++i) {
        const auto role = QPalette::ColorRole(i);
        p.setColor(QPalette::Active, role, s.active.at(i));
        p.setColor(QPalette::Inactive, role, s.inactive.at(i));
        p.setColor(QPalette::Disabled, role, s.disabled.at(i));
    }
    return p;
}

QString schemeToText(const Scheme &s)
{
    const auto join = [](const QList<QColor> &colors) {
        QStringList names;
        for (const QColor &c : colors)
            names << c.name(QColor::HexArgb);
        return names.join(QStringLiteral(", "));
    };
    return QStringLiteral("[ColorScheme]\nactive_colors=%1\ndisabled_colors=%2\ninactive_colors=%3\n")
        .arg(join(s.active), join(s.disabled), join(s.inactive));
}

bool saveScheme(const QString &path, const Scheme &s, QString *error)
{
    return IniFile::writeTextAtomic(path, schemeToText(s), error);
}

bool isManagedSchemePath(const QString &path)
{
    const QString base = QFileInfo(path).completeBaseName().toLower();
    return base == QLatin1String("noctalia");
}

QStringList kdeColorSchemeDirs()
{
    QStringList out;
    for (const QString &d : KwePaths::dataDirs())
        out << d + QStringLiteral("/color-schemes");
    out << QStringLiteral("/usr/share/color-schemes");
    out.removeDuplicates();
    return out;
}

bool isKdeSchemePath(const QString &path)
{
    return path.endsWith(QLatin1String(".colors"), Qt::CaseInsensitive);
}

QString kdeSchemeName(const QString &path)
{
    QSettings s(path, QSettings::IniFormat);
    const QString name = s.value(QStringLiteral("General/Name")).toString().trimmed();
    return name.isEmpty() ? QFileInfo(path).completeBaseName() : name;
}

QString resolvePath(const QString &path)
{
    QString tmp = path;
    tmp.replace(QLatin1Char('~'), KwePaths::home());
    if (!tmp.contains(QLatin1Char('$')))
        return tmp;

    static const QRegularExpression re(QStringLiteral("\\$([A-Z_]+)\\/"));
    QRegularExpressionMatchIterator it = re.globalMatch(tmp);
    while (it.hasNext()) {
        const QString name = it.next().captured(1);
        tmp.replace(QLatin1Char('$') + name, qEnvironmentVariable(name.toLatin1().constData()));
    }
    return tmp;
}

QList<SchemeEntry> findSchemes(QtTarget t)
{
    QList<SchemeEntry> out;
    QSet<QString> seen;

    QStringList dirs{userColorSchemesDir(t)};
    dirs << sharedColorSchemeDirs(t);
    for (const QString &dir : std::as_const(dirs)) {
        const QDir d(dir);
        for (const QFileInfo &fi : d.entryInfoList(QStringList{QStringLiteral("*.conf")}, QDir::Files,
                                                   QDir::Name)) {
            const QString path = fi.absoluteFilePath();
            if (seen.contains(path))
                continue;
            seen.insert(path);
            SchemeEntry e;
            e.path = path;
            e.managed = isManagedSchemePath(path);
            e.name = fi.completeBaseName();
            e.writable = fi.isWritable() && !e.managed;
            out << e;
        }
    }

    // KColorScheme files: only the Qt6 platform theme reads them.
    if (t == QtTarget::Qt6) {
        for (const QString &dir : kdeColorSchemeDirs()) {
            const QDir d(dir);
            for (const QFileInfo &fi : d.entryInfoList(QStringList{QStringLiteral("*.colors")}, QDir::Files,
                                                       QDir::Name)) {
                const QString path = fi.absoluteFilePath();
                if (seen.contains(path))
                    continue;
                seen.insert(path);
                SchemeEntry e;
                e.path = path;
                e.name = kdeSchemeName(path);
                e.managed = isManagedSchemePath(path);
                e.kde = true;
                out << e;
            }
        }
    }
    return out;
}

// ------------------------------------------------------------------ fonts ----

int qt6WeightToQt5(int w)
{
    if (w <= 150) return 0;    // Thin
    if (w <= 250) return 12;   // ExtraLight
    if (w <= 350) return 25;   // Light
    if (w <= 450) return 50;   // Normal
    if (w <= 550) return 57;   // Medium
    if (w <= 650) return 63;   // DemiBold
    if (w <= 750) return 75;   // Bold
    if (w <= 850) return 81;   // ExtraBold
    return 87;                 // Black
}

QString fontToConfigString(const QFont &f, QtTarget target)
{
    if (target == QtTarget::Qt6)
        return f.toString();

    const QStringList fields{f.family(),
                             QString::number(f.pointSizeF()),
                             QString::number(f.pixelSize()),
                             QString::number(int(f.styleHint())),
                             QString::number(qt6WeightToQt5(f.weight())),
                             QString::number(int(f.style())),
                             QString::number(f.underline() ? 1 : 0),
                             QString::number(f.strikeOut() ? 1 : 0),
                             QString::number(f.fixedPitch() ? 1 : 0),
                             QStringLiteral("0")};
    return fields.join(QLatin1Char(','));
}

// ------------------------------------------------------------ config writes ---

StepResult applyUpdates(QtTarget t, const QList<IniFile::Update> &updates, const QString &what)
{
    if (updates.isEmpty())
        return StepResult::ok();
    QString err;
    if (!IniFile::applyUpdates(configFile(t), updates, &err))
        return StepResult::fail(err);
    return StepResult::ok(QStringLiteral("%1: %2").arg(toolName(t), what));
}

IniFile::Update setString(const QString &section, const QString &key, const QString &value)
{
    return IniFile::Update::set(section, key, IniFile::encodeValue(value));
}

IniFile::Update setBool(const QString &section, const QString &key, bool value)
{
    return IniFile::Update::set(section, key, value ? QStringLiteral("true") : QStringLiteral("false"));
}

IniFile::Update setInt(const QString &section, const QString &key, int value)
{
    return IniFile::Update::set(section, key, QString::number(value));
}

IniFile::Update setList(const QString &section, const QString &key, const QStringList &value)
{
    return IniFile::Update::set(section, key, IniFile::encodeList(value));
}

}  // namespace QtCt
