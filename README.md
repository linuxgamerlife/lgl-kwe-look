# LGL KWE Look

One native Qt6 app that replaces **nwg-look**, **qt5ct** and **qt6ct** in a
KineticWE session (Fedora 44).

Choose the GTK theme, icon theme, mouse cursor and fonts once, style Qt5 and Qt6 apps, and apply
everything in one step. The look follows [LGL System Loadout](../lgl-system-loadout): native Qt
widgets, a title and subtitle on every page, framed panels and green/orange badges.

> **Status: first draft, not yet compiled or run.** See [Verification](#verification).

## What it configures

| Area | Writes |
|---|---|
| **Icons** and **Cursors** (shared) | `appearance.kwe`, `kineticwe.kwe [Mouse]`, `kdeglobals`, `qt5ct.conf` + `qt6ct.conf` `icon_theme`, gsettings, then `KGlobalSettings.notifyChange` |
| **GTK** theme, fonts, other | gsettings, `gtk-3.0` / `gtk-4.0` `settings.ini`, `~/.gtkrc-2.0`, `xsettingsd.conf`, `~/.icons/default/index.theme`, optional GTK4 symlinks, optional Flatpak overrides |
| **Qt6** / **Qt5** appearance, fonts, interface, style sheets, troubleshooting | `~/.config/qt6ct/qt6ct.conf`, `~/.config/qt5ct/qt5ct.conf` (only the keys that changed) |

Icons and cursors are top-level pages, not per toolkit: in a KineticWE session one choice has to
reach every sink at once, or it reverts at the next login (`start-kineticwe.sh` step 4h rewrites
`icon_theme` in both `qt*ct.conf` files from `appearance.kwe`). The sink set and order mirror
Kinetic Settings' Icons and Cursors pages, so the two tools stay idempotent with each other.

**Link Qt5 and Qt6** (on by default) writes `qt5ct.conf` and `qt6ct.conf` together, each with
values in its own format (palette role count and `QFont` string differ between the majors).

## Design decisions

- **Config UI only.** The runtime plugins (`libqt5ct.so`, `libqt6ct.so`, the proxy styles) come
  from Fedora's `qt5ct` and `qt6ct` packages, which KineticWE already depends on. Those plugins are
  loaded into *other* processes, locked to one Qt major and use Qt private API, so they cannot be
  merged into a Qt6 app.
- **KineticWE-only.** `appearance.kwe` is the source of truth for the icon and cursor theme. The
  session-owned values are locked: `standard_dialogs` is fixed to `xdgdesktopportal`, and
  `colors/noctalia.conf` is shown as *Noctalia (live)* and is never edited, renamed or removed.
- **No privileged helper.** Every write is under `$HOME` or goes through user-level `gsettings`.
- **The GTK4 export never destroys Noctalia's colours.** nwg-look's `clearGtk4Symlinks()` deletes
  `gtk-4.0/gtk.css`, `gtk-dark.css`, `assets` and `settings.ini`. Here only symlinks that point into
  a `themes` directory are removed, a regular file is never replaced, `settings.ini` is never
  deleted, and the export is off by default.
- **Own state in `~/.config/lgl-kwe-look/`.** Upstream qt6ct saves its window geometry in
  `qt6ct.conf`, which makes the plugin reload every running Qt app whenever the window closes.
- **gsettings writes are read back.** Without a session bus or dconf, gsettings falls back to an
  in-memory backend and `set` exits 0. Every key is read back in a fresh process.
- **GTK preview** is rendered by a small separate GTK3 helper (`lgl-kwe-look-gtk-preview`,
  offscreen window, long-lived, line protocol on stdin) and shown as an image.
- **Qt5 is edited from a Qt6 process.** Style and platform-theme lists for Qt5 come from scanning
  `/usr/lib64/qt5/plugins`; the Qt5 style preview is approximate and says so.

## Command line

The nwg-look flags keep working (they run without a display):

```
lgl-kwe-look -a    apply the stored gsettings backup and quit
lgl-kwe-look -x    export the GTK config files from the current settings and quit
lgl-kwe-look -r    restore default GTK settings (asks first, -y skips the question)
lgl-kwe-look -v    print the version
```

On first start, `~/.config/nwg-look/config` and `~/.local/share/nwg-look/gsettings` are imported
once. nwg-look's GTK4-symlink preference is not imported in a KineticWE session.

## Build

Development builds go through the `fedora-dev1` distrobox, like the other LGL apps:

```
cmake -B build -DCMAKE_BUILD_TYPE=Debug     # -Wall -Wextra -Werror + ASan/UBSan
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Build dependencies (Fedora): `cmake gcc-c++ qt6-qtbase-devel qt6-qttools-devel gtk3-devel`.
Options: `-DBUILD_GTK_PREVIEW=OFF`, `-DBUILD_TESTS=OFF`. The project pins C++17 (Qt 6.10 headers in
C++20 mode break on Fedora 43+).

RPM: `packaging/lgl-kwe-look.spec` (COPR via SCM, like the benchmark). Runtime requires
`qt6-qtbase qt6-qtsvg qt5ct qt6ct gtk3 dconf gsettings-desktop-schemas`.

## Translations

Every string goes through `tr()`. To seed the `.ts` files from qt6ct's translations and nwg-look's
JSON vocabularies:

```
python3 scripts/fill-translations.py --init          # once
cmake --build build --target update_translations     # lupdate
python3 scripts/fill-translations.py                 # fill what upstream already knows
```

Filled entries stay `unfinished` so a translator reviews them.

## Layout

```
src/core/      config I/O, gsettings, theme scan, xcursor decoder, GTK export, Flatpak, qt*ct, sinks
src/pages/     Overview, Icons, Cursors, gtk/ (4 pages), qt/ (5 pages, one class set per Qt major)
helpers/gtk-preview/   offscreen GTK3 renderer
tests/         QtTest suite (temporary HOME, offscreen platform)
colors/ qss/   shipped colour schemes and style sheets (from qt6ct)
source/        the vendored upstream trees this was ported from (not built)
```

## Not ported

- qt5ct/qt6ct's **fontconfig dialog** (creating and removing `~/.config/fontconfig/fonts.conf`).
- The plugins themselves (see above).

## Verification

Nothing has been compiled or run yet. To do, in a KineticWE VM session:

1. Debug build (ASan + UBSan) and `ctest`.
2. After Apply, check `appearance.kwe`, `kineticwe.kwe [Mouse]`, `kdeglobals`, `gsettings get ...`
   and `icon_theme` in both `qt*ct.conf`; then re-run the login sync and confirm nothing reverts.
3. Checksums of `~/.config/gtk-4.0/gtk.css`, `gtk-3.0/gtk.css` and `colors/noctalia.conf` unchanged
   after Apply in every GTK export mode (`tests/tst_sinks.cpp` covers this against a temporary HOME).
4. A running Qt5, Qt6, GTK3 and GTK4 app restyle live; the cursor changes in the compositor.
5. `gtk-preview` under Wayland, dark and light, HiDPI.
6. CodeQL and flawfinder clean; side-by-side with a benchmark screenshot.

Also unverified: that Qt6's `QFont::fromString` reads the Qt5 10-field format
(`tst_qtct.cpp: qt6ReadsBothFontFormats` checks it).

## Licence

MIT. Derived code keeps its upstream licence: nwg-look (MIT), qt5ct / qt6ct (BSD-2-Clause) and the
stylepak port in `src/core/flatpak.*` (**MPL-2.0**, file-level). See `LICENSES/`.
