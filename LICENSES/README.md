# Licences

LGL KWE Look is released under the **MIT** licence (`../LICENSE`). It contains work derived from
three upstream projects. Their notices must stay with the source and with binary packages.

| Upstream | Licence | Where it shows up |
|---|---|---|
| nwg-look 1.1.1 (Piotr Miller and contributors) | MIT | GTK export writers, theme scanning, CLI flags, gsettings handling |
| qt5ct 1.9 / qt6ct 0.10 (Ilya Kotov) | BSD-2-Clause | config keys, palette handling, the Qt appearance / fonts / interface / style-sheet / troubleshooting pages, `colors/*.conf`, `qss/*.qss` |
| stylepak (refi64) via nwg-look's Go port (Eslam Allam) | **MPL-2.0** | `src/core/flatpak.cpp` and `flatpak.h` |

`flatpak.cpp` and `flatpak.h` are a translation of MPL-2.0 code, so **those two files stay under
the MPL-2.0** (file-level copyleft). Everything else is MIT or BSD-2-Clause. The MPL-2.0 files carry an SPDX header;
for the rest, this table is the record of what derives from what.

The runtime plugins (`libqt5ct.so`, `libqt6ct.so` and their proxy styles) are **not** built or
shipped by this project. They come from Fedora's `qt5ct` and `qt6ct` packages.
