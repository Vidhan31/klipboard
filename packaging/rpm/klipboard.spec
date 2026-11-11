Name:           klipboard
Version:        6.8.2
Release:        1%{?dist}
Summary:        Standalone clipboard manager based on KDE Klipper
License:        GPL-2.0-or-later AND BSD-3-Clause
URL:            https://github.com/%{?gh_owner}%{!?gh_owner:klipboard}/klipper
Source0:        %{name}-%{version}.tar.gz
BuildRequires:  cmake, extra-cmake-modules, ninja-build, gcc-c++, pkgconf-pkg-config
BuildRequires:  qt6-qtbase-devel, qt6-qtbase-private-devel, qt6-qtdeclarative-devel, qt6-qtwayland-devel
BuildRequires:  kf6-kconfig-devel, kf6-kcoreaddons-devel, kf6-kdbusaddons-devel, kf6-kglobalaccel-devel
BuildRequires:  kf6-kguiaddons-devel, kf6-ki18n-devel, kf6-kiconthemes-devel, kf6-kio-devel
BuildRequires:  kf6-knotifications-devel, kf6-prison-devel, kf6-kservice-devel, kf6-kstatusnotifieritem-devel
BuildRequires:  kf6-kwidgetsaddons-devel, kf6-kwindowsystem-devel, kf6-kxmlgui-devel, libplasma-devel
BuildRequires:  plasma-wayland-protocols-devel, wayland-devel, libei-devel, xcb-util-devel, zlib-devel
%description
Klipboard runs independently of plasmashell while retaining Klipper's
configuration, history format, shortcuts, and D-Bus interface.
%prep
%autosetup -n %{name}-%{version}
%build
%cmake -G Ninja -DINSTALL_AUTOSTART=OFF -DINSTALL_KCONF_UPDATE=OFF -DWITH_X11=ON
%cmake_build
%install
%cmake_install
%files
%{_bindir}/klipboard
%{_libdir}/libklipboard.so.6*
%{_libdir}/libklipboard.so
%{_libdir}/qt6/qml/org/klipboard/private/clipboard/
%{_datadir}/applications/org.kde.klipboard.desktop
%{_datadir}/config.kcfg/klipper.kcfg
