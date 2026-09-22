#include "qtappearancepage.h"

#include "appmodel.h"
#include "core/kwepaths.h"
#include "core/session.h"
#include "paletteeditdialog.h"
#include "previewpanel.h"

#include <QAction>
#include <QComboBox>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QFileSystemWatcher>
#include <QFormLayout>
#include <QInputDialog>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QSettings>
#include <QStyle>
#include <QStyleFactory>
#include <QTimer>

namespace {

const QString kSystem = QStringLiteral("system");
const QString kStyle = QStringLiteral("style");

// The session's live scheme and KDE colour schemes cannot be edited, renamed or removed here.
bool isReadOnlyScheme(const QString &path)
{
    return QtCt::isManagedSchemePath(path) || QtCt::isKdeSchemePath(path);
}

}  // namespace

QtAppearancePage::QtAppearancePage(QtTarget target, AppModel *model, QWidget *parent)
    : QtPageBase(target, parent), m_model(model)
{
    auto *root = initPage(tr("%1 Appearance").arg(QtCt::majorName(target)),
                          tr("Style, colour scheme and standard dialogs for %1 apps, applied by the %2 platform "
                             "theme.")
                              .arg(QtCt::majorName(target), QtCt::toolName(target)));

    QVBoxLayout *pl = nullptr;
    QFrame *panel = makePanel(this, &pl);
    auto *form = new QFormLayout;

    m_style = new QComboBox(panel);
    m_style->addItems(QtCt::availableStyles(target));
    form->addRow(tr("Style"),
                 withHelp(m_style, tr("The widget style Qt apps draw their buttons, menus and other controls "
                                      "with, for example Fusion or Breeze. Only styles that are installed are "
                                      "listed.")));

    auto *schemeRow = new QHBoxLayout;
    m_scheme = new QComboBox(panel);
    m_scheme->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    m_schemeMenu = makeToolbarBtn(tr("Palette"), panel);
    auto *menu = new QMenu(m_schemeMenu);
    menu->addAction(tr("Create"), this, &QtAppearancePage::createScheme);
    m_edit = menu->addAction(tr("Edit"), this, &QtAppearancePage::editScheme);
    menu->addAction(tr("Create a Copy"), this, &QtAppearancePage::copyScheme);
    m_rename = menu->addAction(tr("Rename"), this, &QtAppearancePage::renameScheme);
    menu->addSeparator();
    m_remove = menu->addAction(tr("Remove"), this, &QtAppearancePage::removeScheme);
    m_schemeMenu->setMenu(menu);
    schemeRow->addWidget(m_scheme, 1);
    schemeRow->addWidget(m_schemeMenu);
    schemeRow->addWidget(makeHelpButton(panel, tr("The colours Qt apps use. Default follows the style, and Style's "
                                                  "colours copies the style's own palette. Named schemes come from "
                                                  "qt5ct/qt6ct, or from KDE colour scheme files (KColorScheme, Qt6 "
                                                  "only). A \"live\" scheme is rewritten by the desktop, such as "
                                                  "Noctalia, and is read-only. The Palette menu creates, edits, "
                                                  "copies, renames and removes your own schemes.")));
    form->addRow(tr("Colour scheme"), schemeRow);

    m_dialogs = new QComboBox(panel);
    m_dialogs->addItem(tr("Default"), QStringLiteral("default"));
    for (const QString &key : QtCt::availablePlatformThemes(target)) {
        if (key == QLatin1String("gtk3") || key == QLatin1String("gtk2"))
            m_dialogs->addItem(key.toUpper(), key);
        else if (key == QLatin1String("kde"))
            m_dialogs->addItem(QStringLiteral("KDE"), key);
        else if (key == QLatin1String("xdgdesktopportal"))
            m_dialogs->addItem(tr("XDG Desktop Portal"), key);
    }
    form->addRow(tr("Standard dialogs"),
                 withHelp(m_dialogs, tr("Which file, colour and font dialogs Qt apps use: Qt's own, the GTK or KDE "
                                        "ones, or the desktop portal. In a KineticWE session this is fixed to XDG "
                                        "Desktop Portal.")));
    pl->addLayout(form);

    m_notes = new QLabel(panel);
    m_notes->setWordWrap(true);
    m_notes->setTextFormat(Qt::RichText);
    pl->addWidget(m_notes);
    root->addWidget(panel);

    QVBoxLayout *vl = nullptr;
    QFrame *previewFrame = makePanel(this, &vl);
    auto *head = new QHBoxLayout;
    head->addWidget(makeSectionTitle(previewFrame, tr("Preview")));
    head->addStretch();
    m_group = new QComboBox(previewFrame);
    m_group->addItems({tr("Active palette"), tr("Inactive palette"), tr("Disabled palette")});
    head->addWidget(withHelp(m_group, tr("Which set of colours the preview shows: Active is a focused window, "
                                         "Inactive an unfocused one, and Disabled the greyed-out controls.")));
    vl->addLayout(head);
    m_preview = new PreviewPanel(previewFrame);
    vl->addWidget(m_preview, 1);
    root->addWidget(previewFrame, 1);

    // The shell rewrites its scheme on every wallpaper change, often in several writes.
    m_watcher = new QFileSystemWatcher(this);
    m_reloadTimer = new QTimer(this);
    m_reloadTimer->setSingleShot(true);
    m_reloadTimer->setInterval(200);
    connect(m_watcher, &QFileSystemWatcher::fileChanged, m_reloadTimer, qOverload<>(&QTimer::start));
    connect(m_watcher, &QFileSystemWatcher::directoryChanged, m_reloadTimer, qOverload<>(&QTimer::start));
    connect(m_reloadTimer, &QTimer::timeout, this, &QtAppearancePage::reloadManagedScheme);

    connect(m_style, qOverload<int>(&QComboBox::activated), this, [this]() {
        applyPreview();
        updateNotes();
        edited();
    });
    connect(m_scheme, qOverload<int>(&QComboBox::activated), this, [this]() {
        onSchemeChosen();
        edited();
    });
    connect(m_dialogs, qOverload<int>(&QComboBox::activated), this, [this]() { edited(); });
    connect(m_group, qOverload<int>(&QComboBox::activated), this, [this]() { applyPreview(); });
    connect(m_model, &AppModel::linkChanged, this, [this]() { updateNotes(); });
    connect(m_schemeMenu->menu(), &QMenu::aboutToShow, this, [this]() {
        const bool file = m_scheme->currentData().toString() != kSystem
                       && m_scheme->currentData().toString() != kStyle;
        const QFileInfo fi(m_scheme->currentData().toString());
        const bool writable = file && fi.isWritable() && !isReadOnlyScheme(fi.filePath());
        m_edit->setEnabled(writable);
        m_rename->setEnabled(writable);
        m_remove->setEnabled(writable);
    });

    load();
}

