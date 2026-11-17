# Klipboard (Standalone Klipper)

## Objective

This repository contains Klipboard, an independent application based on KDE Klipper v6.8.3 extracted from `plasma-workspace`. Klipboard has no affiliation with the KDE project.

The application runs without `plasmashell`, uses its own lifecycle, and preserves user data, configuration, shortcut bindings, and D-Bus interfaces.

## Environment

- Operating system: Fedora Linux 44.
- Desktop environment: KDE Plasma 6.8 on Wayland.
- Desktop panel: Quickshell (`quickshell.service`), not `plasmashell`.
- `plasmashell` is permanently masked (`plasma-plasmashell.service -> /dev/null`). Never enable or start it.
- Upstream reference source: `https://github.com/KDE/plasma-workspace` tag `v6.8.3`.

## Rules

### Do not do

- Do not enable, start, or depend on `plasmashell`.
- Do not run CopyQ or any other clipboard manager (including an installed release `klipboard` binary) at the same time. They contest the Wayland clipboard and primary selection.
- Do not install to `/usr` or run `cmake --install`. Do not install to the system; run directly from the build directory.
- Do not change the configuration filename (`klipperrc`) or schema.
- Do not modify or replace the SQLite database schema (`history3.sqlite` version 3 with WAL mode) or binary storage format in `data/`.
- Do not change the public D-Bus interface. The service must remain `org.kde.klipper` at object `/klipper`.
- Do not remove the `plasmashell` component name from `KActionCollection`. KWin stores user shortcuts under `[plasmashell]` in `~/.config/kglobalshortcutsrc`. Changing this name drops existing shortcut bindings.
- Do not remove the explicit plugin and QML search paths in `src/main.cpp`. Stale files in `~/.local/plugins` or `~/.local/qml` break Qt's loader if system paths are not set first.
- Do not relicense the project or add non-GPL project-level license files. The codebase contains GPL-2.0-or-later code from KDE.

### Do

- Keep the `klipper` CMake target as a shared library, installed as `libklipboard.so`. Both the `klipboard` binary and `libklipperplugin.so` must link the same shared library to share C++ singletons (`Klipper::self()`, `HistoryModel::self()`, `SystemClipboard::self()`).
- Keep `KStatusNotifierItem` active in `src/main.cpp`. Quickshell displays this in the system tray.
- Keep `KDBusService::Unique` in `src/main.cpp` so subsequent launches activate the existing instance.
- Keep `KSignalHandler` handling `SIGTERM`, `SIGINT`, and `SIGHUP` to run `saveSession()` and `saveClipboardHistory()` before exit.
- Keep license attribution intact. Upstream files modified for Klipboard must include a modification notice comment (`Modified by Klipboard contributors`) below their license header.
- Explicitly disconnect KWin EIS contexts via `org.kde.KWin.EIS.RemoteDesktop.disconnect(cookie)` when paste injection finishes or aborts in `ClipboardPaste`. Otherwise, unused EIS contexts accumulate in KWin until Klipboard exits.

## Repository layout

- `CMakeLists.txt`: Root build file. Finds Qt 6, KDE Frameworks 6, Plasma, and Wayland libraries.
- `c_ptr.h`: Smart pointer helper from upstream `plasma-workspace`.
- `config-X11.h.cmake`: CMake template for X11 and XCB build flags.
- `src/main.cpp`: Standalone application entry point. Handles application lifecycle, D-Bus single instance, system tray icon, and signal handling.
- `src/CMakeLists.txt`: Builds the `klipboard` executable.
- `klipper/`: Upstream Klipper source tree containing `libklipper` and declarative QML plugins.
- `klipper/declarative/`: The `org.klipboard.private.clipboard` QML plugin and popup components (`KlipperPopup.qml`, `ClipboardMenu.qml`, `BarcodePage.qml`).
- `packaging/autostart/org.kde.klipboard.desktop`: XDG autostart desktop entry template.

## Build and testing

Install missing development headers if building on a clean system:

```bash
sudo dnf install \
  kf6-kglobalaccel-devel \
  kf6-prison-devel \
  libplasma-devel \
  plasma-wayland-protocols-devel \
  qt6-qtbase-private-devel
```

Configure, build, and test:

```bash
cmake -B build -G Ninja -DBUILD_TESTING=ON
cmake --build build
ctest --test-dir build --output-on-failure -R "klipper-testHistoryCycler|klipper-testHistoryModel|klipper-testUtils|klipper-testClipboardPaste"
```

> **Note on test targets**: Always use the `-R` filter above. Avoid running unfiltered `ctest`:
> - `klippertest` is an upstream legacy test where `testBug465225` executes `KIO::copy()`, which spawns `/usr/libexec/kf6/kioworker` and hangs indefinitely without full Plasma session KIO worker infrastructure.
> - `klipper_v3migrationtest` expects the legacy `plasma6.3-update-clipboard-database-2-to-3` binary, which is disabled by default in standalone builds (`INSTALL_KCONF_UPDATE=OFF`).

## Running Klipboard

Stop CopyQ or any existing clipboard managers (including an installed release `klipboard` binary) if running:

```bash
copyq exit 2>/dev/null || true
pkill -x klipboard || true
```

Run directly from the build tree:

```bash
QML_IMPORT_PATH="$PWD/build/bin" ./build/bin/klipboard
```

`QML_IMPORT_PATH` points Qt to the build directory's QML module (`org.klipboard.private.clipboard`). Symlinks are not required.

## User data locations

- Configuration: `~/.config/klipperrc`
- History database: `~/.local/share/klipper/history3.sqlite`
- Binary clipboard storage: `~/.local/share/klipper/data/`
- Shortcut configuration: `~/.config/kglobalshortcutsrc` under `[plasmashell]`
