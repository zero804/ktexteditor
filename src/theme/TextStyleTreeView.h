/*
 * Copyright (C) 2019 Dominik Haumann <dhaumann@kde.org>
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef TEXT_STYLE_TREE_VIEW_H
#define TEXT_STYLE_TREE_VIEW_H

#include <QTreeView>

class TextStyleTreeView : public QTreeView
{
    Q_OBJECT

public:
    TextStyleTreeView(QWidget * parent = nullptr);
    ~TextStyleTreeView();

public Q_SLOTS:
    void updatePalette();
};
#endif // TEXT_STYLE_TREE_VIEW_H
