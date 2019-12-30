/*
 * Copyright (C) 2012-2019 Dominik Haumann <dhaumann@kde.org>
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef EDITOR_COLOR_MODEL_H
#define EDITOR_COLOR_MODEL_H

#include <QAbstractItemModel>

class EditorColorModelPrivate;

class EditorColorModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    explicit EditorColorModel(QObject *parent = nullptr);
    ~EditorColorModel();

public:
    void setTheme(QJsonObject &theme);
    QJsonObject& theme();

// public Q_SLOTS:
//     void selectDefaults();

Q_SIGNALS:
    void changed();

public: // reimplemented from QAbstractItemModel
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

protected:
//     bool edit(const QModelIndex &index, EditTrigger trigger, QEvent *event) override;
//     void drawBranches(QPainter *painter, const QRect &rect, const QModelIndex &index) const override;

private:
    EditorColorModelPrivate * const d;
};

#endif