void QtAppearancePage::load()
{
    QSettings s(QtCt::configFile(m_target), QSettings::IniFormat);
    s.beginGroup(QStringLiteral("Appearance"));
    const QString style = s.value(QStringLiteral("style"), QStringLiteral("Fusion")).toString();
    const bool custom = s.value(QStringLiteral("custom_palette"), false).toBool();
    const QString path = QtCt::resolvePath(s.value(QStringLiteral("color_scheme_path")).toString());
    const QString dialogs = s.value(QStringLiteral("standard_dialogs"), QStringLiteral("default")).toString();
    s.endGroup();

    int si = m_style->findText(style, Qt::MatchFixedString);   // case-insensitive, like the plugins
    if (si < 0) {
        m_style->addItem(style);
        si = m_style->count() - 1;
    }
    m_style->setCurrentIndex(si);

    int di = m_dialogs->findData(dialogs);
    if (di < 0) {
        m_dialogs->addItem(dialogs, dialogs);
        di = m_dialogs->count() - 1;
    }
    m_dialogs->setCurrentIndex(di);
    // The session ships its own portal: the value is fixed, not chosen.
    m_dialogs->setEnabled(!KwePaths::isKineticWeSession());
    if (KwePaths::isKineticWeSession()) {
        const int px = m_dialogs->findData(QStringLiteral("xdgdesktopportal"));
        if (px >= 0)
            m_dialogs->setCurrentIndex(px);
    }

    populateSchemes(path, custom);
    onSchemeChosen();
    loaded();
}

