/*
    This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2010-2018 Dominik Haumann <dhaumann@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "movingrange_test.h"
#include "moc_movingrange_test.cpp"

#include <katedocument.h>
#include <kateglobal.h>
#include <kateview.h>
#include <ktexteditor/movingrange.h>
#include <ktexteditor/movingrangefeedback.h>

#include <QtTestWidgets>
#include <qtestmouse.h>

using namespace KTextEditor;

QTEST_MAIN(MovingRangeTest)

class RangeFeedback : public MovingRangeFeedback
{
public:
    RangeFeedback()
        : MovingRangeFeedback()
    {
        reset();
    }

    void rangeEmpty(MovingRange * /*range*/) override
    {
        m_rangeEmptyCalled = true;
    }

    void rangeInvalid(MovingRange * /*range*/) override
    {
        m_rangeInvalidCalled = true;
    }

    void mouseEnteredRange(MovingRange * /*range*/, View * /*view*/) override
    {
        m_mouseEnteredRangeCalled = true;
    }

    void mouseExitedRange(MovingRange * /*range*/, View * /*view*/) override
    {
        m_mouseExitedRangeCalled = true;
    }

    void caretEnteredRange(MovingRange * /*range*/, View * /*view*/) override
    {
        m_caretEnteredRangeCalled = true;
    }

    void caretExitedRange(MovingRange * /*range*/, View * /*view*/) override
    {
        m_caretExitedRangeCalled = true;
    }

    //
    // Test functions to reset feedback watcher
    //
public:
    void reset()
    {
        m_rangeEmptyCalled = false;
        m_rangeInvalidCalled = false;
        m_mouseEnteredRangeCalled = false;
        m_mouseExitedRangeCalled = false;
        m_caretEnteredRangeCalled = false;
        m_caretExitedRangeCalled = false;
    }

    void verifyReset()
    {
        QVERIFY(!m_rangeEmptyCalled);
        QVERIFY(!m_rangeInvalidCalled);
        QVERIFY(!m_mouseEnteredRangeCalled);
        QVERIFY(!m_mouseExitedRangeCalled);
        QVERIFY(!m_caretEnteredRangeCalled);
        QVERIFY(!m_caretExitedRangeCalled);
    }

    bool rangeEmptyCalled() const
    {
        return m_rangeEmptyCalled;
    }
    bool rangeInvalidCalled() const
    {
        return m_rangeInvalidCalled;
    }
    bool mouseEnteredRangeCalled() const
    {
        return m_mouseEnteredRangeCalled;
    }
    bool mouseExitedRangeCalled() const
    {
        return m_mouseExitedRangeCalled;
    }
    bool caretEnteredRangeCalled() const
    {
        return m_caretEnteredRangeCalled;
    }
    bool caretExitedRangeCalled() const
    {
        return m_caretExitedRangeCalled;
    }

private:
    bool m_rangeEmptyCalled;
    bool m_rangeInvalidCalled;
    bool m_mouseEnteredRangeCalled;
    bool m_mouseExitedRangeCalled;
    bool m_caretEnteredRangeCalled;
    bool m_caretExitedRangeCalled;
};

MovingRangeTest::MovingRangeTest()
    : QObject()
{
    KTextEditor::EditorPrivate::enableUnitTestMode();
}

MovingRangeTest::~MovingRangeTest()
{
}

