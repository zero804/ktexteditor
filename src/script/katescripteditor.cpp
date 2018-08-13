/* This file is part of the KDE libraries.
 *
 * Copyright (C) 2017 Dominik Haumann <dhaumann@kde.org>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#include "katescripteditor.h"

#include "kateglobal.h"

#include <QApplication>
#include <QClipboard>
#include <QJSEngine>

using KTextEditor::EditorPrivate;

KateScriptEditor::KateScriptEditor(QJSEngine *engine, QObject *parent)
    : QObject(parent)
    , m_engine(engine)
{
}

QString KateScriptEditor::clipboardText() const
{
    return QApplication::clipboard()->text();
}

QStringList KateScriptEditor::clipboardHistory() const
{
    QStringList ret;
    for ( const auto& entry: KTextEditor::EditorPrivate::self()->clipboardHistory() ) {
        ret.append(entry.isEmpty() ? QString() : entry.first());
    }
    return ret;
}

void KateScriptEditor::setClipboardText(const QString &text)
{
    QApplication::clipboard()->setText(text, QClipboard::Clipboard);
}
