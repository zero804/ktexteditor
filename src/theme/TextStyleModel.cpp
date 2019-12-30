/*
 * Copyright (C) 2019 Dominik Haumann <dhaumann@kde.org>
 *
 * SPDX-License-Identifier: MIT
 */
#include "TextStyleModel.h"
#include "TreeItem_p.h"

#include <KSyntaxHighlighting/Theme>
#include <KLocalizedString>

#include <QIcon>
#include <QFont>
#include <QJsonObject>
#include <QDebug>

/**
 * Convert QJsonValue @p val into a color, if possible. Valid colors are only
 * in hex format: #rrggbb. On error, returns 0x00000000.
 */
static inline QRgb readColor(const QJsonValue &val, const QRgb &unsetColor)
{
    if (!val.isString()) {
        return unsetColor;
    }
    const QString str = val.toString();
    if (str.isEmpty() || str[0] != QLatin1Char('#')) {
        return unsetColor;
    }
    const QColor color(str);
    return color.isValid() ? color.rgb() : unsetColor;
}

static inline bool readBool(const QJsonValue &val)
{
    return val.isBool() ? val.toBool() : false;
}

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
static inline QVariant readValue(const QJsonObject &root, const QStringList &path, const QString &key, const T& defaultValue)
{
    const QJsonValue val = readValue(root, path, key);
    if (val.isUndefined()) {
        return defaultValue;
    }
    return qvariant_cast<T>(val);
}

static inline QColor readColor(const QJsonObject &root, const QStringList &path, const QString &key, const QColor& defaultValue)
{
    const QJsonValue val = readValue(root, path, key);
    if (val.isUndefined()) {
        return defaultValue;
    }
    return readColor(val, defaultValue.rgb());
}


static inline void writeValue(QJsonObject &root, QStringList path, const QString &key, const QVariant &value)
{
    if (path.isEmpty()) {
        root[key] = QJsonValue::fromVariant(value);
        qDebug() << root;
        return;
    }

    QJsonObject obj = root[path.front()].toObject();
    QStringList subPath = path;
    subPath.pop_front();
    writeValue(obj, subPath, key, value);
    root[path.front()] = obj;
}

enum class TextStyleRole
{
    TranslatedName = Qt::DisplayRole,
    TextStyle = Qt::UserRole + 1,
    IsDefaultTextStyle,
    TextColor,
    SelectedTextColor,
    BackgroundColor,
    SelectedBackgroundColor,
    Bold,
    Italic,
    Underline,
    StrikeThrough
};

class TextStyleItem : public TreeItem
{
public:
    // Constructor initializing all data.
    TextStyleItem(const QString &translatedName, KSyntaxHighlighting::Theme::TextStyle textStyle, QJsonObject *root, const QStringList &path, TreeItem * parent)
        : TreeItem(parent)
        , m_translatedName(translatedName)
        , m_textStyle(textStyle)
        , m_root(root)
        , m_path(path)
    {
    }

