/*
 * Copyright (C) 2012-2019 Dominik Haumann <dhaumann@kde.org>
 *
 * SPDX-License-Identifier: MIT
 */
#include "EditorColorModel.h"
#include "TreeItem_p.h"

#include <QColor>
#include <QJsonObject>
#include <QJsonValue>

#include <KLocalizedString>

static inline QColor readColor(const QJsonObject &root, const QString &key)
{
    return QColor(root.value(QStringLiteral("editor-colors")).toObject()[key].toString());
}

class ColorTreeItem : public TreeItem
{
public:
    // Constructor initializing all data.
    ColorTreeItem(const QString &translatedName, const QString &whatsThis, QJsonObject *root, const QString &key, TreeItem * parent)
        : TreeItem(parent)
        , m_translatedName(translatedName)
        , m_whatsThis(whatsThis)
        , m_root(root)
        , m_key(key)
    {
    }

    QVariant data(int column, int role) const override
    {
        if (role == Qt::DisplayRole) {
            switch (column) {
                case 0: return m_translatedName;
                case 1: return readColor(*m_root, m_key);
            }
        }

        if (role == Qt::WhatsThisRole) {
            return m_whatsThis;
        }

        if (role == Qt::ForegroundRole) {
            if (column == 1) {
                return readColor(*m_root, m_key);
            }
        }

        if (role == Qt::EditRole) {
            if (column == 1) {
                return readColor(*m_root, m_key);
            }
        }

        return QVariant();
    }

    bool setData(int column, const QVariant &value, int role = Qt::EditRole)
    {
        if (column == 1) {
//             writeValue(*m_root, QStringLiteral("editor-colors"), m_key, qvariant_cast<QColor>(value));
//             return true;
        }
        return false;
    }

    int columnCount() const override
    {
        return 2;
    }

private:
    QString m_translatedName;
    QString m_whatsThis;
    QJsonObject *m_root;
    QString m_key;
};

class ColorCategoryItem : public TreeItem
{
public:
    ColorCategoryItem(const QString &name, TreeItem *parent = nullptr)
        : TreeItem(parent)
        , m_name(name)
    {}

    QVariant data(int column, int role) const override
    {
        if (column == 0 && role == Qt::DisplayRole) {
            return m_name;
        }
        return QVariant();
    }

    int columnCount() const override
    {
        return 1;
    }

private:
    QString m_name;
};

class EditorColorModelPrivate
{
public:
    QJsonObject *theme = nullptr;
    TreeItem root;
};

EditorColorModel::EditorColorModel(QObject *parent)
    : QAbstractItemModel(parent)
    , d(new EditorColorModelPrivate)
{
}

EditorColorModel::~EditorColorModel()
{
    delete d;
}

