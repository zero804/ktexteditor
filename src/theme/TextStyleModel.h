/*
 * Copyright (C) 2019 Dominik Haumann <dhaumann@kde.org>
 *
 * SPDX-License-Identifier: MIT
 */
#ifndef THEME_DEFAULT_STYLES_MODEL_H
#define THEME_DEFAULT_STYLES_MODEL_H

#include <QAbstractItemModel>
#include <QJsonObject>

class TextStyleModelPrivate;

class TextStyleModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    TextStyleModel(QObject * parent = nullptr);
    ~TextStyleModel();

    void setTheme(QJsonObject &theme);
    QJsonObject& theme();

public: // reimplemented from QAbstractItemModel
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole) override;

private:
    TextStyleModelPrivate * const d;
};
#endif // THEME_DEFAULT_STYLES_MODEL_H
