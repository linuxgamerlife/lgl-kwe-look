# Changelog

## 0.1.0 (unreleased)

First development draft. Not yet compiled or run.

- Sidebar app with Overview, Icons, Cursors, four GTK pages and five Qt pages per Qt major.
- One Apply that writes an ordered set of sinks, with a per-sink ok / warn / fail log.
- Icon and cursor choice written to `appearance.kwe`, `kineticwe.kwe`, `kdeglobals`, `qt5ct.conf`,
  `qt6ct.conf` and gsettings, then announced over D-Bus.
- Line-preserving `qt5ct.conf` / `qt6ct.conf` writer (atomic, only changed keys, QSettings-compatible
  encoding); palette files adapted between 21 (Qt5) and 22 (Qt6) roles; per-target `QFont` strings.
- GTK export (`settings.ini`, `gtkrc-2.0`, `xsettingsd.conf`, `index.theme`, GTK4 symlinks) that never
  replaces a regular file; gsettings writes verified by read-back.
- Native Xcursor decoder for the cursor preview; offscreen GTK3 helper for the theme preview.
- nwg-look flags `-a`, `-x`, `-r`, `-v` and a one-time import of nwg-look's state.
- QtTest suite, RPM spec, AppStream metadata, CodeQL and flawfinder workflows.
- Session detection (KineticWE, KDE, GNOME, Xfce, Cinnamon, MATE, LXQt, Budgie, other): the Overview says
  which session was detected and lists the files an Apply writes for it. A KDE session also writes
  `kcminputrc`; qt5ct/qt6ct icon mirrors are only written on a full desktop when those tools are in use.
- KDE colour schemes (KColorScheme `.colors`) are listed for Qt6 and previewed; the Noctalia scheme
  and the GTK preview follow the shell's colour changes live.
- A "?" explainer next to every setting, a resize grip on the main window and the dialogs, and a
  larger font picker.