void EditorColorModel::setTheme(QJsonObject &theme)
{
    beginResetModel();
    d->root.clear();

    d->theme = &theme;

    QStringList l = { QStringLiteral("text-styles") };
    ColorCategoryItem *c = new ColorCategoryItem(i18n("Editor Background Colors"), &d->root);
    d->root.appendChild(c);
    c->appendChild(new ColorTreeItem(i18n("Text Area"), i18n("<p>Sets the background color of the editing area.</p>"), d->theme, QStringLiteral("background-color"), c));
    c->appendChild(new ColorTreeItem(i18n("Selected Text"), i18n("<p>Sets the background color of the selection.</p><p>To set the text color for selected text, use the &quot;<b>Configure Highlighting</b>&quot; dialog.</p>"), d->theme, QStringLiteral("selection"), c));
    c->appendChild(new ColorTreeItem(i18n("Current Line"), i18n("<p>Sets the background color of the currently active line, which means the line where your cursor is positioned.</p>"), d->theme, QStringLiteral("current-line"), c));
    c->appendChild(new ColorTreeItem(i18n("Search Highlight"), i18n("Sets the background color of search results."), d->theme, QStringLiteral("search-highlight"), c));
    c->appendChild(new ColorTreeItem(i18n("Replace Highlight"), i18n("Sets the background color of replaced text."), d->theme, QStringLiteral("replace-highlight"), c));

    c = new ColorCategoryItem(i18n("Icon Border"), &d->root);
    d->root.appendChild(c);
    c->appendChild(new ColorTreeItem(i18n("Background Area"), i18n("Sets the background color of the icon border."), d->theme, QStringLiteral("icon-border"), c));
    c->appendChild(new ColorTreeItem(i18n("Line Numbers"), i18n("<p>This color will be used to draw the line numbers (if enabled).</p>"), d->theme, QStringLiteral("line-numbers"), c));
    c->appendChild(new ColorTreeItem(i18n("Current Line Number"), i18n("<p>This color will be used to draw the number of the current line (if enabled).</p>"), d->theme, QStringLiteral("current-line-number"), c));
    c->appendChild(new ColorTreeItem(i18n("Separator"), i18n("<p>This color will be used to draw the line between line numbers and the icon borders, if both are enabled.</p>"), d->theme, QStringLiteral("separator"), c));
    c->appendChild(new ColorTreeItem(i18n("Word Wrap Marker"), i18n("<p>Sets the color of Word Wrap-related markers:</p><dl><dt>Static Word Wrap</dt><dd>A vertical line which shows the column where text is going to be wrapped</dd><dt>Dynamic Word Wrap</dt><dd>An arrow shown to the left of visually-wrapped lines</dd></dl>"), d->theme, QStringLiteral("word-wrap-marker"), c));
    c->appendChild(new ColorTreeItem(i18n("Code Folding"), i18n("<p>Sets the color of the code folding bar.</p>"), d->theme, QStringLiteral("code-folding"), c));
    c->appendChild(new ColorTreeItem(i18n("Modified Lines"), i18n("<p>Sets the color of the line modification marker for modified lines.</p>"), d->theme, QStringLiteral("modified-lines"), c));
    c->appendChild(new ColorTreeItem(i18n("Saved Lines"), i18n("<p>Sets the color of the line modification marker for saved lines.</p>"), d->theme, QStringLiteral("saved-lines"), c));

    // text decorations
    c = new ColorCategoryItem(i18n("Text Decorations"), &d->root);
    d->root.appendChild(c);
    c->appendChild(new ColorTreeItem(i18n("Spelling Mistake Line"), i18n("<p>Sets the color of the line that is used to indicate spelling mistakes.</p>"), d->theme, QStringLiteral("spell-checking"), c));
    c->appendChild(new ColorTreeItem(i18n("Tab and Space Markers"), i18n("<p>Sets the color of the tabulator marks.</p>"), d->theme, QStringLiteral("tab-marker"), c));
    c->appendChild(new ColorTreeItem(i18n("Indentation Line"), i18n("<p>Sets the color of the vertical indentation lines.</p>"), d->theme, QStringLiteral("indentation-line"), c));
    c->appendChild(new ColorTreeItem(i18n("Bracket Highlight"), i18n("<p>Sets the bracket matching color. This means, if you place the cursor e.g. at a <b>(</b>, the matching <b>)</b> will be highlighted with this color.</p>"), d->theme, QStringLiteral("bracket-matching"), c));


    // marker colors
    c = new ColorCategoryItem(i18n("Marker Colors"), &d->root);
    d->root.appendChild(c);
    c->appendChild(new ColorTreeItem(i18n("Bookmark"), i18n("<p>Sets the background color of mark type.</p><p><b>Note</b>: The marker color is displayed lightly because of transparency.</p>"), d->theme, QStringLiteral("mark-bookmark"), c));
    c->appendChild(new ColorTreeItem(i18n("Active Breakpoint"), i18n("<p>Sets the background color of mark type.</p><p><b>Note</b>: The marker color is displayed lightly because of transparency.</p>"), d->theme, QStringLiteral("mark-breakpoint-active"), c));
    c->appendChild(new ColorTreeItem(i18n("Reached Breakpoint"), i18n("<p>Sets the background color of mark type.</p><p><b>Note</b>: The marker color is displayed lightly because of transparency.</p>"), d->theme, QStringLiteral("mark-breakpoint-reached"), c));
    c->appendChild(new ColorTreeItem(i18n("Disabled Breakpoint"), i18n("<p>Sets the background color of mark type.</p><p><b>Note</b>: The marker color is displayed lightly because of transparency.</p>"), d->theme, QStringLiteral("mark-breakpoint-disabled"), c));
    c->appendChild(new ColorTreeItem(i18n("Execution"), i18n("<p>Sets the background color of mark type.</p><p><b>Note</b>: The marker color is displayed lightly because of transparency.</p>"), d->theme, QStringLiteral("mark-execution"), c));
    c->appendChild(new ColorTreeItem(i18n("Warning"), i18n("<p>Sets the background color of mark type.</p><p><b>Note</b>: The marker color is displayed lightly because of transparency.</p>"), d->theme, QStringLiteral("mark-warning"), c));
    c->appendChild(new ColorTreeItem(i18n("Error"), i18n("<p>Sets the background color of mark type.</p><p><b>Note</b>: The marker color is displayed lightly because of transparency.</p>"), d->theme, QStringLiteral("mark-error"), c));

    // text templates
    c = new ColorCategoryItem(i18n("Text Templates & Snippets"), &d->root);
    d->root.appendChild(c);
    c->appendChild(new ColorTreeItem(i18n("Background"), i18n("<p>Background color for text template areas.</p>"), d->theme, QStringLiteral("template-background"), c));
    c->appendChild(new ColorTreeItem(i18n("Editable Placeholder"), i18n("<p>Background color for text template areas.</p>"), d->theme, QStringLiteral("template-placeholder"), c));
    c->appendChild(new ColorTreeItem(i18n("Focused Editable Placeholder"), i18n("<p>Background color for text template areas.</p>"), d->theme, QStringLiteral("template-focused-placeholder"), c));
    c->appendChild(new ColorTreeItem(i18n("Not Editable Placeholder"), i18n("<p>Background color for text template areas.</p>"), d->theme, QStringLiteral("template-read-only-placeholder"), c));

    endResetModel();
}

