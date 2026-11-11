# SPDX-FileCopyrightText: 2026 Klipboard contributors
# SPDX-License-Identifier: GPL-2.0-or-later

cmake_minimum_required(VERSION 3.16)

project(klipboard VERSION 6.8.2)
set(PROJECT_DEP_VERSION "6.8.0")
set(QT_MIN_VERSION "6.11.0")
set(KF6_MIN_VERSION "6.30.0")

option(WITH_X11 "Build with X11 clipboard fallback support" ON)
option(INSTALL_KCONF_UPDATE "Install legacy Plasma 6.3/6.8 kconf_update migration binaries" OFF)
option(INSTALL_AUTOSTART "Install XDG autostart desktop entry for end users" OFF)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(ECM ${KF6_MIN_VERSION} REQUIRED NO_MODULE)
set(CMAKE_MODULE_PATH ${ECM_MODULE_PATH})

include(KDEInstallDirs)
include(KDECMakeSettings)
include(KDECompilerSettings NO_POLICY_SCOPE)
include(ECMAddTests)
include(ECMQtDeclareLoggingCategory)
include(ECMGenerateExportHeader)
include(ECMDeprecationSettings)
include(ECMQmlModule)
include(FeatureSummary)

ecm_set_disabled_deprecation_versions(
    QT 6.8.0
    KF 6.10.0
)

find_package(Qt6 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS
    DBus
    Qml
    Quick
    Sql
    Widgets
    WaylandClient
)

if(BUILD_TESTING)
    find_package(Qt6 ${QT_MIN_VERSION} CONFIG REQUIRED COMPONENTS Test)
endif()

find_package(KF6 ${KF6_MIN_VERSION} REQUIRED COMPONENTS
    Config
    CoreAddons
    DBusAddons
    GlobalAccel
    GuiAddons
    I18n
    IconThemes
    KIO
    Notifications
    Prison
    Service
    StatusNotifierItem
    WidgetsAddons
    WindowSystem
    XmlGui
)

find_package(Plasma ${PROJECT_DEP_VERSION} REQUIRED)
find_package(PlasmaQuick ${PROJECT_DEP_VERSION} REQUIRED)
find_package(PlasmaWaylandProtocols 1.22.0 REQUIRED)
find_package(Wayland REQUIRED COMPONENTS Client)
find_package(ZLIB REQUIRED)

find_package(PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
    pkg_check_modules(LIBEI IMPORTED_TARGET libei)
endif()

if(TARGET PkgConfig::LIBEI)
    set(LIBEI_LIBRARY PkgConfig::LIBEI)
    set(HAVE_LIBEI 1)
else()
    find_library(LIBEI_LIBRARY NAMES ei libei.so.1 libei)
    if(LIBEI_LIBRARY)
        set(HAVE_LIBEI 1)
    else()
        set(HAVE_LIBEI 0)
    endif()
endif()

if(HAVE_LIBEI)
    message(STATUS "Found libei: ${LIBEI_LIBRARY}")
endif()

if(WITH_X11)
    find_package(X11 REQUIRED)
    find_package(XCB MODULE REQUIRED COMPONENTS AUX XCB)
    set(HAVE_X11 1)
    set(HAVE_XCURSOR 1)
    set(HAVE_XFIXES 1)
else()
    set(HAVE_X11 0)
    set(HAVE_XCURSOR 0)
    set(HAVE_XFIXES 0)
endif()

configure_file(config-X11.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-X11.h)
include_directories("${CMAKE_CURRENT_BINARY_DIR}")
add_definitions(-DQT_NO_URL_CAST_FROM_STRING)

add_subdirectory(klipper)
add_subdirectory(src)

if(INSTALL_AUTOSTART)
    install(FILES packaging/autostart/org.kde.klipper.desktop
            DESTINATION "${KDE_INSTALL_AUTOSTARTDIR}")
endif()

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)