// tests:
// - RangeFeedback::rangeEmpty
void MovingRangeTest::testFeedbackEmptyRange()
{
    KTextEditor::DocumentPrivate doc;
    // the range created below will span the 'x' characters
    QString text(
        "..xxxx\n"
        "xxxx..");
    doc.setText(text);

    // create range feedback
    RangeFeedback rf;

    // allow empty
    MovingRange *range = doc.newMovingRange(Range(Cursor(0, 2), Cursor(1, 4)), KTextEditor::MovingRange::DoNotExpand, KTextEditor::MovingRange::AllowEmpty);
    range->setFeedback(&rf);
    rf.verifyReset();

    // remove exact range
    doc.removeText(range->toRange());
    QVERIFY(rf.rangeEmptyCalled());
    QVERIFY(!rf.rangeInvalidCalled());
    QVERIFY(!rf.mouseEnteredRangeCalled());
    QVERIFY(!rf.mouseExitedRangeCalled());
    QVERIFY(!rf.caretEnteredRangeCalled());
    QVERIFY(!rf.caretExitedRangeCalled());

    // clear document: should call rangeInvalid
    rf.reset();
    rf.verifyReset();
    doc.clear();
    QVERIFY(rf.rangeInvalidCalled());
    QVERIFY(!rf.rangeEmptyCalled());
    QVERIFY(!rf.mouseEnteredRangeCalled());
    QVERIFY(!rf.mouseExitedRangeCalled());
    QVERIFY(!rf.caretEnteredRangeCalled());
    QVERIFY(!rf.caretExitedRangeCalled());

    // setText: should behave just like clear document: call rangeInvalid again
    doc.setText(text);
    range->setRange(Range(Cursor(0, 2), Cursor(1, 4)));
    rf.reset();
    rf.verifyReset();
    doc.setText("--yyyy\nyyyy--");
    QVERIFY(rf.rangeInvalidCalled());
    QVERIFY(!rf.rangeEmptyCalled());
    QVERIFY(!rf.mouseEnteredRangeCalled());
    QVERIFY(!rf.mouseExitedRangeCalled());
    QVERIFY(!rf.caretEnteredRangeCalled());
    QVERIFY(!rf.caretExitedRangeCalled());

    // now remove entire document range. In this case, emptyRange should be called
    // instead of rangeInvalid
    doc.setText(text);
    range->setRange(Range(Cursor(0, 2), Cursor(1, 4)));
    rf.reset();
    rf.verifyReset();
    doc.removeText(doc.documentRange());
    QVERIFY(rf.rangeEmptyCalled());
    QVERIFY(!rf.rangeInvalidCalled());
    QVERIFY(!rf.mouseEnteredRangeCalled());
    QVERIFY(!rf.mouseExitedRangeCalled());
    QVERIFY(!rf.caretEnteredRangeCalled());
    QVERIFY(!rf.caretExitedRangeCalled());
}

// tests:
// - RangeFeedback::rangeInvalid
void MovingRangeTest::testFeedbackInvalidRange()
{
    KTextEditor::DocumentPrivate doc;
    // the range created below will span the 'x' characters
    QString text(
        "..xxxx\n"
        "xxxx..");
    doc.setText(text);

    // create range feedback
    RangeFeedback rf;

    // allow empty
    MovingRange *range =
        doc.newMovingRange(Range(Cursor(0, 2), Cursor(1, 4)), KTextEditor::MovingRange::DoNotExpand, KTextEditor::MovingRange::InvalidateIfEmpty);
    range->setFeedback(&rf);
    rf.verifyReset();

    // remove exact range
    doc.removeText(range->toRange());
    QVERIFY(!rf.rangeEmptyCalled());
    QVERIFY(rf.rangeInvalidCalled());
    QVERIFY(!rf.mouseEnteredRangeCalled());
    QVERIFY(!rf.mouseExitedRangeCalled());
    QVERIFY(!rf.caretEnteredRangeCalled());
    QVERIFY(!rf.caretExitedRangeCalled());

    // clear document: should call rangeInvalid again
    doc.setText(text);
    range->setRange(Range(Cursor(0, 2), Cursor(1, 4)));
    rf.reset();
    rf.verifyReset();
    doc.clear();
    QVERIFY(rf.rangeInvalidCalled());
    QVERIFY(!rf.rangeEmptyCalled());
    QVERIFY(!rf.mouseEnteredRangeCalled());
    QVERIFY(!rf.mouseExitedRangeCalled());
    QVERIFY(!rf.caretEnteredRangeCalled());
    QVERIFY(!rf.caretExitedRangeCalled());

    // setText: should behave just like clear document: call rangeInvalid again
    doc.setText(text);
    range->setRange(Range(Cursor(0, 2), Cursor(1, 4)));
    rf.reset();
    rf.verifyReset();
    doc.setText("--yyyy\nyyyy--");
    QVERIFY(rf.rangeInvalidCalled());
    QVERIFY(!rf.rangeEmptyCalled());
    QVERIFY(!rf.mouseEnteredRangeCalled());
    QVERIFY(!rf.mouseExitedRangeCalled());
    QVERIFY(!rf.caretEnteredRangeCalled());
    QVERIFY(!rf.caretExitedRangeCalled());

    // now remove entire document range. Call rangeInvalid again
    doc.setText(text);
    range->setRange(Range(Cursor(0, 2), Cursor(1, 4)));
    rf.reset();
    rf.verifyReset();
    doc.removeText(doc.documentRange());
    QVERIFY(rf.rangeInvalidCalled());
    QVERIFY(!rf.rangeEmptyCalled());
    QVERIFY(!rf.mouseEnteredRangeCalled());
    QVERIFY(!rf.mouseExitedRangeCalled());
    QVERIFY(!rf.caretEnteredRangeCalled());
    QVERIFY(!rf.caretExitedRangeCalled());
}

