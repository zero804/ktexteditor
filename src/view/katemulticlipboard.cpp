/* This file is part of the KDE and the Kate project
 *
 *   Copyright (C) 2016 Sven Brauch <mail@svenbrauch.de>
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Library General Public
 *  License as published by the Free Software Foundation; either
 *  version 2 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Library General Public License for more details.
 *
 *  You should have received a copy of the GNU Library General Public License
 *  along with this library; see the file COPYING.LIB.  If not, write to
 *  the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 *  Boston, MA 02110-1301, USA.
 */

#include "katemulticlipboard.h"
#include "katemulticursor.h"
#include "katedocument.h"
#include "kateview.h"
#include "kateglobal.h"

#include <QClipboard>
#include <QApplication>
#include <QMimeData>

static QString TEXT_LIST_MIME_KEY = QStringLiteral("application/x-ktexteditor-text-list");

KateMultiClipboard::KateMultiClipboard(KateMultiCursor* cursors)
    : m_cursors(cursors)
{
}

void KateMultiClipboard::copyToClipboard() const
{
    qApp->clipboard()->setMimeData(createMimeData());
}

void KateMultiClipboard::pasteFromClipboard(QClipboard::Mode clipboardMode)
{
    KTextEditor::Document::EditingTransaction t(m_cursors->doc());

    auto mime = qApp->clipboard()->mimeData(clipboardMode);
    if (mime->hasFormat(TEXT_LIST_MIME_KEY)) {
        auto data = mime->data(TEXT_LIST_MIME_KEY);
        int32_t size;
        QDataStream s(data);
        s >> size;
        if (size == m_cursors->cursorsCount()) {
            QVector<QString> texts;
            // can paste as a vector
            for (int j = 0; j < size; j++) {
                if (s.atEnd()) {
                    qWarning() << "invalid data in clipboard (expected size" << size << "got" << j - 1 << ")";
                    return;
                }
                QString text;
                s >> text;
                texts << text;
            }
            pasteVector(texts);
            // ok, we're done
            return;
        }
    }

    if (mime->hasText()) {
        // otherwise, paste the same text everywhere
        // this happens if no vector data is available _or_ the length mismatches.
        pasteText(mime->text());
        // done
        return;
    }

    qDebug() << "no text or text list in clipboard, no action taken";
}

void KateMultiClipboard::pasteVector(const QVector<QString>& texts)
{
    if (texts.isEmpty()) {
        return;
    }

    m_cursors->view()->removeSelectedText();
    if (texts.size() == m_cursors->cursorsCount()) {
        auto cursors = m_cursors->cursors();
        std::sort(cursors.begin(), cursors.end(), [](const Cursor & a, const Cursor & b) {
            return a > b;
        });
        int j = 0;
        Q_FOREACH (const auto& cursor, cursors) {
            m_cursors->doc()->insertText(cursor, texts.at(j++));
        }
    } else {
        // join vector
        QString res;
        Q_FOREACH (const auto& t, texts) {
            if (!res.isEmpty()) {
                res.prepend(QLatin1Char('\n'));
            }
            res.append(t);
        }
        pasteText(res);
    }
}

void KateMultiClipboard::pasteText(const QString& text)
{
    m_cursors->view()->removeSelectedText();
    Q_FOREACH (auto c, m_cursors->movingCursors()) {
        m_cursors->doc()->insertText(c->toCursor(), text);
    }
}

QMimeData* KateMultiClipboard::createMimeData() const
{
    auto mime = new QMimeData;
    QByteArray data;
    QString textData;
    QVector<QString> textList;
    QDataStream s(&data, QIODevice::WriteOnly);

    auto sels = m_cursors->selections()->selections();
    s << (int32_t) sels.size();
    Q_FOREACH (const auto& sel, sels) {
        textData += textData.isEmpty() ? QString() : QLatin1String("\n");
        auto text = m_cursors->doc()->text(sel, false);
        s << text;
        textData += text;
        textList.append(text);
    }

    KTextEditor::EditorPrivate::self()->updateClipboardHistory(textList);

    mime->setData(TEXT_LIST_MIME_KEY, data);
    mime->setText(textData);
    return mime;
}
