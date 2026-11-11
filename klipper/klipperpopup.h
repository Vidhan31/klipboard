/*
    SPDX-FileCopyrightText: 2004 Esben Mose Hansen <kde@mosehansen.dk>
    SPDX-FileCopyrightText: Andrew Stanley-Jones
    SPDX-FileCopyrightText: 2024 Fushan Wen <qydwhotmail@gmail.com>
    SPDX-License-Identifier: GPL-2.0-or-later
    Modified by Klipboard contributors
*/
#pragma once

#include <PlasmaQuick/PlasmaWindow>

#include "klipper_export.h"

class QWindow;

class HistoryModel;

class KLIPPER_EXPORT KlipperPopup : public PlasmaQuick::PlasmaWindow
{
    Q_OBJECT

public:
    explicit KlipperPopup();
    ~KlipperPopup() override;

    void show();

    void editCurrentClipboard();
    void showCurrentBarcode();

public Q_SLOTS:
    void hide();
    void resizePopup();
    void slotPasteAndHide(const QString &uuid = QString(), bool shift = false);

protected:
    void showEvent(QShowEvent *event) override;

private:
    void positionOnScreen();
    void onFocusWindowChanged(QWindow *focusWindow);

    /**
     * The "document" (clipboard history)
     */
    std::shared_ptr<HistoryModel> m_model;

    std::shared_ptr<QQmlEngine> m_engine;
};
