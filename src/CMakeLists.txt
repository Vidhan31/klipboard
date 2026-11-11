# SPDX-FileCopyrightText: 2026 Klipboard contributors
# SPDX-License-Identifier: GPL-2.0-or-later

add_executable(klipboard-bin main.cpp)
set_target_properties(klipboard-bin PROPERTIES OUTPUT_NAME klipboard)

target_compile_definitions(klipboard-bin PRIVATE
    KLIPPER_VERSION_STRING="${PROJECT_VERSION}"
    KLIPPER_QML_IMPORT_DIR="${KDE_INSTALL_FULL_QMLDIR}"
)

target_link_libraries(klipboard-bin
    PRIVATE
        klipper
        Qt::Widgets
        Qt::Qml
        KF6::CoreAddons
        KF6::DBusAddons
        KF6::I18n
        KF6::StatusNotifierItem
        KF6::XmlGui
        Plasma::PlasmaQuick
)

install(TARGETS klipboard-bin ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
