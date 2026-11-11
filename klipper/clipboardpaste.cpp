/*
    SPDX-FileCopyrightText: 2026 Klipboard contributors
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "clipboardpaste.h"

#include <chrono>
#include <thread>
#include <poll.h>
#include <unistd.h>
#include <linux/input-event-codes.h>

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusUnixFileDescriptor>
#include <QGuiApplication>
#include <QThread>
#include <QTimer>

#include <KConfigGroup>
#include <KSharedConfig>
#include <KWindowSystem>

#include "historyitem.h"
#include "historymodel.h"
#include "systemclipboard.h"
#include "klipper_debug.h"

#ifdef HAVE_LIBEI
#include "libei.h"
#endif

std::shared_ptr<ClipboardPaste> ClipboardPaste::self()
{
    static std::shared_ptr<ClipboardPaste> s_self = std::make_shared<ClipboardPaste>();
    return s_self;
}

ClipboardPaste::ClipboardPaste(QObject *parent)
    : QObject(parent)
{
}

ClipboardPaste::~ClipboardPaste() = default;

void ClipboardPaste::pasteToActiveWindow(const QString &uuid, bool shift)
{
    KConfigGroup group = KSharedConfig::openConfig(QStringLiteral("klipperrc"))->group(QStringLiteral("General"));
    const bool autoPaste = group.readEntry("AutoPasteOnSelect", true);
    if (!autoPaste) {
        qCDebug(KLIPPER_LOG) << "AutoPasteOnSelect is disabled in klipperrc";
        return;
    }

    const QString defaultKeySeq = group.readEntry("PasteKeySequence", QStringLiteral("Ctrl+V"));
    const QString terminalKeySeq = group.readEntry("TerminalPasteKeySequence", QStringLiteral("Ctrl+Shift+V"));
    const int delayMs = group.readEntry("PasteDelayMs", 100);

    if (!KWindowSystem::isPlatformWayland()) {
        qCDebug(KLIPPER_LOG) << "Clipboard paste via EIS is only applicable on Wayland";
        return;
    }

    const bool isShift = shift || ((QGuiApplication::keyboardModifiers() & Qt::ShiftModifier) != 0);

    bool isImage = false;
    auto model = HistoryModel::self();
    if (!uuid.isEmpty()) {
        const int row = model->indexOf(uuid);
        if (row >= 0) {
            auto item = model->index(row).data(HistoryModel::HistoryItemConstPtrRole).value<HistoryItemConstPtr>();
            if (item) {
                if (item->type() == HistoryItemType::Image || (item->allTypes() & HistoryItemType::Image)) {
                    isImage = true;
                }
            } else {
                const auto type = model->index(row).data(HistoryModel::TypeRole).value<HistoryItemType>();
                if (type == HistoryItemType::Image) {
                    isImage = true;
                }
            }
        }
    }
    if (!isImage) {
        if (auto first = model->first()) {
            if (first->type() == HistoryItemType::Image || (first->allTypes() & HistoryItemType::Image)) {
                isImage = true;
            }
        }
    }

    // Determine the key sequence to inject:
    // - Images must ALWAYS use Ctrl+V (applications reject images on plain-text paste shortcuts).
    // - Normal Enter / Click: sends PasteKeySequence (default: Ctrl+V for universal GUI/editor pasting).
    // - Shift + Enter / Shift + Click: sends TerminalPasteKeySequence (default: Ctrl+Shift+V for terminals).
    QString actualKeySeq;
    if (isImage) {
        actualKeySeq = QStringLiteral("Ctrl+V");
    } else if (isShift) {
        actualKeySeq = terminalKeySeq;
    } else {
        actualKeySeq = defaultKeySeq;
    }

    if (isImage && !uuid.isEmpty()) {
        // SystemClipboard loads image data from disk asynchronously via DatabaseRecordToMimeDataJob.
        // Wait for clipboardChanged() before injecting the paste shortcut, with a fallback timeout.
        const auto startTime = std::chrono::steady_clock::now();
        auto sysClip = SystemClipboard::self();
        auto timeoutTimer = new QTimer(this);
        timeoutTimer->setSingleShot(true);

        auto connection = std::make_shared<QMetaObject::Connection>();
        auto triggered = std::make_shared<std::atomic<bool>>(false);

        auto triggerPaste = [this, actualKeySeq, delayMs, startTime, triggered]() {
            if (triggered->exchange(true)) {
                return;
            }
            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startTime).count();
            int remainingDelay = std::max(0, delayMs - static_cast<int>(elapsed));
            // Add a small 30ms margin to allow Wayland data offer propagation to the target window
            executePaste(actualKeySeq, remainingDelay + 30);
        };

        *connection = connect(sysClip.get(), &SystemClipboard::clipboardChanged, this, [connection, timeoutTimer, triggerPaste]() {
            QObject::disconnect(*connection);
            timeoutTimer->stop();
            timeoutTimer->deleteLater();
            triggerPaste();
        });

        connect(timeoutTimer, &QTimer::timeout, this, [connection, timeoutTimer, triggerPaste]() {
            QObject::disconnect(*connection);
            timeoutTimer->deleteLater();
            qCDebug(KLIPPER_LOG) << "Fallback timeout reached waiting for image data; executing paste";
            triggerPaste();
        });

        timeoutTimer->start(350);
    } else {
        executePaste(actualKeySeq, delayMs);
    }
}

void ClipboardPaste::executePaste(const QString &keySeq, int delayMs)
{
#ifdef HAVE_LIBEI
    if (m_isPasting.exchange(true)) {
        qCDebug(KLIPPER_LOG) << "Paste operation already in progress, ignoring duplicate trigger";
        return;
    }

    auto *thread = QThread::create([this, keySeq, delayMs]() {
        if (delayMs > 0) {
            std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
        }

        auto msg = QDBusMessage::createMethodCall(QStringLiteral("org.kde.KWin"),
                                                  QStringLiteral("/org/kde/KWin/EIS/RemoteDesktop"),
                                                  QStringLiteral("org.kde.KWin.EIS.RemoteDesktop"),
                                                  QStringLiteral("connectToEIS"));
        msg << 1; // capabilities = 1 (keyboard)

        QDBusMessage reply = QDBusConnection::sessionBus().call(msg);
        if (reply.type() == QDBusMessage::ErrorMessage) {
            qCWarning(KLIPPER_LOG) << "KWin connectToEIS failed:" << reply.errorMessage();
            m_isPasting = false;
            return;
        }

        const QList<QVariant> args = reply.arguments();
        if (args.count() < 2) {
            qCWarning(KLIPPER_LOG) << "connectToEIS returned unexpected argument count:" << args.count();
            m_isPasting = false;
            return;
        }

        auto fdWrapper = qvariant_cast<QDBusUnixFileDescriptor>(args.at(0));
        if (!fdWrapper.isValid()) {
            qCWarning(KLIPPER_LOG) << "connectToEIS returned invalid file descriptor";
            m_isPasting = false;
            return;
        }

        int rawFd = fdWrapper.fileDescriptor();
        int fd = ::dup(rawFd);
        if (fd < 0) {
            qCWarning(KLIPPER_LOG) << "Failed to dup KWin EIS socket fd:" << strerror(errno);
            m_isPasting = false;
            return;
        }

        struct ei *ei = ei_new_sender(nullptr);
        if (!ei) {
            qCWarning(KLIPPER_LOG) << "Failed to create libei sender context";
            ::close(fd);
            m_isPasting = false;
            return;
        }

        ei_configure_name(ei, "klipper-paste");
        int ret = ei_setup_backend_fd(ei, fd);
        if (ret < 0) {
            qCWarning(KLIPPER_LOG) << "ei_setup_backend_fd failed:" << strerror(-ret);
            ei_unref(ei);
            ::close(fd);
            m_isPasting = false;
            return;
        }

        std::vector<uint32_t> modKeys;
        uint32_t pasteKey = KEY_V;

        if (keySeq.compare(QStringLiteral("Shift+Insert"), Qt::CaseInsensitive) == 0) {
            modKeys = {KEY_LEFTSHIFT};
            pasteKey = KEY_INSERT;
        } else if (keySeq.compare(QStringLiteral("Ctrl+V"), Qt::CaseInsensitive) == 0) {
            modKeys = {KEY_LEFTCTRL};
            pasteKey = KEY_V;
        } else {
            // Default: Ctrl+Shift+V
            modKeys = {KEY_LEFTCTRL, KEY_LEFTSHIFT};
            pasteKey = KEY_V;
        }

        bool done = false;
        struct ei_device *kbdDev = nullptr;
        struct ei_ping *ping = nullptr;
        const auto startTime = std::chrono::steady_clock::now();

        while (!done) {
            ei_dispatch(ei);
            struct ei_event *event;
            while ((event = ei_get_event(ei))) {
                const auto type = ei_event_get_type(event);
                if (type == EI_EVENT_SEAT_ADDED) {
                    struct ei_seat *seat = ei_event_get_seat(event);
                    ei_seat_bind_capabilities(seat, EI_DEVICE_CAP_KEYBOARD, NULL);
                } else if (type == EI_EVENT_DEVICE_ADDED) {
                    struct ei_device *dev = ei_event_get_device(event);
                    if (ei_device_has_capability(dev, EI_DEVICE_CAP_KEYBOARD)) {
                        kbdDev = dev;
                    }
                } else if (type == EI_EVENT_DEVICE_RESUMED) {
                    struct ei_device *dev = ei_event_get_device(event);
                    if (kbdDev && dev == kbdDev && !ping) {
                        qCDebug(KLIPPER_LOG) << "Injecting paste keystroke sequence (" << keySeq << ") via KWin EIS";
                        ei_device_start_emulating(dev, 1);
                        for (uint32_t mod : modKeys) {
                            ei_device_keyboard_key(dev, mod, true);
                        }
                        ei_device_frame(dev, ei_now(ei));

                        ei_device_keyboard_key(dev, pasteKey, true);
                        ei_device_frame(dev, ei_now(ei));
                        ei_device_keyboard_key(dev, pasteKey, false);
                        ei_device_frame(dev, ei_now(ei));

                        for (auto it = modKeys.rbegin(); it != modKeys.rend(); ++it) {
                            ei_device_keyboard_key(dev, *it, false);
                        }
                        ei_device_frame(dev, ei_now(ei));
                        ei_device_stop_emulating(dev);

                        ping = ei_new_ping(ei);
                        ei_ping(ping);
                    }
                } else if (type == EI_EVENT_PONG) {
                    qCDebug(KLIPPER_LOG) << "KWin EIS paste operation confirmed";
                    done = true;
                } else if (type == EI_EVENT_DISCONNECT) {
                    qCDebug(KLIPPER_LOG) << "KWin EIS disconnected";
                    done = true;
                }
                ei_event_unref(event);
            }

            if (!done) {
                auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - startTime).count();
                if (elapsed > 1000) {
                    qCWarning(KLIPPER_LOG) << "Timed out waiting for KWin EIS completion";
                    break;
                }
                struct pollfd pfd = { ei_get_fd(ei), POLLIN, 0 };
                poll(&pfd, 1, 50);
            }
        }

        if (ping) {
            ei_ping_unref(ping);
        }
        ei_unref(ei);
        ::close(fd);
        m_isPasting = false;
    });

    connect(thread, &QThread::finished, thread, &QObject::deleteLater);
    thread->start();
#endif
}
