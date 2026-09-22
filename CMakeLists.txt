cmake_minimum_required(VERSION 3.16)
project(lgl-kwe-look VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# C++17 is used rather than C++20 (the preferred standard) because compiling
# Qt6 headers in C++20 mode activates C++20-specific Qt code paths that
# reference symbols only present in Qt 6.10+, breaking compatibility with
# the Qt6 version shipped on Fedora 43+ target systems.
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

include(GNUInstallDirs)

option(BUILD_GTK_PREVIEW "Build the GTK3 offscreen preview helper" ON)
option(BUILD_TESTS       "Build the QtTest suite"                  ON)

# Treat warnings as errors on GCC and Clang (dev standard requirement).
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Werror)
endif()

# Enable AddressSanitizer and UndefinedBehaviorSanitizer for Debug builds.
# ThreadSanitizer is mutually exclusive with ASan — use -DCMAKE_BUILD_TYPE=TSan.
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
        add_link_options(-fsanitize=address,undefined)
    endif()
elseif(CMAKE_BUILD_TYPE STREQUAL "TSan")
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        add_compile_options(-g -O1 -fsanitize=thread -fno-omit-frame-pointer)
        add_link_options(-fsanitize=thread)
    endif()
endif()

# Qt6::Svg is a runtime-only dependency (SVG icon themes in the preview), so
# it is intentionally not linked here.
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Concurrent DBus LinguistTools)

# ---------------------------------------------------------------------------
# Core library — everything that touches config files, gsettings, themes.
# No windows or pages in here (only QStyleFactory / QPalette / QImage), so the
# QtTest suite and the CLI mode can link it directly.
# ---------------------------------------------------------------------------
add_library(lglkwe-core STATIC
    src/core/kwepaths.cpp        src/core/kwepaths.h
    src/core/inifile.cpp         src/core/inifile.h
    src/core/stepresult.h
    src/core/applypipeline.cpp   src/core/applypipeline.h
    src/core/gsettingsclient.cpp src/core/gsettingsclient.h
    src/core/themescan.cpp       src/core/themescan.h
    src/core/xcursor.cpp         src/core/xcursor.h
    src/core/themestate.cpp      src/core/themestate.h
    src/core/gtkexport.cpp       src/core/gtkexport.h
    src/core/flatpak.cpp         src/core/flatpak.h
    src/core/qtct.cpp            src/core/qtct.h
    src/core/session.cpp         src/core/session.h
    src/core/sessionsinks.cpp    src/core/sessionsinks.h
    src/core/preferences.cpp     src/core/preferences.h
    src/core/migration.cpp       src/core/migration.h
    src/core/cli.cpp             src/core/cli.h
)
target_include_directories(lglkwe-core PUBLIC ${PROJECT_SOURCE_DIR}/src)
target_link_libraries(lglkwe-core PUBLIC Qt6::Core Qt6::Gui Qt6::Widgets Qt6::DBus)
target_compile_definitions(lglkwe-core PUBLIC
    LGLKWE_VERSION="${PROJECT_VERSION}"
    LGLKWE_DATADIR="${CMAKE_INSTALL_FULL_DATADIR}/lgl-kwe-look"
    LGLKWE_LIBEXECDIR="${CMAKE_INSTALL_FULL_LIBEXECDIR}/lgl-kwe-look")