    QVariant data(int column, int role) const override
    {
        const auto tsRole = static_cast<TextStyleRole>(role);
        if (tsRole == TextStyleRole::TextStyle) {
            return m_textStyle;
        }

        if (tsRole == TextStyleRole::IsDefaultTextStyle) {
            return isSet();
        }

        if (role == Qt::DisplayRole) {
            switch (column) {
                case 0: return m_translatedName;
                case 5: return readColor(*m_root, m_path, QStringLiteral("text-color"), QColor());
                case 6: return readColor(*m_root, m_path, QStringLiteral("selected-text-color"), QColor());
                case 7: return readColor(*m_root, m_path, QStringLiteral("background-color"), QColor());
                case 8: return readColor(*m_root, m_path, QStringLiteral("selected-background-color"), QColor());
            }
        }

        if (role == Qt::CheckStateRole) {
            switch (column) {
                case 1: return readValue(*m_root, m_path, QStringLiteral("bold"), false).toBool() ? Qt::Checked : Qt::Unchecked;
                case 2: return readValue(*m_root, m_path, QStringLiteral("italic"), false).toBool() ? Qt::Checked : Qt::Unchecked;
                case 3: return readValue(*m_root, m_path, QStringLiteral("underline"), false).toBool() ? Qt::Checked : Qt::Unchecked;
                case 4: return readValue(*m_root, m_path, QStringLiteral("strike-through"), false).toBool() ? Qt::Checked : Qt::Unchecked;
            }
        }

        if (role == Qt::FontRole) {
            switch (column) {
                case 0: {
                    const bool bold = readValue(*m_root, m_path, QStringLiteral("bold"), false).toBool();
                    const bool italic = readValue(*m_root, m_path, QStringLiteral("italic"), false).toBool();
                    const bool underline = readValue(*m_root, m_path, QStringLiteral("underline"), false).toBool();
                    const bool strikeThrough = readValue(*m_root, m_path, QStringLiteral("strike-through"), false).toBool();
                    QFont f;
                    f.setBold(bold);
                    f.setItalic(italic);
                    f.setUnderline(underline);
                    f.setStrikeOut(strikeThrough);
                    return f;
                }
            }
        }

        if (role == Qt::ForegroundRole) {
            switch (column) {
                case 0: return readColor(*m_root, m_path, QStringLiteral("text-color"), QRgb(0));
                case 5: return readColor(*m_root, m_path, QStringLiteral("text-color"), QRgb(0));
                case 6: return readColor(*m_root, m_path, QStringLiteral("selected-text-color"), QRgb(0));
                case 7: return readColor(*m_root, m_path, QStringLiteral("background-color"), QRgb(0));
                case 8: return readColor(*m_root, m_path, QStringLiteral("selected-background-color"), QRgb(0));
            }
        }

        if (role == Qt::BackgroundRole) {
            switch (column) {
                case 0: {
                    const auto bgcol = readColor(*m_root, m_path, QStringLiteral("background-color"), QColor(0, 0, 0, 0));
                    return (bgcol == QColor(0, 0, 0, 0)) ? QVariant() : bgcol;
                }
            }
        }

        if (role == Qt::EditRole) {
            switch (column) {
                case 1: return readValue(*m_root, m_path, QStringLiteral("bold"), false).toBool() ? Qt::Checked : Qt::Unchecked;
                case 2: return readValue(*m_root, m_path, QStringLiteral("italic"), false).toBool() ? Qt::Checked : Qt::Unchecked;
                case 3: return readValue(*m_root, m_path, QStringLiteral("underline"), false).toBool() ? Qt::Checked : Qt::Unchecked;
                case 4: return readValue(*m_root, m_path, QStringLiteral("strike-through"), false).toBool() ? Qt::Checked : Qt::Unchecked;
                case 5: return readColor(*m_root, m_path, QStringLiteral("text-color"), QColor(0, 0, 0, 0));
                case 6: return readColor(*m_root, m_path, QStringLiteral("selected-text-color"), QColor(0, 0, 0, 0));
                case 7: return readColor(*m_root, m_path, QStringLiteral("background-color"), QColor(0, 0, 0, 0));
                case 8: return readColor(*m_root, m_path, QStringLiteral("selected-background-color"), QColor(0, 0, 0, 0));
            }
        }

//         if (role == Qt::DecorationRole) {
//             switch (column) {
//                 case 5: return QColor(readValue(*m_root, m_path, QStringLiteral("text-color"), QColor(Qt::black)).toString());
//                 case 6: return QColor(readValue(*m_root, m_path, QStringLiteral("selected-text-color"), QColor(Qt::black)).toString());
//                 case 7: return QColor(readValue(*m_root, m_path, QStringLiteral("background-color"), QColor(Qt::black)).toString());
//                 case 8: return QColor(readValue(*m_root, m_path, QStringLiteral("selected-background-color"), QColor(Qt::black)).toString());
//             }
//         }

        return QVariant();
    }

    bool setData(int column, const QVariant &value, int role = Qt::EditRole)
    {
        switch (column) {
            case 1: writeValue(*m_root, m_path, QStringLiteral("bold"), value.toBool()); break;
            case 2: writeValue(*m_root, m_path, QStringLiteral("italic"), value.toBool()); break;
            case 3: writeValue(*m_root, m_path, QStringLiteral("underline"), value.toBool()); break;
            case 4: writeValue(*m_root, m_path, QStringLiteral("strike-through"), value.toBool()); break;
            case 5: writeValue(*m_root, m_path, QStringLiteral("text-color"), qvariant_cast<QColor>(value)); break;
            case 6: writeValue(*m_root, m_path, QStringLiteral("selected-text-color"), qvariant_cast<QColor>(value)); break;
            case 7: writeValue(*m_root, m_path, QStringLiteral("background-color"), qvariant_cast<QColor>(value)); break;
            case 8: writeValue(*m_root, m_path, QStringLiteral("selected-background-color"), qvariant_cast<QColor>(value)); break;
            default: return false;
        }

        return true;
    }

    int columnCount() const override
    {
        return 9;
    }


