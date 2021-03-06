/*
    SPDX-FileCopyrightText: 2007 Sebastian Pipping <webmaster@hartwork.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include "katewildcardmatcher.h"
#include <QChar>
#include <QString>

namespace KateWildcardMatcher
{
bool exactMatch(const QString &candidate, const QString &wildcard, int candidatePosFromRight, int wildcardPosFromRight, bool caseSensitive = true)
{
    for (; wildcardPosFromRight >= 0; wildcardPosFromRight--) {
        const ushort ch = wildcard[wildcardPosFromRight].unicode();
        switch (ch) {
        case L'*':
            if (candidatePosFromRight == -1) {
                break;
            }

            if (wildcardPosFromRight == 0) {
                return true;
            }

            // Eat all we can and go back as far as we have to
            for (int j = -1; j <= candidatePosFromRight; j++) {
                if (exactMatch(candidate, wildcard, j, wildcardPosFromRight - 1)) {
                    return true;
                }
            }
            return false;

        case L'?':
            if (candidatePosFromRight == -1) {
                return false;
            }

            candidatePosFromRight--;
            break;

        default:
            if (candidatePosFromRight == -1) {
                return false;
            }

            const ushort candidateCh = candidate[candidatePosFromRight].unicode();
            const bool match = caseSensitive ? (candidateCh == ch) : (QChar::toLower(candidateCh) == QChar::toLower(ch));
            if (match) {
                candidatePosFromRight--;
            } else {
                return false;
            }
        }
    }
    return true;
}

bool exactMatch(const QString &candidate, const QString &wildcard, bool caseSensitive)
{
    return exactMatch(candidate, wildcard, candidate.length() - 1, wildcard.length() - 1, caseSensitive);
}

}