# ---------------------------------------------------------------------------
# GUI application — Qt Widgets, always runs as the normal user.
# ---------------------------------------------------------------------------
qt_add_executable(lgl-kwe-look
    src/main.cpp
    src/pagehelpers.h
    src/mainwindow.cpp            src/mainwindow.h
    src/appmodel.cpp              src/appmodel.h
    src/gtkpreviewclient.cpp      src/gtkpreviewclient.h
    src/pages/pagebase.h
    src/pages/gtk/gtkbindings.h
    src/pages/overviewpage.cpp    src/pages/overviewpage.h
    src/pages/iconspage.cpp       src/pages/iconspage.h
    src/pages/cursorspage.cpp     src/pages/cursorspage.h
    src/pages/gtk/gtkthemepage.cpp    src/pages/gtk/gtkthemepage.h
    src/pages/gtk/gtkfontspage.cpp    src/pages/gtk/gtkfontspage.h
    src/pages/gtk/gtkotherpage.cpp    src/pages/gtk/gtkotherpage.h
    src/pages/gtk/gtkexportpage.cpp   src/pages/gtk/gtkexportpage.h
    src/pages/qt/qtpagebase.h
    src/pages/qt/previewpanel.cpp         src/pages/qt/previewpanel.h
    src/pages/qt/paletteeditdialog.cpp    src/pages/qt/paletteeditdialog.h
    src/pages/qt/qsseditordialog.cpp      src/pages/qt/qsseditordialog.h
    src/pages/qt/qtappearancepage.cpp     src/pages/qt/qtappearancepage.h
    src/pages/qt/qtfontspage.cpp          src/pages/qt/qtfontspage.h
    src/pages/qt/qtinterfacepage.cpp      src/pages/qt/qtinterfacepage.h
    src/pages/qt/qtstylesheetspage.cpp    src/pages/qt/qtstylesheetspage.h
    src/pages/qt/qttroubleshootingpage.cpp src/pages/qt/qttroubleshootingpage.h
    resources/resources.qrc
)
target_link_libraries(lgl-kwe-look PRIVATE lglkwe-core Qt6::Widgets Qt6::Concurrent)

qt_add_translations(lgl-kwe-look
    TS_FILES
        translations/lgl-kwe-look_de.ts
        translations/lgl-kwe-look_es.ts
        translations/lgl-kwe-look_fr.ts
        translations/lgl-kwe-look_it.ts
        translations/lgl-kwe-look_ja.ts
        translations/lgl-kwe-look_ko.ts
        translations/lgl-kwe-look_pl.ts
        translations/lgl-kwe-look_pt_BR.ts
        translations/lgl-kwe-look_ru.ts
        translations/lgl-kwe-look_tr.ts
        translations/lgl-kwe-look_zh_CN.ts
        translations/lgl-kwe-look_cs.ts
)

install(TARGETS lgl-kwe-look RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(DIRECTORY colors/ DESTINATION ${CMAKE_INSTALL_DATADIR}/lgl-kwe-look/colors
        FILES_MATCHING PATTERN "*.conf")
install(DIRECTORY qss/ DESTINATION ${CMAKE_INSTALL_DATADIR}/lgl-kwe-look/qss
        FILES_MATCHING PATTERN "*.qss")
install(FILES packaging/com.linuxgamerlife.lgl-kwe-look.desktop
        DESTINATION ${CMAKE_INSTALL_DATADIR}/applications)
install(FILES packaging/com.linuxgamerlife.lgl-kwe-look.metainfo.xml
        DESTINATION ${CMAKE_INSTALL_DATADIR}/metainfo)
install(FILES packaging/lgl-kwe-look.svg
        DESTINATION ${CMAKE_INSTALL_DATADIR}/icons/hicolor/scalable/apps
        RENAME com.linuxgamerlife.lgl-kwe-look.svg)
install(FILES LICENSE LICENSES/BSD-2-Clause-qtct.txt LICENSES/MIT-nwg-look.txt
              LICENSES/MPL-2.0-stylepak.txt
        DESTINATION ${CMAKE_INSTALL_DATADIR}/licenses/lgl-kwe-look)

# ---------------------------------------------------------------------------
# GTK3 preview helper — a separate binary so GTK never shares a process with Qt.
# The GTK C headers use deprecated API that -Werror would reject, so this
# target opts out of that one warning class only.
# ---------------------------------------------------------------------------
if(BUILD_GTK_PREVIEW)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GTK3 REQUIRED IMPORTED_TARGET gtk+-3.0)
    add_executable(lgl-kwe-look-gtk-preview helpers/gtk-preview/main.cpp)
    target_link_libraries(lgl-kwe-look-gtk-preview PRIVATE PkgConfig::GTK3)
    target_compile_options(lgl-kwe-look-gtk-preview PRIVATE -Wno-deprecated-declarations)
    install(TARGETS lgl-kwe-look-gtk-preview
            RUNTIME DESTINATION ${CMAKE_INSTALL_LIBEXECDIR}/lgl-kwe-look)
endif()

if(BUILD_TESTS)
    find_package(Qt6 REQUIRED COMPONENTS Test)
    enable_testing()
    add_subdirectory(tests)
endif()
