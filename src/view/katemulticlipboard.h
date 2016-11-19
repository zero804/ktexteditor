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

#ifndef KATEMULTICLIPBOARD_H
#define KATEMULTICLIPBOARD_H

#include <QClipboard>

class KateMultiCursor;
class QMimeData;

class KateMultiClipboard
{
public:
    KateMultiClipboard(KateMultiCursor* cursors);
    void copyToClipboard() const;
    void pasteFromClipboard(QClipboard::Mode clipboardMode);
    void pasteVector(const QVector<QString>& texts);
    void pasteText(const QString& text);
    QMimeData* createMimeData() const;

private:
    KateMultiCursor* m_cursors;
};

#endif // KATEMULTICLIPBOARD_H
