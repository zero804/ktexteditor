/*
    This file is part of the KDE libraries
    SPDX-FileCopyrightText: 2010 Bernhard Beschow <bbeschow@cs.tu-berlin.de>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "regexpsearch_test.h"
#include "moc_regexpsearch_test.cpp"

#include <katedocument.h>
#include <kateglobal.h>
#include <kateregexpsearch.h>

#include <QRegularExpression>

#include <QtTestWidgets>

QTEST_MAIN(RegExpSearchTest)

#define testNewRow() (QTest::newRow(QString("line %1").arg(__LINE__).toLatin1().data()))

using namespace KTextEditor;

RegExpSearchTest::RegExpSearchTest()
    : QObject()
{
    KTextEditor::EditorPrivate::enableUnitTestMode();
}

RegExpSearchTest::~RegExpSearchTest()
{
}

void RegExpSearchTest::testReplaceEscapeSequences_data()
{
    QTest::addColumn<QString>("pattern");
    QTest::addColumn<QString>("expected");

    testNewRow() << "\\"
                 << "\\";
    testNewRow() << "\\0"
                 << "0";
    testNewRow() << "\\00"
                 << "00";
    testNewRow() << "\\000"
                 << "000";
    testNewRow() << "\\0000" << QString(QChar(0));
    testNewRow() << "\\0377" << QString(QChar(0377));
    testNewRow() << "\\0378"
                 << "0378";
    testNewRow() << "\\a"
                 << "\a";
    testNewRow() << "\\f"
                 << "\f";
    testNewRow() << "\\n"
                 << "\n";
    testNewRow() << "\\r"
                 << "\r";
    testNewRow() << "\\t"
                 << "\t";
    testNewRow() << "\\v"
                 << "\v";
    testNewRow() << "\\x"
                 << "x";
    testNewRow() << "\\x0"
                 << "x0";
    testNewRow() << "\\x00"
                 << "x00";
    testNewRow() << "\\x000"
                 << "x000";
    testNewRow() << "\\x0000" << QString(QChar(0x0000));
    testNewRow() << "\\x00000" << QString(QChar(0x0000) + '0');
    testNewRow() << "\\xaaaa" << QString(QChar(0xaaaa));
    testNewRow() << "\\xFFFF" << QString(QChar(0xFFFF));
    testNewRow() << "\\xFFFg"
                 << "xFFFg";
}

void RegExpSearchTest::testReplaceEscapeSequences()
{
    QFETCH(QString, pattern);
    QFETCH(QString, expected);

    const QString result1 = KateRegExpSearch::escapePlaintext(pattern);
    const QString result2 = KateRegExpSearch::buildReplacement(pattern, QStringList(), 0);

    QCOMPARE(result1, expected);
    QCOMPARE(result2, expected);
}

void RegExpSearchTest::testReplacementReferences_data()
{
    QTest::addColumn<QString>("pattern");
    QTest::addColumn<QString>("expected");
    QTest::addColumn<QStringList>("capturedTexts");

    testNewRow() << "\\0"
                 << "b" << (QStringList() << "b");
    testNewRow() << "\\00"
                 << "b0" << (QStringList() << "b");
    testNewRow() << "\\000"
                 << "b00" << (QStringList() << "b");
    testNewRow() << "\\0000" << QString(QChar(0)) << (QStringList() << "b");
    testNewRow() << "\\1"
                 << "1" << (QStringList() << "b");
    testNewRow() << "\\0"
                 << "b"
                 << (QStringList() << "b"
                                   << "c");
    testNewRow() << "\\1"
                 << "c"
                 << (QStringList() << "b"
                                   << "c");
}

void RegExpSearchTest::testReplacementReferences()
{
    QFETCH(QString, pattern);
    QFETCH(QString, expected);
    QFETCH(QStringList, capturedTexts);

    const QString result = KateRegExpSearch::buildReplacement(pattern, capturedTexts, 1);

    QCOMPARE(result, expected);
}

void RegExpSearchTest::testReplacementCaseConversion_data()
{
    QTest::addColumn<QString>("pattern");
    QTest::addColumn<QString>("expected");

    testNewRow() << "a\\Uaa"
                 << "aAA";
    testNewRow() << "a\\UAa"
                 << "aAA";
    testNewRow() << "a\\UaA"
                 << "aAA";

    testNewRow() << "a\\uaa"
                 << "aAa";
    testNewRow() << "a\\uAa"
                 << "aAa";
    testNewRow() << "a\\uaA"
                 << "aAA";

    testNewRow() << "A\\LAA"
                 << "Aaa";
    testNewRow() << "A\\LaA"
                 << "Aaa";
    testNewRow() << "A\\LAa"
                 << "Aaa";

    testNewRow() << "A\\lAA"
                 << "AaA";
    testNewRow() << "A\\lAa"
                 << "Aaa";
    testNewRow() << "A\\laA"
                 << "AaA";

    testNewRow() << "a\\EaA"
                 << "aaA";
    testNewRow() << "A\\EAa"
                 << "AAa";
}

void RegExpSearchTest::testReplacementCaseConversion()
{
    QFETCH(QString, pattern);
    QFETCH(QString, expected);

    const QString result = KateRegExpSearch::buildReplacement(pattern, QStringList(), 1);

    QCOMPARE(result, expected);
}

void RegExpSearchTest::testReplacementCounter_data()
{
    QTest::addColumn<QString>("pattern");
    QTest::addColumn<int>("counter");
    QTest::addColumn<QString>("expected");

    testNewRow() << "a\\#b" << 1 << "a1b";
    testNewRow() << "a\\#b" << 10 << "a10b";
    testNewRow() << "a\\#####b" << 1 << "a00001b";
}

void RegExpSearchTest::testReplacementCounter()
{
    QFETCH(QString, pattern);
    QFETCH(int, counter);
    QFETCH(QString, expected);

    const QString result = KateRegExpSearch::buildReplacement(pattern, QStringList(), counter);

    QCOMPARE(result, expected);
}

void RegExpSearchTest::testAnchoredRegexp_data()
{
    QTest::addColumn<QString>("pattern");
    QTest::addColumn<Range>("inputRange");
    QTest::addColumn<bool>("backwards");
    QTest::addColumn<Range>("expected");

    testNewRow() << "fe" << Range(0, 0, 0, 8) << false << Range(0, 0, 0, 2);
    testNewRow() << "fe" << Range(0, 0, 0, 8) << true << Range(0, 6, 0, 8);

    testNewRow() << "^fe" << Range(0, 0, 0, 8) << false << Range(0, 0, 0, 2);
    testNewRow() << "^fe" << Range(0, 0, 0, 1) << false << Range::invalid();
    testNewRow() << "^fe" << Range(0, 0, 0, 2) << false << Range(0, 0, 0, 2);
    testNewRow() << "^fe" << Range(0, 3, 0, 8) << false << Range::invalid(); // only match at line start
    testNewRow() << "^fe" << Range(0, 0, 0, 2) << true << Range(0, 0, 0, 2);
    testNewRow() << "^fe" << Range(0, 0, 0, 1) << true << Range::invalid();
    testNewRow() << "^fe" << Range(0, 0, 0, 2) << true << Range(0, 0, 0, 2);
    testNewRow() << "^fe" << Range(0, 3, 0, 8) << true << Range::invalid();

    testNewRow() << "fe$" << Range(0, 0, 0, 8) << false << Range(0, 6, 0, 8);
    testNewRow() << "fe$" << Range(0, 7, 0, 8) << false << Range::invalid();
    testNewRow() << "fe$" << Range(0, 6, 0, 8) << false << Range(0, 6, 0, 8);
    testNewRow() << "fe$" << Range(0, 0, 0, 5) << false << Range::invalid(); // only match at line end, fails
    testNewRow() << "fe$" << Range(0, 0, 0, 8) << true << Range(0, 6, 0, 8);
    testNewRow() << "fe$" << Range(0, 7, 0, 8) << true << Range::invalid();
    testNewRow() << "fe$" << Range(0, 6, 0, 8) << true << Range(0, 6, 0, 8);
    testNewRow() << "fe$" << Range(0, 0, 0, 5) << true << Range::invalid();

    testNewRow() << "^fe fe fe$" << Range(0, 0, 0, 8) << false << Range(0, 0, 0, 8);
    testNewRow() << "^fe fe fe$" << Range(0, 3, 0, 8) << false << Range::invalid();
    testNewRow() << "^fe fe fe$" << Range(0, 0, 0, 5) << false << Range::invalid();
    testNewRow() << "^fe fe fe$" << Range(0, 3, 0, 5) << false << Range::invalid();
    testNewRow() << "^fe fe fe$" << Range(0, 0, 0, 8) << true << Range(0, 0, 0, 8);
    testNewRow() << "^fe fe fe$" << Range(0, 3, 0, 8) << true << Range::invalid();
    testNewRow() << "^fe fe fe$" << Range(0, 0, 0, 5) << true << Range::invalid();
    testNewRow() << "^fe fe fe$" << Range(0, 3, 0, 5) << true << Range::invalid();

    testNewRow() << "^fe( fe)*$" << Range(0, 0, 0, 8) << false << Range(0, 0, 0, 8);
    testNewRow() << "^fe( fe)*" << Range(0, 0, 0, 8) << false << Range(0, 0, 0, 8);
    testNewRow() << "fe( fe)*$" << Range(0, 0, 0, 8) << false << Range(0, 0, 0, 8);
    testNewRow() << "fe( fe)*" << Range(0, 0, 0, 8) << false << Range(0, 0, 0, 8);
    testNewRow() << "^fe( fe)*$" << Range(0, 3, 0, 8) << false << Range::invalid();
    testNewRow() << "fe( fe)*$" << Range(0, 3, 0, 8) << false << Range(0, 3, 0, 8);
    testNewRow() << "^fe( fe)*$" << Range(0, 0, 0, 5) << false << Range::invalid();
    // fails because the whole line is fed to QRegularExpression, then matches
    // that end beyond the search range are rejected, see KateRegExpSearch::searchText()
    // testNewRow() << "^fe( fe)*"  << Range(0, 0, 0, 5) << false << Range(0, 0, 0, 5);

    testNewRow() << "^fe( fe)*$" << Range(0, 0, 0, 8) << true << Range(0, 0, 0, 8);
    testNewRow() << "^fe( fe)*" << Range(0, 0, 0, 8) << true << Range(0, 0, 0, 8);
    testNewRow() << "fe( fe)*$" << Range(0, 0, 0, 8) << true << Range(0, 0, 0, 8);
    testNewRow() << "fe( fe)*" << Range(0, 0, 0, 8) << true << Range(0, 0, 0, 8);
    testNewRow() << "^fe( fe)*$" << Range(0, 3, 0, 8) << true << Range::invalid();
    testNewRow() << "fe( fe)*$" << Range(0, 3, 0, 8) << true << Range(0, 3, 0, 8);
    testNewRow() << "^fe( fe)*$" << Range(0, 0, 0, 5) << true << Range::invalid();

    testNewRow() << "^fe|fe$" << Range(0, 0, 0, 5) << false << Range(0, 0, 0, 2);
    testNewRow() << "^fe|fe$" << Range(0, 3, 0, 8) << false << Range(0, 6, 0, 8);
    testNewRow() << "^fe|fe$" << Range(0, 0, 0, 5) << true << Range(0, 0, 0, 2);
    testNewRow() << "^fe|fe$" << Range(0, 3, 0, 8) << true << Range(0, 6, 0, 8);
}

void RegExpSearchTest::testAnchoredRegexp()
{
    QFETCH(QString, pattern);
    QFETCH(Range, inputRange);
    QFETCH(bool, backwards);
    QFETCH(Range, expected);

    KTextEditor::DocumentPrivate doc;
    doc.setText("fe fe fe");

    KateRegExpSearch searcher(&doc);

    static int i = 0;
    if (i == 34 || i == 36) {
        qDebug() << i;
    }
    i++;

    const Range result = searcher.search(pattern, inputRange, backwards, QRegularExpression::CaseInsensitiveOption)[0];

    QCOMPARE(result, expected);
}

void RegExpSearchTest::testSearchForward()
{
    KTextEditor::DocumentPrivate doc;
    doc.setText("  \\piinfercong");

    KateRegExpSearch searcher(&doc);
    const Range result = searcher.search("\\\\piinfer(\\w)", Range(0, 2, 0, 15), false)[0];

    QCOMPARE(result, Range(0, 2, 0, 11));
}

void RegExpSearchTest::testSearchBackwardInSelection()
{
    KTextEditor::DocumentPrivate doc;
    doc.setText("foobar foo bar foo bar foo");

    KateRegExpSearch searcher(&doc);
    const Range result = searcher.search("foo", Range(0, 0, 0, 15), true)[0];

    QCOMPARE(result, Range(0, 7, 0, 10));
}

void RegExpSearchTest::test()
{
    KTextEditor::DocumentPrivate doc;
    doc.setText("\\newcommand{\\piReductionOut}");

    KateRegExpSearch searcher(&doc);
    const QVector<Range> result = searcher.search("\\\\piReduction(\\S)", Range(0, 10, 0, 28), true);

    QCOMPARE(result.size(), 2);
    QCOMPARE(result[0], Range(0, 12, 0, 25));
    QCOMPARE(result[1], Range(0, 24, 0, 25));
    QCOMPARE(doc.text(result[0]), QString("\\piReductionO"));
    QCOMPARE(doc.text(result[1]), QString("O"));
}
