/*
    SPDX-FileCopyrightText: 2026 Klipboard contributors
    SPDX-License-Identifier: GPL-2.0-or-later
    Modified by Klipboard contributors
*/

#include <csignal>

#include <QApplication>
#include <QIcon>
#include <QLibraryInfo>
#include <QMenu>
#include <QQmlEngine>

#include <KAboutData>
#include <KActionCollection>
#include <KDBusService>
#include <KLocalizedString>
#include <KSignalHandler>
#include <KStandardAction>
#include <KStatusNotifierItem>
#include <PlasmaQuick/PlasmaQuick>

#include "../klipper/historymodel.h"
#include "../klipper/klipper.h"
#include "../klipper/klipperpopup.h"

using namespace Qt::StringLiterals;

namespace
{
void updateTrayToolTip(KStatusNotifierItem *sni, const std::shared_ptr<HistoryModel> &model)
{
    sni->setToolTipTitle(i18n("Clipboard Contents"));
    if (model->rowCount() == 0) {
        sni->setToolTipSubTitle(i18n("Clipboard is empty"));
    } else if (model->hasPassword()) {
        sni->setToolTipSubTitle(i18nc("@info", "The clipboard currently holds sensitive data"));
    } else {
        sni->setToolTipSubTitle(model->index(0).data(Qt::DisplayRole).toString());
    }
}
}

int main(int argc, char *argv[])
{
    // Ensure Qt finds system Wayland/XCB platform plugins and KDE QML modules,
    // even if stale/partial Qt trees exist in ~/.local/plugins or ~/.local/qml
    const QString systemPluginPath = QLibraryInfo::path(QLibraryInfo::PluginsPath);
    const QString systemQmlPath = QLibraryInfo::path(QLibraryInfo::QmlImportsPath);

    if (!systemPluginPath.isEmpty()) {
        QCoreApplication::addLibraryPath(systemPluginPath);
        if (qEnvironmentVariableIsEmpty("QT_PLUGIN_PATH")) {
            qputenv("QT_PLUGIN_PATH", systemPluginPath.toLocal8Bit());
        }
    }
    if (qEnvironmentVariableIsEmpty("QML2_IMPORT_PATH")) {
        const QString defaultQmlPath = systemQmlPath.isEmpty()
            ? QStringLiteral(KLIPPER_QML_IMPORT_DIR)
            : QStringLiteral(KLIPPER_QML_IMPORT_DIR ":") + systemQmlPath;
        qputenv("QML2_IMPORT_PATH", defaultQmlPath.toLocal8Bit());
    }

    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);
    QCoreApplication::setQuitLockEnabled(false);

    KLocalizedString::setApplicationDomain(QByteArrayLiteral("klipper"));

    // Keep the component name so KDBusService::Unique still arbitrates on org.kde.klipper.
    KAboutData aboutData(u"klipper"_s,
                         i18n("Klipboard"),
                         QStringLiteral(KLIPPER_VERSION_STRING),
                         i18n("Clipboard history utility"),
                         KAboutLicense::GPL_V2);
    aboutData.setLicense(KAboutLicense::GPL, KAboutLicense::OrLaterVersions);
    aboutData.setShortDescription(i18n("Standalone clipboard history manager"));
    aboutData.setOtherText(i18n("Klipboard is an independent project with no affiliation with the KDE project."));
    aboutData.setBugAddress(QByteArray());
    aboutData.setDesktopFileName(u"org.kde.klipboard"_s);
    KAboutData::setApplicationData(aboutData);
    app.setWindowIcon(QIcon::fromTheme(u"klipper"_s));

    // Acquire unique org.kde.klipper ownership before instantiating Klipper or SQLite WAL
    KDBusService dbusService(KDBusService::Unique);

    // Ensure patched standalone QML plugin takes precedence over system QML path
    if (const auto engine = PlasmaQuick::globalEngine()) {
        engine->addImportPath(QStringLiteral(KLIPPER_QML_IMPORT_DIR));
        if (!systemQmlPath.isEmpty()) {
            engine->addImportPath(systemQmlPath);
        }
    }

    auto klipper = Klipper::self();
    auto historyModel = HistoryModel::self();

    QObject::connect(&dbusService, &KDBusService::activateRequested, klipper.get(), [klipper](const QStringList &, const QString &) {
        klipper->slotPopupMenu();
    });

    // System tray item (StatusNotifierItem -> kded6 StatusNotifierWatcher -> quickshell)
    auto *sni = new KStatusNotifierItem(u"klipboard"_s, klipper.get());
    sni->setCategory(KStatusNotifierItem::SystemServices);
    sni->setStatus(KStatusNotifierItem::Active);
    sni->setIconByName(u"klipper-symbolic"_s);
    sni->setTitle(i18n("Clipboard"));
    sni->setStandardActionsEnabled(true);
    updateTrayToolTip(sni, historyModel);

    QObject::connect(historyModel.get(), &HistoryModel::changed, sni, [sni, historyModel] {
        updateTrayToolTip(sni, historyModel);
    });
    QObject::connect(historyModel.get(), &HistoryModel::hasPasswordChanged, sni, [sni, historyModel] {
        updateTrayToolTip(sni, historyModel);
    });
    QObject::connect(sni, &KStatusNotifierItem::activateRequested, klipper.get(), &Klipper::slotPopupMenu);

    auto *trayMenu = new QMenu;
    if (auto *collection = klipper->actionCollection()) {
        for (const auto &actionId : {u"clipboard_action"_s, u"edit_clipboard"_s, u"show-barcode"_s, u"repeat_action"_s, u"clear-history"_s}) {
            if (QAction *action = collection->action(actionId)) {
                trayMenu->addAction(action);
            }
        }
    }
    trayMenu->addSeparator();
    trayMenu->addAction(KStandardAction::preferences(klipper.get(), &Klipper::slotConfigure, trayMenu));
    sni->setContextMenu(trayMenu);

    // Graceful POSIX shutdown (SIGTERM / SIGINT / SIGHUP)
    auto *signals = KSignalHandler::self();
    signals->watchSignal(SIGTERM);
    signals->watchSignal(SIGINT);
    signals->watchSignal(SIGHUP);
    QObject::connect(signals, &KSignalHandler::signalReceived, &app, &QCoreApplication::quit);

    QObject::connect(&app, &QCoreApplication::aboutToQuit, klipper.get(), [k = klipper.get()] {
        k->saveSession();
        k->saveClipboardHistory();
    });

    const int ret = app.exec();
    klipper.reset();
    return ret;
}