void QtAppearancePage::populateSchemes(const QString &selectedPath, bool custom)
{
    const QSignalBlocker b(m_scheme);
    m_scheme->clear();
    m_scheme->addItem(tr("Default"), kSystem);
    m_scheme->addItem(tr("Style's colours"), kStyle);
    m_scheme->insertSeparator(2);

    bool found = false;
    for (const QtCt::SchemeEntry &e : QtCt::findSchemes(m_target)) {
        const QString shown = e.managed ? e.name.left(1).toUpper() + e.name.mid(1) : e.name;
        const QString label = e.kde ? (e.managed ? tr("%1 (KColorScheme, live)") : tr("%1 (KColorScheme)")).arg(shown)
                              : e.managed ? tr("%1 (live)").arg(shown)
                                          : shown;
        m_scheme->addItem(label, e.path);
        if (custom && e.path == selectedPath) {
            m_scheme->setCurrentIndex(m_scheme->count() - 1);
            found = true;
        }
    }

    if (!custom) {
        m_scheme->setCurrentIndex(m_scheme->findData(kSystem));
    } else if (selectedPath == QtCt::styleColorsFile(m_target)) {
        m_scheme->setCurrentIndex(m_scheme->findData(kStyle));
    } else if (!found && !selectedPath.isEmpty()) {
        // A scheme outside the known folders (for example one written by the session):
        // keep it selectable or Apply would silently replace it.
        const QString base = QFileInfo(selectedPath).completeBaseName();
        m_scheme->addItem(QtCt::isManagedSchemePath(selectedPath) ? tr("%1 (live)").arg(base) : tr("%1 (external)").arg(base),
                          selectedPath);
        m_scheme->setCurrentIndex(m_scheme->count() - 1);
    }
}

QString QtAppearancePage::schemeData() const
{
    return m_scheme->currentData().toString();
}

void QtAppearancePage::onSchemeChosen()
{
    const QString data = schemeData();
    if (data != kSystem && data != kStyle) {
        QtCt::Scheme s;
        m_custom = QtCt::loadScheme(data, &s) ? s : QtCt::Scheme();
    }
    applyPreview();
    updateNotes();
    watchScheme();
}

// Only the session's live scheme is watched: user schemes change only through this page, and
// reloading one would drop the unsaved edits of the palette dialog.
void QtAppearancePage::watchScheme()
{
    const QStringList old = m_watcher->files() + m_watcher->directories();
    if (!old.isEmpty())
        m_watcher->removePaths(old);

    const QString data = schemeData();
    if (!QtCt::isManagedSchemePath(data))
        return;
    // The directory catches a file that is replaced by a rename, which drops the file watch.
    m_watcher->addPath(QFileInfo(data).absolutePath());
    if (QFileInfo::exists(data))
        m_watcher->addPath(data);
}

void QtAppearancePage::reloadManagedScheme()
{
    const QString data = schemeData();
    if (!QtCt::isManagedSchemePath(data))
        return;
    QtCt::Scheme s;
    if (QtCt::loadScheme(data, &s)) {
        m_custom = s;
        applyPreview();
    }
    watchScheme();
}