    QString translatedName() const
    {
        return m_translatedName;
    }

//     QRgb textColor() const
//     {
//         return readColor(m_root.value(QStringLiteral("text-color")));
//     }
//
//     QRgb backgroundColor() const
//     {
//         return readColor(m_root.value(QStringLiteral("background-color")));
//     }
//
//     QRgb selectedTextColor() const
//     {
//         return readColor(m_root.value(QStringLiteral("selected-text-color")));
//     }
//
//     QRgb selectedBackgroundColor() const
//     {
//         return readColor(m_root.value(QStringLiteral("selected-background-color")));
//     }
//
//     bool bold() const
//     {
//         return readBool(m_root.value(QStringLiteral("bold")));
//     }
//
//     bool italic() const
//     {
//         return readBool(m_root.value(QStringLiteral("italic")));
//     }
//
//     bool underline() const
//     {
//         return readBool(m_root.value(QStringLiteral("underline")));
//     }
//
//     bool strikeThrough() const
//     {
//         return readBool(m_root.value(QStringLiteral("strike-through")));
//     }

    bool isSet() const
    {
        return false;
//         return !(m_root.value(QStringLiteral("text-color")).isUndefined()
//             && m_root.value(QStringLiteral("background-color")).isUndefined()
//             && m_root.value(QStringLiteral("selected-text-color")).isUndefined()
//             && m_root.value(QStringLiteral("selected-background-color")).isUndefined()
//             && m_root.value(QStringLiteral("bold")).isUndefined()
//             && m_root.value(QStringLiteral("italic")).isUndefined()
//             && m_root.value(QStringLiteral("underline")).isUndefined()
//             && m_root.value(QStringLiteral("strike-through")).isUndefined());
    }

    void reset()
    {
//         m_root = QJsonObject();
    }

private:
    QString m_translatedName;
    KSyntaxHighlighting::Theme::TextStyle m_textStyle;
    QJsonObject *m_root;
    QStringList m_path;
};

class CategoryItem : public TreeItem
{
public:
    CategoryItem(const QString &name, QJsonObject *root, TreeItem *parent = nullptr)
        : TreeItem(parent)
        , m_name(name)
        , m_root(root)
    {}

    QVariant data(int column, int role) const override
    {
        if (column == 0 && role == Qt::DisplayRole) {
            return m_name;
        }
        if (column == 0 && role == Qt::ForegroundRole) {
            return readColor(*m_root, QStringList({ QStringLiteral("text-styles"), QStringLiteral("Normal") }), QStringLiteral("text-color"), QColor());
        }
        return QVariant();
    }

    int columnCount() const override
    {
        return 1;
    }

private:
    QString m_name;
    QJsonObject *m_root;
};

class TextStyleModelPrivate
{
public:
    QJsonObject *theme = nullptr;
    TreeItem root;
};

TextStyleModel::TextStyleModel(QObject * parent)
    : QAbstractItemModel(parent)
    , d(new TextStyleModelPrivate())
{
    QStringList headers;
    headers << i18nc("@title:column Meaning of text in editor", "Context") << QString() << QString() << QString() << QString() << i18nc("@title:column Text style", "Normal") << i18nc("@title:column Text style", "Selected") << i18nc("@title:column Text style", "Background") << i18nc("@title:column Text style", "Background Selected");
}

TextStyleModel::~TextStyleModel()
{
    delete d;
}

