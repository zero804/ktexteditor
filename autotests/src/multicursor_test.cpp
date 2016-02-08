/* This file is part of the KDE libraries
   Copyright (C) 2016 Sven Brauch <mail@svenbrauch.de>

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public
   License as published by the Free Software Foundation; either
   version 2 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public License
   along with this library; see the file COPYING.LIB.  If not, write to
   the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.
*/

#include "multicursor_test.h"
#include "moc_multicursor_test.cpp"

#include <katedocument.h>
#include <kateview.h>

#include <QtTestWidgets>

#include <KActionCollection>

using namespace KTextEditor;

QTEST_MAIN(MulticursorTest)

#define STOP view->show(); QEventLoop loop; loop.exec();


void MulticursorTest::testMultiSelectionBasic()
{
    KTextEditor::DocumentPrivate doc;
    auto view = static_cast<KTextEditor::ViewPrivate*>(doc.createView(nullptr, nullptr));

    doc.setText("test test test test test\n"
                "\t test test test test  test\n"
                "     test test test test  test\n"
                "\n"
                "test test test");

    QFETCH(QVector<KTextEditor::Range>, existingSelections);
    QFETCH(KTextEditor::Range, toggle);
    QFETCH(QVector<KTextEditor::Range>, expectedSelections);

//     view->selections()->setSelection(existingSelections);
//     QCOMPARE(view->selections()->selections(), existingSelections);
//     view->selections()->toggleSelection(toggle);
//     qDebug() << "toggled:" << toggle;
//     qDebug() << "initial:" << existingSelections;
//     qDebug() << "expected:" << expectedSelections;
//     qDebug() << "existing selections:" << view->selections()->selections();
//     QCOMPARE(view->selections()->selections(), expectedSelections);
}

void MulticursorTest::testMultiSelectionBasic_data()
{
    using RL = QVector<KTextEditor::Range>;
    using R = KTextEditor::Range;
    QTest::addColumn<RL>("existingSelections");
    QTest::addColumn<R>("toggle");
    QTest::addColumn<RL>("expectedSelections");

    QTest::newRow("select") << RL{} << R{0, 3, 0, 5} << RL{{0, 3, 0, 5}};
    QTest::newRow("deselect") << RL{{0, 3, 0, 5}} << R{0, 3, 0, 5} << RL{};
    QTest::newRow("expand_left") << RL{{0, 4, 0, 8}} << R{0, 2, 0, 4} << RL{{0, 2, 0, 8}};
    QTest::newRow("expand_right") << RL{{0, 4, 0, 8}} << R{0, 8, 0, 11} << RL{{0, 4, 0, 11}};
}

char* toString(const QVector<KTextEditor::Cursor>& t) {
    char* ret = 0;
    if ( t.isEmpty() ) {
        ret = new char[3];
        strcpy(ret, "[]");
    }
    else {
        QString s;
        s = "[ ";
        Q_FOREACH ( const auto& c, t ) {
            s.append(QString::number(c.line())).append(QLatin1String(",")).append(QString::number(c.column()));
            s.append(QLatin1String(", "));
        }
        s = s.left(s.size() - 2);
        s.append(QLatin1String(" ]"));
        ret = new char[s.toUtf8().size()+1];
        strcpy(ret, s.toUtf8().data());
    }
    return ret;
}

void MulticursorTest::testBlockMode()
{
    KTextEditor::DocumentPrivate doc;
    const QString testText("0123456789ABCDEF\n"     // 0
                           "0123456789ABCDEF\n"     // 1
                           "0123456789ABCDEFG\n"    // 2
                           "0123456789ABCDEFGHI\n"  // 3
                           "0123456789ABCDEF\n"     // 4
                           "0123456789ABCDEF\n"     // 5
                           "0123456789ABCDEF\n");   // 6
    doc.setText(testText);
    auto view = static_cast<KTextEditor::ViewPrivate*>(doc.createView(nullptr, nullptr));
    view->show();
    QApplication::processEvents();
    view->setBlockSelection(true);

    view->setCursorPosition({0, 4});
    view->shiftDown();
    view->shiftDown();
    view->shiftDown();
    view->doc()->typeChars(view, "X");
    QCOMPARE(doc.text(), QString("0123X456789ABCDEF\n"     // 0
                                 "0123X456789ABCDEF\n"     // 1
                                 "0123X456789ABCDEFG\n"    // 2
                                 "0123X456789ABCDEFGHI\n"  // 3
                                 "0123456789ABCDEF\n"     // 4
                                 "0123456789ABCDEF\n"     // 5
                                 "0123456789ABCDEF\n"));   // 6))
    view->backspace();
    QCOMPARE(doc.text(), testText);

    view->doc()->typeChars(view, "X");
    view->cursorLeft();
    view->keyDelete();
    QCOMPARE(doc.text(), testText);

    view->toggleInsert();
    view->doc()->typeChars(view, "X");
    QCOMPARE(doc.text(), QString("0123X56789ABCDEF\n"     // 0
                                 "0123X56789ABCDEF\n"     // 1
                                 "0123X56789ABCDEFG\n"    // 2
                                 "0123X56789ABCDEFGHI\n"  // 3
                                 "0123456789ABCDEF\n"     // 4
                                 "0123456789ABCDEF\n"     // 5
                                 "0123456789ABCDEF\n"));  // 6
    view->backspace();
    QEXPECT_FAIL("", "Fixme: backspace in block overwrite mode", Continue);
    QCOMPARE(doc.text(), testText);
}

