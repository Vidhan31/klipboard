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

Build and install into `$HOME/.local`:

```bash
cmake -B build -G Ninja -DCMAKE_INSTALL_PREFIX="$HOME/.local" -DBUILD_TESTING=ON
cmake --build build
ctest --test-dir build --output-on-failure -R "klipper-testHistoryCycler|klipper-testHistoryModel|klipper-testUtils"
cmake --install build
```

The build installs the binary to `~/.local/bin/klipper` with a `~/.local/bin/klipboard` symlink.

## Running

Start Klipboard directly in the background:

```bash
~/.local/bin/klipboard &
```

Or enable and start the systemd user service:

```bash
systemctl --user daemon-reload
systemctl --user enable --now klipboard.service
```

View service logs with journalctl:

```bash
journalctl --user -u klipboard.service -f
```