void TextStyleModel::setTheme(QJsonObject &theme)
{
    beginResetModel();
    d->root.clear();

    d->theme = &theme;

    QStringList l = { QStringLiteral("text-styles") };
    CategoryItem *c = new CategoryItem(i18n("Normal Text & Source Code"), d->theme, &d->root);
    d->root.appendChild(c);
    c->appendChild(new TextStyleItem(i18n("Normal"), KSyntaxHighlighting::Theme::Normal, d->theme, QStringList(l) << QStringLiteral("Normal"), c));
    c->appendChild(new TextStyleItem(i18n("Keyword"), KSyntaxHighlighting::Theme::Keyword, d->theme, QStringList(l) << QStringLiteral("Keyword"), c));
    c->appendChild(new TextStyleItem(i18n("Function"), KSyntaxHighlighting::Theme::Function, d->theme, QStringList(l) << QStringLiteral("Function"), c));
    c->appendChild(new TextStyleItem(i18n("Variable"), KSyntaxHighlighting::Theme::Variable, d->theme, QStringList(l) << QStringLiteral("Variable"), c));
    c->appendChild(new TextStyleItem(i18n("Control Flow"), KSyntaxHighlighting::Theme::ControlFlow, d->theme, QStringList(l) << QStringLiteral("ControlFlow"), c));
    c->appendChild(new TextStyleItem(i18n("Operator"), KSyntaxHighlighting::Theme::Operator, d->theme, QStringList(l) << QStringLiteral("Operator"), c));
    c->appendChild(new TextStyleItem(i18n("Built-In"), KSyntaxHighlighting::Theme::BuiltIn, d->theme, QStringList(l) << QStringLiteral("BuiltIn"), c));
    c->appendChild(new TextStyleItem(i18n("Extension"), KSyntaxHighlighting::Theme::Extension, d->theme, QStringList(l) << QStringLiteral("Extension"), c));
    c->appendChild(new TextStyleItem(i18n("Preprocessor"), KSyntaxHighlighting::Theme::Preprocessor, d->theme, QStringList(l) << QStringLiteral("Preprocessor"), c));
    c->appendChild(new TextStyleItem(i18n("Attribute"), KSyntaxHighlighting::Theme::Attribute, d->theme, QStringList(l) << QStringLiteral("Attribute"), c));

    c = new CategoryItem(i18n("Numbers, Types & Constants"), d->theme, &d->root);
    d->root.appendChild(c);
    c->appendChild(new TextStyleItem(i18n("Data Type"), KSyntaxHighlighting::Theme::DataType, d->theme, QStringList(l) << QStringLiteral("DataType"), c));
    c->appendChild(new TextStyleItem(i18n("Decimal/Value"), KSyntaxHighlighting::Theme::DecVal, d->theme, QStringList(l) << QStringLiteral("DecVal"), c));
    c->appendChild(new TextStyleItem(i18n("Base-N Integer"), KSyntaxHighlighting::Theme::BaseN, d->theme, QStringList(l) << QStringLiteral("BaseN"), c));
    c->appendChild(new TextStyleItem(i18n("Floating Point"), KSyntaxHighlighting::Theme::Float, d->theme, QStringList(l) << QStringLiteral("Float"), c));
    c->appendChild(new TextStyleItem(i18n("Constant"), KSyntaxHighlighting::Theme::Constant, d->theme, QStringList(l) << QStringLiteral("Constant"), c));

    c = new CategoryItem(i18n("Strings & Characters"), d->theme, &d->root);
    d->root.appendChild(c);
    c->appendChild(new TextStyleItem(i18n("Character"), KSyntaxHighlighting::Theme::Char, d->theme, QStringList(l) << QStringLiteral("Char"), c));
    c->appendChild(new TextStyleItem(i18n("Special Character"), KSyntaxHighlighting::Theme::SpecialChar, d->theme, QStringList(l) << QStringLiteral("SpecialChar"), c));
    c->appendChild(new TextStyleItem(i18n("String"), KSyntaxHighlighting::Theme::String, d->theme, QStringList(l) << QStringLiteral("String"), c));
    c->appendChild(new TextStyleItem(i18n("Verbatim String"), KSyntaxHighlighting::Theme::VerbatimString, d->theme, QStringList(l) << QStringLiteral("VerbatimString"), c));
    c->appendChild(new TextStyleItem(i18n("Special String"), KSyntaxHighlighting::Theme::SpecialString, d->theme, QStringList(l) << QStringLiteral("SpecialString"), c));
    c->appendChild(new TextStyleItem(i18n("Import, Modules, Includes"), KSyntaxHighlighting::Theme::Import, d->theme, QStringList(l) << QStringLiteral("Import"), c));

    c = new CategoryItem(i18n("Comments & Documentation"), d->theme, &d->root);
    d->root.appendChild(c);
    c->appendChild(new TextStyleItem(i18n("Comment"), KSyntaxHighlighting::Theme::Comment, d->theme, QStringList(l) << QStringLiteral("Comment"), c));
    c->appendChild(new TextStyleItem(i18n("Documentation"), KSyntaxHighlighting::Theme::Documentation, d->theme, QStringList(l) << QStringLiteral("Documentation"), c));
    c->appendChild(new TextStyleItem(i18n("Annotation"), KSyntaxHighlighting::Theme::Annotation, d->theme, QStringList(l) << QStringLiteral("Annotation"), c));
    c->appendChild(new TextStyleItem(i18n("CommentVar"), KSyntaxHighlighting::Theme::CommentVar, d->theme, QStringList(l) << QStringLiteral("CommentVar"), c));
    c->appendChild(new TextStyleItem(i18n("RegionMarker"), KSyntaxHighlighting::Theme::RegionMarker, d->theme, QStringList(l) << QStringLiteral("RegionMarker"), c));
    c->appendChild(new TextStyleItem(i18n("Information"), KSyntaxHighlighting::Theme::Information, d->theme, QStringList(l) << QStringLiteral("Information"), c));
    c->appendChild(new TextStyleItem(i18n("Warning"), KSyntaxHighlighting::Theme::Warning, d->theme, QStringList(l) << QStringLiteral("Warning"), c));
    c->appendChild(new TextStyleItem(i18n("Alert"), KSyntaxHighlighting::Theme::Alert, d->theme, QStringList(l) << QStringLiteral("Alert"), c));

    c = new CategoryItem(i18n("Miscellaneous"), d->theme, &d->root);
    d->root.appendChild(c);
    c->appendChild(new TextStyleItem(i18n("Error"), KSyntaxHighlighting::Theme::Error, d->theme, QStringList(l) << QStringLiteral("Error"), c));
    c->appendChild(new TextStyleItem(i18n("Others"), KSyntaxHighlighting::Theme::Others, d->theme, QStringList(l) << QStringLiteral("Others"), c));

    endResetModel();
}