void MulticursorTest::testNavigationKeys()
{
    KTextEditor::DocumentPrivate doc;
    //                  0         1         2         3         4         5
    //                  012345678901234567890123456789012345678901234567890
    QString playground("This is a test document\n"                         // 0
                       "with multiple lines, some [ special chars ]\n"     // 1
                       "   some space indent and trailing spaces       \n" // 2
                       "   some space indent and trailing spaces       \n" // 3
                      "\tsome tab indent\n"                                // 4
                     "\t\tsome mixed indent\n"                             // 5
                       "     some more space indent\n");                   // 6
    doc.setText(playground);

    auto view = static_cast<KTextEditor::ViewPrivate*>(doc.createView(nullptr, nullptr));
    QVERIFY(view);
    // needed for some layout cache related testing
    view->show();

    auto right = view->actionCollection()->action("move_cursor_right");
    auto left = view->actionCollection()->action("move_cusor_left");
    auto toMatchingBracket = view->actionCollection()->action("to_matching_bracket");
    auto wordRight = view->actionCollection()->action("word_right");
    auto wordLeft = view->actionCollection()->action("word_left");
    auto end = view->actionCollection()->action("end_of_line");
    auto toggleMC = view->actionCollection()->action("add_virtual_cursor");
    auto freezeMC = view->actionCollection()->action("freeze_secondary_cursors");

    using C = KTextEditor::Cursor;
    using CL = QVector<KTextEditor::Cursor>;

    // BEGIN GENERAL
    view->setCursorPosition({1, 3});
    right->trigger();
    QCOMPARE(view->cursorPosition(), C(1, 4));

    toggleMC->trigger();
    view->setCursorPosition({2, 5});
    toggleMC->trigger();
    view->setCursorPosition({5, 9});
    {
        auto expected = CL{{5, 9}, {2, 5}, {1, 4}};
        QCOMPARE(view->cursors()->cursors(), expected);
        auto expected2 = CL{{2, 5}, {1, 4}};
        QCOMPARE(view->cursors()->secondaryCursors(), expected2);
    }
    QVERIFY(freezeMC->isChecked());
    QVERIFY(view->cursors()->secondaryFrozen());
    freezeMC->trigger();
    QVERIFY(!view->cursors()->secondaryFrozen());

    view->cursors()->clearSecondaryCursors();
    {
        auto expected = CL{view->cursorPosition()};
        auto expected2 = CL{{5, 9}};
        QCOMPARE(expected, expected2);
        QCOMPARE(view->allCursors(), expected);
    }
    // END GENERAL

    // Some basic left-right pressing without newline transitions
    view->setCursorPosition({2, 3});
    toggleMC->trigger();
    QVERIFY(view->cursors()->secondaryFrozen());
    right->trigger();
    right->trigger();
    {
        auto expected = CL{{2, 5}, {2, 3}};
        QCOMPARE(view->allCursors(), expected);
    }
    freezeMC->trigger();
    right->trigger();
    right->trigger();
    left->trigger();
    {
        auto expected = CL{{2, 6}, {2, 4}};
        QCOMPARE(view->allCursors(), expected);
    }

    // Use end key and navigate to a newline.
    end->trigger();
    {
        auto expected = CL{{2, 47}};
        QCOMPARE(view->allCursors(), expected);
        QCOMPARE(view->cursors()->secondaryCursors(), expected);
    }
    view->cursors()->setSecondaryFrozen(true);
    view->up();
    toggleMC->trigger();
    view->cursors()->setSecondaryFrozen(false);
    auto prev = view->allCursors();
    right->trigger();
    {
        auto expected = CL{{3, 0}, {2, 0}};
        QCOMPARE(view->allCursors(), expected);
    }
    // Go back.
    left->trigger();
    QCOMPARE(view->allCursors(), prev);

    QApplication::processEvents(); // needed here to update layout cache apparently?
    // Try going to the beginning of the line.
    view->home();
    {
        auto expected = CL{{2, 3}, {1, 0}};
        QCOMPARE(view->allCursors(), expected);
    }
    // toggle start of line / start of text
    view->down();
    view->home();
    {
        auto expected = CL{{3, 0}, {2, 3}};
        QCOMPARE(view->allCursors(), expected);
    }

    // word navigation
    view->cursors()->clearSecondaryCursors();
    view->cursors()->toggleSecondaryCursorAt({2, 8});
    view->setCursorPosition({4, 6});
    prev = view->allCursors();
    wordRight->trigger();
    {
        auto expected = CL{{4, 10}, {2, 14}};
        QCOMPARE(view->allCursors(), expected);
    }
    wordLeft->trigger();
    QCOMPARE(prev, view->allCursors());

    // bracket navigation
    view->cursors()->clearSecondaryCursors();
    view->setCursorPosition({3, 14});
    view->cursors()->toggleSecondaryCursorAt({1, 26});
    prev = view->allCursors();
    toMatchingBracket->trigger();
    QVERIFY(! view->cursors()->hasSecondaryCursors()); /// FIXME
}



