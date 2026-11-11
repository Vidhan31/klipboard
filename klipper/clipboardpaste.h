/*
    SPDX-FileCopyrightText: 2026 Klipboard contributors
    SPDX-License-Identifier: GPL-2.0-or-later
*/
#pragma once

#include <atomic>
#include <memory>

#include <QObject>

#include "klipper_export.h"

class KLIPPER_EXPORT ClipboardPaste : public QObject
{
    Q_OBJECT

public:
    static std::shared_ptr<ClipboardPaste> self();
    explicit ClipboardPaste(QObject *parent = nullptr);
    ~ClipboardPaste() override;

    /**
     * Pastes the current clipboard content into the previously active window.
     * Configuration is read from klipperrc [General]:
     * - AutoPasteOnSelect: bool (default: true)
     * - PasteKeySequence: string (default: "Ctrl+V" for standard Click / Enter)
     * - TerminalPasteKeySequence: string (default: "Ctrl+Shift+V" for Shift + Click / Shift + Enter)
     * - PasteDelayMs: int (default: 100)
     *
     * Note: If the selected item is an image, "Ctrl+V" is always used because GUI
     * applications treat Ctrl+Shift+V as plain text only and reject image data.
     */
    void pasteToActiveWindow(const QString &uuid = QString(), bool shift = false);

private:
    void executePaste(const QString &keySeq, int delayMs);

    std::atomic<bool> m_isPasting{false};
};
