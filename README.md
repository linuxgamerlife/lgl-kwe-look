<div align="center">

# LGL KWE Look

**GTK, Qt5 and Qt6 theming in one native Qt app — replaces nwg-look, qt5ct and qt6ct in a KineticWE session.**

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)
[![Fedora](https://img.shields.io/badge/Fedora-44-blue?logo=fedora&logoColor=white)](https://fedoraproject.org)
[![Qt](https://img.shields.io/badge/Qt-6-green?logo=qt&logoColor=white)](https://www.qt.io)
<p align="center">
  <a href="https://ko-fi.com/G2G3V70LW">
    <img src="https://storage.ko-fi.com/cdn/kofi6.png?v=6" height="36" alt="Buy Me a Coffee at ko-fi.com" />
  </a>
</p>
</div>

---

## Overview

One Qt6 app that configures the GTK theme, icon theme, mouse cursor and fonts, styles Qt5 and Qt6 apps, and applies everything in one step.

- No privileged helper — everything writes under `$HOME` or through user-level gsettings
- GTK4 export never destroys Noctalia's colours
- Own window state in `~/.config/lgl-kwe-look/`

---

## Install

### RPM from Releases

Download the `.rpm` for your Fedora version from [GitHub Releases](https://github.com/linuxgamerlife/lgl-kwe-look/releases) and double-click to install via Discover.

> After installing from Discover, close it and launch the app from your application menu rather than from the Discover install screen.

---

## Build from Source

```bash
# Install build dependencies
sudo dnf install cmake gcc-c++ qt6-qtbase-devel qt6-qttools-devel gtk3-devel

# Clone and build
git clone https://github.com/linuxgamerlife/lgl-kwe-look.git
cd lgl-kwe-look
mkdir build && cd build
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
```

---

## License

MIT — see [LICENSE](LICENSE). Derived code keeps its upstream licence: nwg-look (MIT), qt5ct / qt6ct (BSD-2-Clause) and the stylepak port in `src/core/flatpak.*` (MPL-2.0). See `LICENSES/`.

---

<div align="center">
Made for <a href="https://fedoraproject.org">Fedora</a> · by <a href="https://www.youtube.com/@linuxgamerlife">LinuxGamerLife</a>
</div>