QJsonObject& EditorColorModel::theme()
{
    return *d->theme;
}


QModelIndex EditorColorModel::index(int row, int column, const QModelIndex &parent) const
{
    // top-level categories
    if (!parent.isValid()) {
        if (row < static_cast<int>(d->root.childCount())) {
            return createIndex(row, column, d->root.child(row));
        }
        return QModelIndex();
    }

    auto treeItem = static_cast<TreeItem*>(parent.internalPointer());
    if (treeItem && parent.row() < treeItem->childCount()) {
        return createIndex(row, column, treeItem->child(row));
    }

    return QModelIndex();
}

QModelIndex EditorColorModel::parent(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return QModelIndex();
    }

    const auto treeItem = static_cast<TreeItem*>(index.internalPointer());
    if (treeItem && treeItem->parentItem() && treeItem->parentItem() != &d->root) {
        return createIndex(treeItem->parentItem()->row(), 0, treeItem->parentItem());
    }

    return QModelIndex();
}

int EditorColorModel::rowCount(const QModelIndex &index) const
{
    // top-level categories
    if (!index.isValid()) {
        return d->root.childCount();
    }

    auto treeItem = static_cast<TreeItem*>(index.internalPointer());
    if (treeItem) {
        return treeItem->childCount();
    }

    return 0;
}

int EditorColorModel::columnCount(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return 2;
    }

    auto treeItem = static_cast<TreeItem*>(index.internalPointer());
    if (treeItem) {
        return treeItem->columnCount();
    }
    return 0;
}

QVariant EditorColorModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    auto treeItem = static_cast<TreeItem*>(index.internalPointer());
    if (treeItem) {
        return treeItem->data(index.column(), role);
    }

    return QVariant();
}

Qt::ItemFlags EditorColorModel::flags(const QModelIndex &index) const
{
    auto f = QAbstractItemModel::flags(index);
    if (index.isValid() && index.column() > 0) {
        auto treeItem = static_cast<TreeItem*>(index.internalPointer());
        auto colorItem = dynamic_cast<ColorTreeItem*>(treeItem);
        if (colorItem) {
            f = f | Qt::ItemIsEditable;
        }
    }
    return f;
}

bool EditorColorModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || index.column() == 0) {
        return false;
    }

    auto treeItem = static_cast<TreeItem*>(index.internalPointer());
    auto colorItem = dynamic_cast<ColorTreeItem*>(treeItem);
    if (colorItem) {
        const auto ok = colorItem->setData(index.column(), value, role);
        if (ok) {
            Q_EMIT dataChanged(index, index);
        }
    }

    return false;
}


