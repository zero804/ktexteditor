/*
 * Copyright (C) 2019 Dominik Haumann <dhaumann@kde.org>
 *
 * SPDX-License-Identifier: MIT
 */
#include "TextStyleTreeView.h"
#include "TextStyleModel.h"

#include <KLocalizedString>

#include <QStyledItemDelegate>
#include <QHeaderView>
#include <QJsonObject>
#include <QPainter>
#include <QDebug>

//BEGIN TextStyleDelegate
class TextStyleDelegate : public QStyledItemDelegate
{
public:
    TextStyleDelegate(TextStyleTreeView *treeView)
        : QStyledItemDelegate(treeView)
        , m_treeView(treeView)
    {}

    void paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const Q_DECL_OVERRIDE;

private:
    QColor getBrushForColorColumn(const QModelIndex &index, int column) const;
    TextStyleTreeView *m_treeView;
};

QColor TextStyleDelegate::getBrushForColorColumn(const QModelIndex &index, int column) const
{
    QModelIndex colorIndex = index.sibling(index.row(), column);
    QVariant displayData = colorIndex.model()->data(colorIndex, Qt::EditRole);
    return displayData.value<QColor>();
}

void TextStyleDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    if (index.column() == 0 && index.parent().internalPointer()) {
        QStyleOptionViewItem styleContextItem(option);

        QColor color = getBrushForColorColumn(index, 8); // selected background
        if (color != QColor(0, 0, 0, 0)) {
            styleContextItem.palette.setBrush(QPalette::Highlight, color);
        }

        color = getBrushForColorColumn(index, 6); // selected foreground
        if (color != QColor(0, 0, 0, 0)) {
            styleContextItem.palette.setBrush(QPalette::HighlightedText, color);
        }

        return QStyledItemDelegate::paint(painter, styleContextItem, index);
    }

    QStyledItemDelegate::paint(painter, option, index);

    if (index.column() < 5 || index.column() > 8) {
        return;
    }

    const QColor col = qvariant_cast<QColor>(index.model()->data(index, Qt::EditRole));
    QBrush brush = col;

    QStyleOptionButton opt;
    opt.rect = option.rect;
    opt.palette = m_treeView->palette();

    const bool set = col != QColor(0, 0, 0, 0);

    if (!set) {
        opt.text = i18nc("No text or background color set", "None set");
        brush = Qt::white;
    }

    m_treeView->style()->drawControl(QStyle::CE_PushButton, &opt, painter, m_treeView);

    if (set) {
        painter->fillRect(m_treeView->style()->subElementRect(QStyle::SE_PushButtonContents, &opt, m_treeView), brush);
    }
}
//END

static inline QJsonObject resolvePath(const QJsonObject &root, const QStringList &path)
{
    QJsonObject obj = root;
    for (const auto &s : path) {
        QJsonValue val = obj.value(s);
        if (!val.isObject()) {
            return QJsonObject();
        }
        obj = val.toObject();
    }
    return obj;
}

static inline QJsonValue readValue(const QJsonObject &root, const QStringList &path, const QString &key)
{
    const auto obj = resolvePath(root, path);
    return obj.value(key);
}

template <class T>
static inline QJsonValue readValue(const QJsonObject &root, const QStringList &path, const QString &key, const T& defaultValue)
{
    const QJsonValue val = readValue(root, path, key);
    if (val.isUndefined()) {
        return QJsonValue::fromVariant(defaultValue);
    }
    return val;
}

TextStyleTreeView::TextStyleTreeView(QWidget * parent)
    : QTreeView(parent)
{
    setItemDelegate(new TextStyleDelegate(this));
    setRootIsDecorated(false);
    setSelectionBehavior(SelectRows);
    header()->setMinimumSectionSize(header()->minimumSectionSize() / 2);
}

TextStyleTreeView::~TextStyleTreeView()
{
}

void TextStyleTreeView::updatePalette()
{
    auto m = qobject_cast<TextStyleModel*>(model());
    if (!m) {
        return;
    }

    QPalette pal = palette();
    QJsonObject &doc = m->theme();
    QColor col = QColor(readValue(doc, QStringList(QStringLiteral("editor-colors")), QStringLiteral("background-color"), QColor(0, 0, 0, 0)).toString());
    if (col != QColor(0, 0, 0, 0)) {
        pal.setColor(QPalette::Base, col);
    }
    col = QColor(readValue(doc, QStringList(QStringLiteral("editor-colors")), QStringLiteral("selection"), QColor(0, 0, 0, 0)).toString());
    if (col != QColor(0, 0, 0, 0)) {
        pal.setColor(QPalette::Highlight, col);
    }
    setPalette(pal);
}
