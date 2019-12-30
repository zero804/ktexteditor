#ifndef TREE_ITEM_H
#define TREE_ITEM_H

#include <QVector>
#include <QtAlgorithms>

class TreeItem
{
public:
    explicit TreeItem(TreeItem *parentItem = nullptr)
        : m_parentItem(parentItem)
    {}

    virtual ~TreeItem()
    {
        qDeleteAll(m_childItems);
    }

    void clear()
    {
        qDeleteAll(m_childItems);
        m_childItems.clear();
    }

    void appendChild(TreeItem *item)
    {
        m_childItems.append(item);
    }

    TreeItem *child(int row)
    {
        if (row < 0 || row >= m_childItems.size()) {
            return nullptr;
        }

        return m_childItems.at(row);
    }

    int childCount() const
    {
        return m_childItems.count();
    }

    virtual int columnCount() const
    {
        return 1;
    }

    virtual QVariant data(int column, int role) const
    {
        Q_UNUSED(column)
        Q_UNUSED(role)

        return QVariant();
    }

    int row() const
    {
        if (m_parentItem) {
            return m_parentItem->m_childItems.indexOf(const_cast<TreeItem*>(this));
        }
        return 0;
    }

    TreeItem *parentItem()
    {
        return m_parentItem;
    }

private:
    QVector<TreeItem*> m_childItems;
    TreeItem *m_parentItem = nullptr;
};

#endif
