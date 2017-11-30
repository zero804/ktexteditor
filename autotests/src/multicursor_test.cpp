/* This file is part of the KDE libraries
   Copyright (C) 2016-2017 Sven Brauch <mail@svenbrauch.de>

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
#include <kateconfig.h>

#include <QtTestWidgets>

#include <KActionCollection>

using namespace KTextEditor;

QTEST_MAIN(MulticursorTest)

#define STOP view->show(); QEventLoop loop; loop.exec();

// Implementation of the multicursor test DSL

KTextEditor::Cursor parseCursor(const QString& s) {
    auto parts = s.split(',');
    Q_ASSERT(parts.size() == 2);
    bool ok1, ok2;
    auto cur = KTextEditor::Cursor(parts[0].toInt(&ok1), parts[1].toInt(&ok2));
    Q_ASSERT(ok1);
    Q_ASSERT(ok2);
    Q_ASSERT(cur.isValid());
    return cur;
}

struct MulticursorScriptRunner {
    enum Mode {
        Move,
        MouseSelect
    };

    MulticursorScriptRunner(QString script, QString states) {
        m_script = script;
        m_script.remove(" ");
        m_script.remove("\t");
        m_script.remove("\n");
        Q_ASSERT(script.count('|') == states.count('|'));
        states.remove(" ");
        states.remove("\t");
        states.remove("\n");
        m_states = states.split('|');
    }

    bool execNextPart(KateMultiCursor* s) {
        qDebug() << "exec" << part << pos;

        auto nextCursor = [this]() {
            auto start = pos;
            auto end1 = m_script.indexOf(')', pos);
            auto end2 = m_script.indexOf(';', pos);
            auto end = end1 == -1 && end2 != -1 ? end2 : end1 != -1 && end2 == -1 ? end1 : qMin(end1, end2); // sigh
            Q_ASSERT(end != -1);
            pos = end;
            return parseCursor(m_script.mid(start, end-start));
        };

        for ( ; pos < m_script.size(); ) {
            QChar c = m_script.at(pos);
            pos++;
            if ( mode == Move ) {
                switch ( c.unicode() ) {
                    case '|':
                        // next part
                        part++;
                        return true;
                    case '[':
                        select = true;
                        break;
                    case ']':
                        select = false;
                        break;
                    case 'L':
                        s->moveCursorsLeft(select, 1);
                        break;
                    case 'R':
                        s->moveCursorsRight(select, 1);
                        break;
                    case '>':
                        s->moveCursorsEndOfLine(select);
                        break;
                    case '<':
                        s->moveCursorsStartOfLine(select);
                        break;
                    case 'U':
                        s->moveCursorsUp(select, 1);
                        break;
                    case 'D':
                        s->moveCursorsDown(select, 1);
                        break;
                    case '+':
                        s->toggleSecondaryCursorAt(s->primaryCursor());
                        s->setSecondaryFrozen(true);
                        break;
                    case 'N':
                        s->moveCursorsWordNext(select);
                        break;
                    case 'P':
                        s->moveCursorsWordPrevious(select);
                        break;
                    case '$':
                        s->clearSecondaryCursors();
                        break;
                    case '#':
                        s->toggleSecondaryFrozen();
                        break;
                    case '(': {
                        KateMultiSelection::SelectionFlags flags = KateMultiSelection::UsePrimaryCursor;
                        if ( m_script.at(pos) == '+' ) {
                            pos++;
                            flags = KateMultiSelection::AddNewCursor;
                        }
                        KateMultiSelection::SelectionMode smode = KateMultiSelection::None;
                        auto modeChar = m_script.at(pos);
                        pos++;
                        if ( modeChar == 'C' ) {
                            smode = KateMultiSelection::Character;
                        }
                        else if ( modeChar == 'W' ) {
                            smode = KateMultiSelection::Word;
                        }
                        else if ( modeChar == 'L' ) {
                            smode = KateMultiSelection::Line;
                        }
                        else {
                            qWarning() << "invalid mode char" << modeChar << "in script" << m_script << "pos";
                        }
                        auto anchor = nextCursor();
                        mode = MouseSelect;
                        s->selections()->beginNewSelection(anchor, smode, flags);
                        break;
                    }
                    default:
                        qWarning() << "unhandled character" << c << "in script:" << m_script;
                }
            }
            else if ( mode == MouseSelect ) {
                switch ( c.unicode() ) {
                    case ')':
                        s->selections()->finishNewSelection();
                        mode = Move;
                        break;
                    default:
                        auto next = nextCursor();
                        s->selections()->updateNewSelection(next);
                        break;
                }
            }
        }
        part++;
        return false;
    }

    bool compareState(KateMultiCursor* c, const QString& state) {
        auto cursors = c->cursors();
        auto selections = c->selections()->selections();
        selections.erase(std::remove_if(selections.begin(), selections.end(), [](const KTextEditor::Range& r) {
            return r.isEmpty();
        }), selections.end());
        auto items = state.split(';');
        qDebug() << QString("[State %1]").arg(part) << "compare:" << state << cursors << selections;
        Q_FOREACH ( const auto& item, items ) {
            if ( item.contains("->") ) {
                auto parts = item.split("->");
                auto range = KTextEditor::Range(parseCursor(parts[0]), parseCursor(parts[1]));
                if ( ! selections.contains(range) ) {
                    qWarning() << "Selection" << range << "not found in" << selections;
                    return false;
                }
                selections.removeOne(range);
            }
            else {
                auto cursor = parseCursor(item);
                if ( ! cursors.contains(cursor) ) {
                    qWarning() << "Cursor" << cursor << "not found in" << cursors;
                    return false;
                }
                cursors.removeOne(cursor);
            }
        }
        if ( ! cursors.isEmpty() ) {
            qWarning() << cursors.size() << "cursors remain:" << cursors;
            return false;
        }
        if ( ! selections.isEmpty() ) {
            qWarning() << selections.size() << "selections remain:" << selections;
            return false;
        }
        return true;
    }

    QString currentState() const {
        return m_states.at(part-1);
    }

    QString m_script;
    QStringList m_states;
    int pos = 0;
    int part = 0;
    bool select = false;
    Mode mode = Move;
};

void MulticursorTest::testCursorMovement()
{
    QFETCH(QString, script);
    QFETCH(QString, states);

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
    // much easier to test like this, doesn't require the view to show
    // should have a separate test for dynwrap
    view->config()->setDynWordWrap(false);

    MulticursorScriptRunner runner(script, states);
    forever {
        auto cont = runner.execNextPart(view->cursors());
        QVERIFY(runner.compareState(view->cursors(), runner.currentState()));
        if ( ! cont ) {
            break;
        }
    }
}

void MulticursorTest::testCursorMovement_data()
{
    QTest::addColumn<QString>("script");
    QTest::addColumn<QString>("states");

    QTest::newRow("move_around") << "RRR|LL" << "0,3 | 0,1";
    QTest::newRow("move_word") << "N|P" << "0,5 | 0,0";
    QTest::newRow("select_word") << "[N|P]" << "0,5 ; 0,0->0,5 | 0,0";
    QTest::newRow("select_two_words") << "[NN|P|P]" << "0,8 ; 0,0->0,8 | 0,5 ; 0,0->0,5 | 0,0";

    QTest::newRow("move_up_down") << "RRRDRR|ULL" << "1,5 | 0,3";
    QTest::newRow("remember_x") << ">D|>|DDD|U" << "1,23 | 1,43 | 4,16 | 3,43";

    QTest::newRow("select_down") << "RRR[D]" << "1,3 ; 0,3->1,3";
    QTest::newRow("select_up") << "RRRD[U]" << "0,3 ; 0,3->1,3";

    QTest::newRow("reduce_selection_left") << "RRRRR[LLL]|[R]" << "0,2 ; 0,2->0,5 | 0,3 ; 0,3->0,5";
    QTest::newRow("reduce_selection_right") << "RRRRR[RRR]|[L]" << "0,8 ; 0,5->0,8 | 0,7 ; 0,5->0,7";
    QTest::newRow("umklapp") << ">LLL[P]|[N]|[P]" << "0,15 ; 0,15->0,20 | 0,23 ; 0,20->0,23 | 0,15 ; 0,15->0,20";

    QTest::newRow("two_cursors") << "+RRR|#RR" << "0,0 ; 0,3 | 0,2 ; 0,5";
    QTest::newRow("join_right") << "+RR#RR [RR] | [R]" << "0,4 ; 0,6 ; 0,2->0,4 ; 0,4->0,6 | 0,7 ; 0,2->0,7";
    QTest::newRow("join_left") << "+RR#RRR [LL] | [L]" << "0,1 ; 0,3 ; 0,1->0,3 ; 0,3->0,5 | 0,0 ; 0,0->0,5";

    QTest::newRow("multi_select_up") << "RRRD +D +D + [U] | [U]" << "0,3 ; 1,3 ; 2,3 ; 0,3->1,3 ; 1,3->2,3 ; 2,3->3,3 | 0,0 ; 0,0->3,3";
    QTest::newRow("multi_select_up2") << "RRRD +D +D [U] | [U]" << "0,3 ; 1,3 ; 2,3 ; 0,3->1,3 ; 1,3->2,3 ; 2,3->3,3 | 0,0 ; 0,0->3,3";
    QTest::newRow("multi_select_down_right") << "RRR +D +D [D] | [R]" << "1,3 ; 2,3 ; 3,3 ; 0,3->1,3 ; 1,3->2,3 ; 2,3->3,3 | 3,4 ; 0,3->3,4";
    QTest::newRow("multi_select_up_intersect") << "RRRD +DL +DL [U] | [U]" << "0,3; 0,3->3,1 | 0,0 ; 0,0->3,1";

    QTest::newRow("simple_mouse") << "RRR(C 0,5;0,7)" << "0,7; 0,5->0,7";
    QTest::newRow("simple_mouse_add") << "RRR(+C 0,5;0,7)" << "0,3; 0,7; 0,5->0,7";
    QTest::newRow("two_selections") << "RRR(+C 0,5;0,7) (+C 1,10;1,13)" << "0,3; 0,7; 1,13; 0,5->0,7; 1,10->1,13";
    QTest::newRow("multiselect_clear") << "RRR(+C 0,5;0,7) (C 1,10;1,13)" << "1,13; 1,10->1,13";
    QTest::newRow("multiselect_reverse_range") << "RRR(+C 0,5;0,7) (+C 1,13;1,10)" << "0,3; 0,7; 1,10; 0,5->0,7; 1,10->1,13";
    QTest::newRow("multiselect_stepwise") << "RRR(+C 0,5;0,6;0,6;0,7) (+C 1,10;1,11;1,13)" << "0,3; 0,7; 1,13; 0,5->0,7; 1,10->1,13";

    QTest::newRow("multiselect_overlap_undo") << "(C 0,5;0,7) (+C 0,9;0,8;0,7;0,2;0,8)" << "0,7; 0,8; 0,5->0,7; 0,8->0,9";
    QTest::newRow("multiselect_overlap_join") << "(C 0,5;0,7) (+C 0,9;0,8;0,7;0,2)" << "0,2; 0,2->0,9";
    QTest::newRow("multiselect_overlap_join_into") << "(C 0,5;0,7) (+C 0,9;0,8;0,7;0,6)" << "0,5; 0,5->0,9";
    QTest::newRow("multiselect_overlap_join_into2") << "(C 0,5;0,10) (+C 0,2;0,4;0,5;0,6)" << "0,10; 0,2->0,10";
    QTest::newRow("multiselect_overlap_join_into3") << "(C 0,10;0,5) (+C 0,2;0,4;0,5;0,6)" << "0,10; 0,2->0,10";
    QTest::newRow("multiselect_overlap_full") << "(C 0,5;0,10) (+C 0,9;0,8;0,7;0,2)" << "0,2; 0,2->0,10";
    QTest::newRow("multiselect_start_inside") << "(C 0,5;0,10) (+C 0,7;0,12)" << "0,12; 0,5->0,12";

    QTest::newRow("mselect_one_word") << "(W 0,0;0,1)" << "0,4; 0,0->0,4";
    QTest::newRow("mselect_word_mid_right") << "(W 0,3;0,4)" << "0,4; 0,0->0,4";
    QTest::newRow("mselect_word_mid_left") << "(W 0,3;0,2)" << "0,0; 0,0->0,4";
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

void MulticursorTest::testBlockModeView()
{
    QSKIP("Not implemented yet");
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

void MulticursorTest::testNavigationKeysView()
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
        auto expected2 = CL{{1, 4}, {2, 5}};
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
        QCOMPARE(view->cursors()->secondaryCursors(), CL{});
    }
    toggleMC->trigger();
    view->cursors()->setSecondaryFrozen(true);
    view->up();
    auto prev = view->allCursors();
    view->cursors()->setSecondaryFrozen(false);
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



