Name:           lgl-kwe-look
Version:        0.0.1
Release:        1%{?dist}
Summary:        One appearance tool for GTK, Qt5 and Qt6 in a KineticWE session

# MIT: this project and nwg-look. BSD-2-Clause: qt5ct / qt6ct derived code. MPL-2.0: the
# stylepak port (src/core/flatpak.*).
License:        MIT AND BSD-2-Clause AND MPL-2.0
URL:            https://github.com/linuxgamerlife/lgl-kwe-look
Source0:        https://github.com/linuxgamerlife/lgl-kwe-look/archive/refs/tags/v%{version}.tar.gz

BuildRequires:  cmake >= 3.16
BuildRequires:  gcc-c++
BuildRequires:  qt6-qtbase-devel
BuildRequires:  qt6-qttools-devel
BuildRequires:  gtk3-devel
BuildRequires:  desktop-file-utils
BuildRequires:  libappstream-glib

# Runtime dependencies.
# qt5ct and qt6ct provide the platform-theme plugins that apply the settings inside Qt apps;
# this package only edits their configuration, so they are Requires, not Conflicts (the stock
# qt5ct / qt6ct / nwg-look desktop entries can stay installed).
# gtk3 is for the preview helper, dconf + gsettings-desktop-schemas for gsettings persistence,
# qt6-qtsvg to show SVG icon themes.
Requires:       qt6-qtbase
Requires:       qt6-qtsvg
Requires:       qt5ct
Requires:       qt6ct
Requires:       gtk3
Requires:       dconf
Requires:       gsettings-desktop-schemas

%description
LGL KWE Look replaces nwg-look, qt5ct and qt6ct with a single native Qt6 app. Choose the GTK
theme, icon theme, mouse cursor and fonts once, style Qt5 and Qt6 apps, and apply everything in
one step.

In a KineticWE session the icon and cursor choice is written to every place that reads it, so it
survives the next login. The Noctalia colour scheme is shown as live and is never edited.

%prep
%autosetup

%build
%cmake -DBUILD_TESTS=OFF
%cmake_build

%install
%cmake_install

%check
desktop-file-validate %{buildroot}%{_datadir}/applications/com.linuxgamerlife.lgl-kwe-look.desktop
appstream-util validate-relax --nonet \
    %{buildroot}%{_datadir}/metainfo/com.linuxgamerlife.lgl-kwe-look.metainfo.xml

%post
if [ -x /usr/bin/gtk-update-icon-cache ]; then
    gtk-update-icon-cache -f -t %{_datadir}/icons/hicolor &>/dev/null || :
fi
if [ -x /usr/bin/update-desktop-database ]; then
    update-desktop-database -q %{_datadir}/applications &>/dev/null || :
fi

%postun
if [ -x /usr/bin/gtk-update-icon-cache ]; then
    gtk-update-icon-cache -f -t %{_datadir}/icons/hicolor &>/dev/null || :
fi
if [ -x /usr/bin/update-desktop-database ]; then
    update-desktop-database -q %{_datadir}/applications &>/dev/null || :
fi

%files
%license %{_datadir}/licenses/%{name}/LICENSE
%license %{_datadir}/licenses/%{name}/BSD-2-Clause-qtct.txt
%license %{_datadir}/licenses/%{name}/MIT-nwg-look.txt
%license %{_datadir}/licenses/%{name}/MPL-2.0-stylepak.txt
%doc README.md CHANGELOG.md
%{_bindir}/lgl-kwe-look
%dir %{_libexecdir}/lgl-kwe-look
%{_libexecdir}/lgl-kwe-look/lgl-kwe-look-gtk-preview
%{_datadir}/lgl-kwe-look/
%{_datadir}/applications/com.linuxgamerlife.lgl-kwe-look.desktop
%{_datadir}/metainfo/com.linuxgamerlife.lgl-kwe-look.metainfo.xml
%{_datadir}/icons/hicolor/scalable/apps/com.linuxgamerlife.lgl-kwe-look.svg

%changelog
* Mon Sep 21 2026 LinuxGamerLife - 0.0.1-1
- First development release