// tests:
// - RangeFeedback::caretEnteredRange
// - RangeFeedback::caretExitedRange
void MovingRangeTest::testFeedbackCaret()
{
    KTextEditor::DocumentPrivate doc;
    // the range created below will span the 'x' characters
    QString text(
        "..xxxx\n"
        "xxxx..");
    doc.setText(text);

    KTextEditor::ViewPrivate *view = static_cast<KTextEditor::ViewPrivate *>(doc.createView(nullptr));

    // create range feedback
    RangeFeedback rf;

    // first test: with ExpandLeft | ExpandRight
    {
        view->setCursorPosition(Cursor(1, 6));

        MovingRange *range = doc.newMovingRange(Range(Cursor(0, 2), Cursor(1, 4)),
                                                KTextEditor::MovingRange::ExpandLeft | KTextEditor::MovingRange::ExpandRight,
                                                KTextEditor::MovingRange::InvalidateIfEmpty);
        rf.reset();
        range->setFeedback(&rf);
        rf.verifyReset();

        // left
        view->cursorLeft();
        QCOMPARE(view->cursorPosition(), Cursor(1, 5));
        QVERIFY(!rf.caretEnteredRangeCalled());
        QVERIFY(!rf.caretExitedRangeCalled());

        view->cursorLeft();
        QCOMPARE(view->cursorPosition(), Cursor(1, 4));
        QVERIFY(rf.caretEnteredRangeCalled()); // ExpandRight: include cursor already now
        QVERIFY(!rf.caretExitedRangeCalled());

        rf.reset();
        view->cursorLeft();
        QCOMPARE(view->cursorPosition(), Cursor(1, 3));
        QVERIFY(!rf.caretEnteredRangeCalled());
        QVERIFY(!rf.caretExitedRangeCalled());

        rf.reset();
        view->up();
        QCOMPARE(view->cursorPosition(), Cursor(0, 3));
        QVERIFY(!rf.caretEnteredRangeCalled());
        QVERIFY(!rf.caretExitedRangeCalled());

        rf.reset();
        view->cursorLeft();
        QCOMPARE(view->cursorPosition(), Cursor(0, 2));
        QVERIFY(!rf.caretEnteredRangeCalled());
        QVERIFY(!rf.caretExitedRangeCalled());

        rf.reset();
        view->cursorLeft();
        QCOMPARE(view->cursorPosition(), Cursor(0, 1)); // ExpandLeft: now we left it, not before
        QVERIFY(!rf.caretEnteredRangeCalled());
        QVERIFY(rf.caretExitedRangeCalled());

        delete range;
    }

    // second test: with DoNotExpand
    {
        view->setCursorPosition(Cursor(1, 6));

        MovingRange *range =
            doc.newMovingRange(Range(Cursor(0, 2), Cursor(1, 4)), KTextEditor::MovingRange::DoNotExpand, KTextEditor::MovingRange::InvalidateIfEmpty);
        rf.reset();
        range->setFeedback(&rf);
        rf.verifyReset();

        // left
        view->cursorLeft();
        QCOMPARE(view->cursorPosition(), Cursor(1, 5));
        QVERIFY(!rf.caretEnteredRangeCalled());
        QVERIFY(!rf.caretExitedRangeCalled());

        view->cursorLeft();
        QCOMPARE(view->cursorPosition(), Cursor(1, 4));
        QVERIFY(!rf.caretEnteredRangeCalled()); // DoNotExpand: does not include cursor
        QVERIFY(!rf.caretExitedRangeCalled());

        rf.reset();
        view->cursorLeft();
        QCOMPARE(view->cursorPosition(), Cursor(1, 3));
        QVERIFY(rf.caretEnteredRangeCalled());
        QVERIFY(!rf.caretExitedRangeCalled());

        rf.reset();
        view->up();
        QCOMPARE(view->cursorPosition(), Cursor(0, 3));
        QVERIFY(!rf.caretEnteredRangeCalled());
        QVERIFY(!rf.caretExitedRangeCalled());

        rf.reset();
        view->cursorLeft();
        QCOMPARE(view->cursorPosition(), Cursor(0, 2));
        QVERIFY(!rf.caretEnteredRangeCalled());
        QVERIFY(rf.caretExitedRangeCalled()); // DoNotExpand: that's why we leave already now

        rf.reset();
        view->cursorLeft();
        QCOMPARE(view->cursorPosition(), Cursor(0, 1));
        QVERIFY(!rf.caretEnteredRangeCalled());
        QVERIFY(!rf.caretExitedRangeCalled());

        delete range;
    }
}