QJsonObject& TextStyleModel::theme()
{
    return *d->theme;
}

QModelIndex TextStyleModel::index(int row, int column, const QModelIndex &parent) const
{
    // top-level categories for the default styles
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

QModelIndex TextStyleModel::parent(const QModelIndex &index) const
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

int TextStyleModel::rowCount(const QModelIndex &index) const
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

int TextStyleModel::columnCount(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return 9;
    }

    auto treeItem = static_cast<TreeItem*>(index.internalPointer());
    if (treeItem) {
        return treeItem->columnCount();
    }
    return 0;
}

QVariant TextStyleModel::data(const QModelIndex &index, int role) const
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

Qt::ItemFlags TextStyleModel::flags(const QModelIndex &index) const
{
    auto f = QAbstractItemModel::flags(index);
    if (index.isValid() && index.column() > 0) {
        auto treeItem = static_cast<TreeItem*>(index.internalPointer());
        auto textStyleItem = dynamic_cast<TextStyleItem*>(treeItem);
        if (textStyleItem) {
            f = f | Qt::ItemIsEditable;
            if (index.column() >= 1 && index.column() <= 4) {
                f = f | Qt::ItemIsUserCheckable;
            }
        }
    }
    return f;
}

QVariant TextStyleModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Vertical) {
        return QVariant();
    }

    // Icons: Alternatively, we could return icons for the DecorationRole for B, I, U, S:
    // QIcon::fromTheme(QStringLiteral("format-text-bold"))
    // QIcon::fromTheme(QStringLiteral("format-text-italic"))
    // QIcon::fromTheme(QStringLiteral("format-text-underline"))
    // QIcon::fromTheme(QStringLiteral("format-text-strikethrough"))

    if (role == Qt::DisplayRole) {
        switch (section) {
            case 0: return QLatin1String("Context");
            case 1: return QLatin1String("B");
            case 2: return QLatin1String("I");
            case 3: return QLatin1String("U");
            case 4: return QLatin1String("S");
            case 5: return QLatin1String("Normal");
            case 6: return QLatin1String("Selected");
            case 7: return QLatin1String("Background");
            case 8: return QLatin1String("Selected Background");
        }
    }
    else if (role == Qt::FontRole) {
        switch (section) {
            case 1: {
                QFont f;
                f.setBold(true);
                return f;
            }
            case 2: {
                QFont f;
                f.setItalic(true);
                return f;
            }
            case 3: {
                QFont f;
                f.setUnderline(true);
                return f;
            }
            case 4: {
                QFont f;
                f.setStrikeOut(true);
                return f;
            }
        }
    }
    else if (role == Qt::TextAlignmentRole) {
        switch (section) {
            case 1:
            case 2:
            case 3:
            case 4:return Qt::AlignHCenter;
        }
    }
    else if (role == Qt::ToolTipRole) {
        switch (section) {
            case 1: return QStringLiteral("Bold Text");
            case 2: return QStringLiteral("Italic Text");
            case 3: return QStringLiteral("Underline Text");
            case 4: return QStringLiteral("Strikeout Text");
        }
    }

    return QVariant();
}

bool TextStyleModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || index.column() == 0) {
        return false;
    }

    auto treeItem = static_cast<TreeItem*>(index.internalPointer());
    auto textStyleItem = dynamic_cast<TextStyleItem*>(treeItem);
    if (textStyleItem) {
        const auto ok = textStyleItem->setData(index.column(), value, role);
        if (ok) {
            Q_EMIT dataChanged(createIndex(index.row(), 0, index.internalPointer()), createIndex(index.row(), 9, index.internalPointer()));
        }
    }

    return false;
}