void QtAppearancePage::applyPreview()
{
    const QString name = m_style->currentText();
    QStyle *style = QStyleFactory::create(name);
    m_approximate = false;
    if (!style) {
        // A Qt5-only style cannot be instantiated from this Qt6 process.
        style = QStyleFactory::create(QStringLiteral("Fusion"));
        m_approximate = true;
    } else if (m_target == QtTarget::Qt5) {
        m_approximate = true;   // same-named Qt6 style, not the Qt5 one
    }
    style->setParent(this);
    m_preview->setPreviewStyle(style);
    delete m_previewStyle;
    m_previewStyle = style;

    QPalette base = style->standardPalette();
    const QString data = schemeData();
    if (data != kSystem && data != kStyle && m_custom.isValid())
        base = QtCt::toPalette(QtCt::adaptRoleCount(m_custom, QPalette::NColorRoles), base);

    const QPalette::ColorGroup group = m_group->currentIndex() == 0   ? QPalette::Active
                                       : m_group->currentIndex() == 1 ? QPalette::Inactive
                                                                      : QPalette::Disabled;
    m_preview->setPreviewPalette(base, group);
}

void QtAppearancePage::updateNotes()
{
    QStringList notes;
    if (m_approximate) {
        notes << tr("This is an approximate preview: %1 apps use the %1 build of the style, which this window "
                    "cannot load. Colours are exact.")
                     .arg(QtCt::majorName(m_target));
    }

    bool noctaliaPresent = QFileInfo::exists(QtCt::userColorSchemesDir(m_target) + QStringLiteral("/noctalia.conf"));
    for (const QString &dir : QtCt::kdeColorSchemeDirs())
        noctaliaPresent = noctaliaPresent || QFileInfo::exists(dir + QStringLiteral("/noctalia.colors"));

    const QString data = schemeData();
    if (QtCt::isManagedSchemePath(data)) {
        notes << tr("<b>Noctalia (live)</b> is rewritten by the shell whenever the wallpaper colours change. It "
                    "is read-only here.");
    } else if (data != kSystem && data != kStyle && noctaliaPresent) {
        notes << tr("<span style='color:#cc7700;'><b>Note:</b></span> this replaces the Noctalia palette, so Qt "
                    "apps stop following the shell's colours until you choose Noctalia again.");
    }
    if (KwePaths::isKineticWeSession())
        notes << tr("Standard dialogs are fixed to XDG Desktop Portal: the KineticWE session ships its own portal.");
    if (Session::detect().type == Session::Type::Kde
        && !qEnvironmentVariable("QT_QPA_PLATFORMTHEME").contains(QtCt::toolName(m_target))) {
        notes << tr("<span style='color:#cc7700;'><b>KDE session:</b></span> Plasma uses its own Qt platform theme, "
                    "so these settings only reach apps started with QT_QPA_PLATFORMTHEME=%1.")
                     .arg(QtCt::toolName(m_target));
    }

    if (m_model->prefs.linkQt) {
        const QtTarget other = m_target == QtTarget::Qt6 ? QtTarget::Qt5 : QtTarget::Qt6;
        const QStringList otherStyles = QtCt::availableStyles(other);
        if (!otherStyles.contains(m_style->currentText(), Qt::CaseInsensitive)) {
            notes << tr("<span style='color:#cc7700;'><b>Note:</b></span> the style %1 is not installed for %2; "
                        "%2 apps will fall back to their default style.")
                         .arg(m_style->currentText(), QtCt::majorName(other));
        }
        if (QtCt::isKdeSchemePath(data)) {
            notes << tr("<span style='color:#cc7700;'><b>Note:</b></span> KColorScheme files are read by the Qt6 "
                        "platform theme only; Qt5 apps keep their current colours.");
        }
    }
    m_notes->setText(notes.join(QStringLiteral("<br>")));
    m_notes->setVisible(!notes.isEmpty());
}

QVariantMap QtAppearancePage::snapshot() const
{
    QVariantMap m;
    m[QStringLiteral("style")] = m_style->currentText();
    m[QStringLiteral("scheme")] = schemeData();
    m[QStringLiteral("dialogs")] = m_dialogs->currentData().toString();
    // A palette edited in the dialog is saved to its file immediately, so the file's
    // modification is not part of the draft.
    return m;
}