// tests:
// - RangeFeedback::mouseEnteredRange
// - RangeFeedback::mouseExitedRange
void MovingRangeTest::testFeedbackMouse()
{
    KTextEditor::DocumentPrivate doc;
    // the range created below will span the 'x' characters
    QString text(
        "..xxxx\n"
        "xxxx..");
    doc.setText(text);

    KTextEditor::ViewPrivate *view = static_cast<KTextEditor::ViewPrivate *>(doc.createView(nullptr));
    view->setCursorPosition(Cursor(1, 6));
    view->show();
    view->resize(200, 100);

    // create range feedback
    RangeFeedback rf;
    QVERIFY(!rf.mouseEnteredRangeCalled());
    QVERIFY(!rf.mouseExitedRangeCalled());

    // allow empty
    MovingRange *range = doc.newMovingRange(Range(Cursor(0, 2), Cursor(1, 4)),
                                            KTextEditor::MovingRange::ExpandLeft | KTextEditor::MovingRange::ExpandRight,
                                            KTextEditor::MovingRange::InvalidateIfEmpty);
    range->setFeedback(&rf);
    rf.verifyReset();

    // left (nothing)
    QTest::mouseMove(view, view->cursorToCoordinate(Cursor(0, 0)) + QPoint(0, 5));
    QTest::qWait(200); // process mouse events. do not move mouse manually
    QVERIFY(!rf.mouseEnteredRangeCalled());
    QVERIFY(!rf.mouseExitedRangeCalled());

    // middle (enter)
    rf.reset();
    QTest::mouseMove(view, view->cursorToCoordinate(Cursor(0, 3)) + QPoint(0, 5));
    QTest::qWait(200); // process mouse events. do not move mouse manually
    QVERIFY(rf.mouseEnteredRangeCalled());
    QVERIFY(!rf.mouseExitedRangeCalled());

    // right (exit)
    rf.reset();
    QTest::mouseMove(view, view->cursorToCoordinate(Cursor(1, 6)) + QPoint(10, 5));
    QTest::qWait(200); // process mouse events. do not move mouse manually
    QVERIFY(!rf.mouseEnteredRangeCalled());
    QVERIFY(rf.mouseExitedRangeCalled());
}