#if 0
bool EditorColorModel::edit(const QModelIndex &index, EditTrigger trigger, QEvent *event)
{
    // accept edit only for color buttons in column 1 and reset in column 2
    if (!index.parent().isValid() || index.column() < 1) {
        return QTreeWidget::edit(index, trigger, event);
    }

    bool accept = false;
    if (event && event->type() == QEvent::KeyPress) {
        QKeyEvent *ke = static_cast<QKeyEvent *>(event);
        accept = (ke->key() == Qt::Key_Space); // allow Space to edit
    }

    switch (trigger) {
    case QAbstractItemView::DoubleClicked:
    case QAbstractItemView::SelectedClicked:
    case QAbstractItemView::EditKeyPressed: // = F2
        accept = true;
        break;
    default: break;
    }

    if (accept) {
        KateColorTreeItem *item = dynamic_cast<KateColorTreeItem *>(itemFromIndex(index));
        const QColor color = item->useDefaultColor() ? item->defaultColor() : item->color();

        if (index.column() == 1) {
            const QColor selectedColor = QColorDialog::getColor(color, this);

            if (selectedColor.isValid()) {
                item->setUseDefaultColor(false);
                item->setColor(selectedColor);
                viewport()->update();
                emit changed();
            }
        } else if (index.column() == 2 && !item->useDefaultColor()) {
            item->setUseDefaultColor(true);
            viewport()->update();
            emit changed();
        }

        return false;
    }
    return QTreeWidget::edit(index, trigger, event);
}

void EditorColorModel::drawBranches(QPainter *painter, const QRect &rect, const QModelIndex &index) const
{
    Q_UNUSED(painter)
    Q_UNUSED(rect)
    Q_UNUSED(index)
}

#endif

/*
void EditorColorModel::selectDefaults()
{
    bool somethingChanged = false;

    // use default colors for all selected items
    for (int a = 0; a < topLevelItemCount(); ++a) {
        QTreeWidgetItem *top = topLevelItem(a);
        for (int b = 0; b < top->childCount(); ++b) {
            KateColorTreeItem *it = dynamic_cast<KateColorTreeItem *>(top->child(b));
            Q_ASSERT(it);
            if (!it->useDefaultColor()) {
                it->setUseDefaultColor(true);
                somethingChanged = true;
            }
        }
    }

    if (somethingChanged) {
        viewport()->update();
        emit changed();
    }
}

void EditorColorModel::addColorItem(const KateColorItem &colorItem)
{
    QTreeWidgetItem *categoryItem = 0;
    for (int i = 0; i < topLevelItemCount(); ++i) {
        if (topLevelItem(i)->text(0) == colorItem.category) {
            categoryItem = topLevelItem(i);
            break;
        }
    }

    if (!categoryItem) {
        categoryItem = new QTreeWidgetItem();
        categoryItem->setText(0, colorItem.category);
        addTopLevelItem(categoryItem);
        expandItem(categoryItem);
    }

    new KateColorTreeItem(colorItem, categoryItem);

    resizeColumnToContents(0);
}

void EditorColorModel::addColorItems(const QVector<KateColorItem> &colorItems)
{
    foreach (const KateColorItem &item, colorItems) {
        addColorItem(item);
    }
}

QVector<KateColorItem> EditorColorModel::colorItems() const
{
    QVector<KateColorItem> items;
    for (int a = 0; a < topLevelItemCount(); ++a) {
        QTreeWidgetItem *top = topLevelItem(a);
        for (int b = 0; b < top->childCount(); ++b) {
            KateColorTreeItem *item = dynamic_cast<KateColorTreeItem *>(top->child(b));
            Q_ASSERT(item);
            items.append(item->colorItem());
        }
    }
    return items;
}

QColor EditorColorModel::findColor(const QString &key) const
{
    for (int a = 0; a < topLevelItemCount(); ++a) {
        QTreeWidgetItem *top = topLevelItem(a);
        for (int b = 0; b < top->childCount(); ++b) {
            KateColorTreeItem *item = dynamic_cast<KateColorTreeItem *>(top->child(b));
            if (item->key() == key) {
                if (item->useDefaultColor()) {
                    return item->defaultColor();
                } else {
                    return item->color();
                }
            }
        }
    }
    return QColor();
}
*/