int QtAppearancePage::writeRoleCount(bool linked) const
{
    // A 22-role file works for both plugins (Qt5 reads the first 21), so linked uses Qt6's.
    return linked ? QtCt::paletteRoleCount(QtTarget::Qt6) : QtCt::paletteRoleCount(m_target);
}

QList<IniFile::Update> QtAppearancePage::updatesFor(QtTarget target) const
{
    const QString data = schemeData();
    QList<IniFile::Update> u;
    u << QtCt::setString(QStringLiteral("Appearance"), QStringLiteral("style"), m_style->currentText());

    if (data == kSystem) {
        u << QtCt::setBool(QStringLiteral("Appearance"), QStringLiteral("custom_palette"), false);
    } else if (target == QtTarget::Qt5 && QtCt::isKdeSchemePath(data)) {
        // qt5ct cannot read a KColorScheme: leave its colour keys as they are.
    } else {
        // "Style's colours" is written to the page target's style-colors.conf (see addToPlan).
        const QString path = data == kStyle ? QtCt::styleColorsFile(m_target) : data;
        u << QtCt::setBool(QStringLiteral("Appearance"), QStringLiteral("custom_palette"), true)
          << QtCt::setString(QStringLiteral("Appearance"), QStringLiteral("color_scheme_path"), path);
    }

    const QString dialogs = KwePaths::isKineticWeSession() ? QStringLiteral("xdgdesktopportal")
                                                           : m_dialogs->currentData().toString();
    u << QtCt::setString(QStringLiteral("Appearance"), QStringLiteral("standard_dialogs"), dialogs);
    return u;
}

void QtAppearancePage::addToPlan(ApplyPlan &plan, bool linked)
{
    if (!isDirty() && !m_force)
        return;

    // "Style's colours": snapshot the style's standard palette into style-colors.conf first.
    if (schemeData() == kStyle && m_previewStyle) {
        const QString path = QtCt::styleColorsFile(m_target);
        const QtCt::Scheme scheme = QtCt::fromPalette(m_previewStyle->standardPalette(), writeRoleCount(linked));
        plan.add(ApplyOrder::QtCtConfig, tr("%1 style colours").arg(QtCt::toolName(m_target)), [path, scheme]() {
            QString err;
            if (!QtCt::saveScheme(path, scheme, &err))
                return StepResult::fail(err);
            return StepResult::ok(path);
        });
    }
    QtPageBase::addToPlan(plan, linked);
}

// ---------------------------------------------------------- scheme actions ----

QString QtAppearancePage::newSchemePath(const QString &title, const QString &suggestion)
{
    bool ok = false;
    QString name = QInputDialog::getText(this, title, tr("File name:"),
                                         QLineEdit::Normal, suggestion, &ok);
    name = name.trimmed();
    if (!ok || name.isEmpty())
        return QString();
    // A name is a file name, never a path.
    if (name.contains(QLatin1Char('/')) || name.contains(QStringLiteral(".."))) {
        QMessageBox::warning(this, tr("Error"), tr("The name is not valid."));
        return QString();
    }
    if (!name.endsWith(QLatin1String(".conf"), Qt::CaseInsensitive))
        name += QStringLiteral(".conf");
    const QString path = QtCt::userColorSchemesDir(m_target) + QLatin1Char('/') + name;
    if (QFileInfo::exists(path)) {
        QMessageBox::warning(this, tr("Error"),
                             tr("The colour scheme \"%1\" already exists.").arg(name.section(QLatin1Char('.'), 0, 0)));
        return QString();
    }
    return path;
}

