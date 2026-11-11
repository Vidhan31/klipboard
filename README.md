# Klipboard

Klipboard is an independent fork of KDE Klipper (part of `plasma-workspace`). Klipboard has no affiliation with KDE or the KDE project.

## Differences from upstream Klipper

Klipboard adapts Klipper for standalone desktop sessions without `plasmashell`:

- **Standalone process.** Klipboard manages its own application lifecycle and runs without `plasmashell`.
- **Direct text insertion.** Klipboard pastes into active Wayland windows by synthesizing key events through the XDG Remote Desktop portal and `libei`.
- **System tray integration.** Klipboard provides a system tray icon through the StatusNotifierItem protocol, which allows standalone panels like Quickshell to display the clipboard menu.
- **Data compatibility.** Klipboard uses the same configuration file (`~/.config/klipperrc`), database (`~/.local/share/klipper/history3.sqlite`), binary storage directory (`~/.local/share/klipper/data/`), and D-Bus interface (`org.kde.klipper` at `/klipper`). Existing clipboard history, settings, and shortcuts continue to work without migration.

## Requirements

Quit or disable any other active clipboard manager before starting Klipboard to avoid selection contests on Wayland.

Building Klipboard requires:

- C++23 compiler
- CMake 3.16 or newer
- Ninja
- Qt 6 (Core, Gui, Widgets, Qml, Quick, DBus, Sql, WaylandClient)
- KDE Frameworks 6 (Config, CoreAddons, DBusAddons, GlobalAccel, GuiAddons, I18n, IconThemes, KIO, Notifications, Prison, Service, StatusNotifierItem, WidgetsAddons, WindowSystem, XmlGui)
- Plasma Quick and Plasma Wayland protocols
- Wayland client libraries
- libei

## Build and installation

Configure for a standard `/usr` installation, then stage the install under a temporary root with `DESTDIR`:

```bash
cmake -B build -G Ninja -DCMAKE_INSTALL_PREFIX=/usr -DBUILD_TESTING=ON
cmake --build build
ctest --test-dir build --output-on-failure -R "klipper-testHistoryCycler|klipper-testHistoryModel|klipper-testUtils"
DESTDIR="$HOME/klipboard-stage" cmake --install build
```

This stages the installation at `$HOME/klipboard-stage/usr`. The binary path in a normal `/usr` installation is `/usr/bin/klipboard`.

### Running directly from the build directory (without installing)

You can run the built binary directly from the project directory for quick testing without installing to `/usr`:

```bash
QML_IMPORT_PATH="$PWD/build/bin" ./build/bin/klipboard
```

`QML_IMPORT_PATH` points Qt to the uninstalled QML module (`org.kde.plasma.private.clipboard`) located in `build/bin`. No binary symlinks are needed.

## Running

Start Klipboard in the background:

```bash
/usr/bin/klipboard &
```

### Autostart on login

Klipboard supports the XDG autostart specification. To install the autostart entry:

```bash
cmake -B build -DINSTALL_AUTOSTART=ON ...
DESTDIR="$HOME/klipboard-stage" cmake --install build
```

This stages `org.kde.klipboard.desktop` under `$HOME/klipboard-stage` at KDE's configured autostart directory. After packaging and installing to `/usr`, the desktop session can start Klipboard automatically on login.

View service logs with journalctl:

```bash
journalctl --user -u klipboard.service -f
```
