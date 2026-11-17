/*
    SPDX-FileCopyrightText: 2026 Klipboard contributors
    SPDX-License-Identifier: GPL-2.0-or-later
*/

#include "../clipboardpaste.h"
#include "../klipper.h"
#include "../klippersettings.h"

#include <QSignalSpy>
#include <QTest>
#include <KConfigGroup>
#include <KSharedConfig>

class ClipboardPasteTest : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void initTestCase();
    void testSingleton();
    void testDisabledAutoPaste();
    void testPasteTrigger();
    void testPasteWithUuid();
    void testPasteShortcutConfig();
};

void ClipboardPasteTest::initTestCase()
{
    qputenv("QT_QPA_PLATFORM", "offscreen");
}

void ClipboardPasteTest::testSingleton()
{
    auto paste1 = ClipboardPaste::self();
    auto paste2 = ClipboardPaste::self();
    QVERIFY(paste1 != nullptr);
    QCOMPARE(paste1.get(), paste2.get());
}

void ClipboardPasteTest::testDisabledAutoPaste()
{
    KConfigGroup group = KSharedConfig::openConfig(QStringLiteral("klipperrc"))->group(QStringLiteral("General"));
    group.writeEntry("AutoPasteOnSelect", false);
    group.sync();

    // Calling pasteToActiveWindow should safely return without performing paste
    ClipboardPaste::self()->pasteToActiveWindow();

    // Restore setting
    group.writeEntry("AutoPasteOnSelect", true);
    group.sync();
}

void ClipboardPasteTest::testPasteTrigger()
{
    // Calling pasteToActiveWindow should execute safely without crashing or hanging
    ClipboardPaste::self()->pasteToActiveWindow();
    // Wait briefly for worker thread to complete
    QTest::qWait(250);
}

void ClipboardPasteTest::testPasteWithUuid()
{
    // Calling pasteToActiveWindow with arbitrary UUID or empty string should handle gracefully
    ClipboardPaste::self()->pasteToActiveWindow(QStringLiteral("non-existent-uuid"), false);
    QTest::qWait(250);

    ClipboardPaste::self()->pasteToActiveWindow(QStringLiteral("non-existent-uuid"), true);
    QTest::qWait(250);

    ClipboardPaste::self()->pasteToActiveWindow(QString(), false);
    QTest::qWait(250);

    ClipboardPaste::self()->pasteToActiveWindow(QString(), true);
    QTest::qWait(250);
}

void ClipboardPasteTest::testPasteShortcutConfig()
{
    auto config = KSharedConfig::openConfig(QStringLiteral("klipperrc"));
    KConfigGroup group = config->group(QStringLiteral("General"));

    // Test configured values
    group.writeEntry("PasteKeySequence", QStringLiteral("Ctrl+Shift+V"));
    group.writeEntry("TerminalPasteKeySequence", QStringLiteral("Ctrl+V"));
    group.sync();

    config->reparseConfiguration();
    QCOMPARE(group.readEntry("PasteKeySequence", QStringLiteral("Ctrl+V")), QStringLiteral("Ctrl+Shift+V"));
    QCOMPARE(group.readEntry("TerminalPasteKeySequence", QStringLiteral("Ctrl+Shift+V")), QStringLiteral("Ctrl+V"));

    ClipboardPaste::self()->pasteToActiveWindow(QString(), false);
    QTest::qWait(250);
    ClipboardPaste::self()->pasteToActiveWindow(QString(), true);
    QTest::qWait(250);

    // Test setting both to same shortcut
    group.writeEntry("PasteKeySequence", QStringLiteral("Ctrl+Shift+V"));
    group.writeEntry("TerminalPasteKeySequence", QStringLiteral("Ctrl+Shift+V"));
    group.sync();
    config->reparseConfiguration();

    ClipboardPaste::self()->pasteToActiveWindow(QString(), false);
    QTest::qWait(250);
    ClipboardPaste::self()->pasteToActiveWindow(QString(), true);
    QTest::qWait(250);

    // Restore defaults
    group.writeEntry("PasteKeySequence", QStringLiteral("Ctrl+V"));
    group.writeEntry("TerminalPasteKeySequence", QStringLiteral("Ctrl+Shift+V"));
    group.sync();
    config->reparseConfiguration();
}

QTEST_MAIN(ClipboardPasteTest)

#include "clipboardpastetest.moc"