void QtAppearancePage::createScheme()
{
    const QString path = newSchemePath(tr("Enter Colour Scheme Name"), QString());
    if (path.isEmpty())
        return;
    QString err;
    const QtCt::Scheme s = QtCt::fromPalette(m_previewStyle ? m_previewStyle->standardPalette() : QPalette(),
                                             QtCt::paletteRoleCount(QtTarget::Qt6));
    if (!QtCt::saveScheme(path, QtCt::adaptRoleCount(s, writeRoleCount(m_model->prefs.linkQt)), &err)) {
        QMessageBox::warning(this, tr("Error"), err);
        return;
    }
    populateSchemes(path, true);
    onSchemeChosen();
    edited();
}

void QtAppearancePage::editScheme()
{
    const QString path = schemeData();
    if (path == kSystem || path == kStyle || isReadOnlyScheme(path))
        return;
    if (!QFileInfo(path).isWritable()) {
        QMessageBox::information(this, tr("Warning"), tr("The colour scheme \"%1\" is read only.")
                                                          .arg(QFileInfo(path).completeBaseName()));
        return;
    }

    PaletteEditDialog d(QtCt::adaptRoleCount(m_custom, QtCt::paletteRoleCount(QtTarget::Qt6)), this);
    connect(&d, &PaletteEditDialog::schemeChanged, this, [this](const QtCt::Scheme &s) {
        m_custom = s;
        applyPreview();
    });
    if (d.exec() != QDialog::Accepted) {
        onSchemeChosen();   // drop the live edits
        return;
    }
    QString err;
    m_custom = d.scheme();
    if (!QtCt::saveScheme(path, QtCt::adaptRoleCount(m_custom, writeRoleCount(m_model->prefs.linkQt)), &err))
        QMessageBox::warning(this, tr("Error"), err);
    applyPreview();
}

void QtAppearancePage::copyScheme()
{
    const QString src = schemeData();
    if (src == kSystem || src == kStyle) {
        createScheme();
        return;
    }
    const QString path = newSchemePath(tr("Enter Colour Scheme Name"),
                                       tr("%1 (copy)").arg(QFileInfo(src).completeBaseName()));
    if (path.isEmpty())
        return;
    QDir().mkpath(QFileInfo(path).absolutePath());
    if (QtCt::isKdeSchemePath(src)) {
        // A KDE colour scheme becomes an editable qt5ct/qt6ct scheme of the same colours.
        QString err;
        if (!m_custom.isValid()
            || !QtCt::saveScheme(path, QtCt::adaptRoleCount(m_custom, writeRoleCount(m_model->prefs.linkQt)), &err)) {
            QMessageBox::warning(this, tr("Error"), err.isEmpty() ? tr("Unable to copy file") : err);
            return;
        }
    } else if (!QFile::copy(src, path)) {
        QMessageBox::warning(this, tr("Error"), tr("Unable to copy file"));
        return;
    }
    populateSchemes(path, true);
    onSchemeChosen();
    edited();
}

void QtAppearancePage::renameScheme()
{
    const QString src = schemeData();
    if (src == kSystem || src == kStyle || isReadOnlyScheme(src))
        return;
    const QString path = newSchemePath(tr("Enter Colour Scheme Name"), QFileInfo(src).completeBaseName());
    if (path.isEmpty())
        return;
    if (!QFile::rename(src, path)) {
        QMessageBox::warning(this, tr("Error"), tr("Unable to rename file"));
        return;
    }
    populateSchemes(path, true);
    edited();
}

void QtAppearancePage::removeScheme()
{
    const QString src = schemeData();
    if (src == kSystem || src == kStyle || isReadOnlyScheme(src))
        return;
    if (QMessageBox::question(this, tr("Confirm Remove"),
                              tr("Are you sure you want to remove colour scheme \"%1\"?")
                                  .arg(QFileInfo(src).completeBaseName()),
                              QMessageBox::Yes | QMessageBox::No) != QMessageBox::Yes)
        return;
    if (!QFile::remove(src))
        return;
    populateSchemes(QString(), false);
    onSchemeChosen();
    edited();
}
