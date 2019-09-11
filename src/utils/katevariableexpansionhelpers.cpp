/* This file is part of the KDE project
 *
 *  Copyright 2019 Dominik Haumann <dhaumann@kde.org>
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
#include "katevariableexpansionhelpers.h"

#include <KTextEditor/Editor>
#include <KTextEditor/Application>
#include <KTextEditor/MainWindow>
#include "kateglobal.h"
#include <KLocalizedString>

#include <QAbstractItemModel>
#include <QAction>
#include <QEvent>
#include <QHelpEvent>
#include <QLabel>
#include <QLineEdit>
#include <QListView>
#include <QTextEdit>
#include <QToolTip>
#include <QVBoxLayout>

/**
 * Find closing bracket for @p str starting a position @p pos.
 */
static int findClosing(QStringView str, int pos = 0)
{
    const int len = str.size();
    int nesting = 0;

    while (pos < len) {
        ++pos;
        const QChar c = str[pos];
        if (c == QLatin1Char('}')) {
            if (nesting == 0) {
                return pos;
            }
            nesting--;
        } else if (c == QLatin1Char('{')) {
            nesting++;
        }
    }
    return -1;
}

namespace KateMacroExpander
{

QString expandMacro(const QString& input, KTextEditor::View* view)
{
    QString output = input;
    QString oldStr;
    do {
        oldStr = output;
        const int startIndex = output.indexOf(QLatin1String("%{"));
        if (startIndex < 0) {
            break;
        }

        const int endIndex = findClosing(output, startIndex + 2);
        if (endIndex <= startIndex) {
            break;
        }

        const int varLen = endIndex - (startIndex + 2);
        QString variable = output.mid(startIndex + 2, varLen);
        variable = expandMacro(variable, view);
        if (KTextEditor::Editor::instance()->expandVariable(variable, view, variable)) {
            output.replace(startIndex, endIndex - startIndex + 1, variable);
        }
    } while (output != oldStr); // str comparison guards against infinite loop
    return output;
}

}

class VariableItemModel : public QAbstractItemModel
{
public:
    VariableItemModel(QObject *parent = nullptr)
        : QAbstractItemModel(parent)
    {
    }

    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override
    {
        if (parent.isValid() ||
            row < 0 ||
            row >= m_variables.size())
        {
            return {};
        }

        return createIndex(row, column);
    }

    QModelIndex parent(const QModelIndex &index) const override
    {
        Q_UNUSED(index)
        // flat list -> we never have parents
        return {};
    }

    int rowCount(const QModelIndex &parent = QModelIndex()) const override
    {
        return parent.isValid() ? 0 : m_variables.size();
    }

    int columnCount(const QModelIndex &parent = QModelIndex()) const override
    {
        Q_UNUSED(parent)
        return 3; // name | description | current value
    }

    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override
    {
        if (!index.isValid()) {
            return {};
        }

        const auto & var = m_variables[index.row()];
        switch (role) {
            case Qt::DisplayRole: {
                const QString suffix = var.isPrefixMatch() ? i18n("<value>") : QString();
                return QString(var.name() + suffix);
            }
            case Qt::ToolTipRole: return var.description();
        }

        return {};
    }

    void setVariables(const QVector<KTextEditor::Variable> &variables)
    {
        beginResetModel();
        m_variables = variables;
        endResetModel();
    }

private:
    QVector<KTextEditor::Variable> m_variables;
};

KateVariableExpansionDialog::KateVariableExpansionDialog(QWidget *parent)
    : QDialog(parent, Qt::Tool)
    , m_showAction(new QAction(QIcon::fromTheme(QStringLiteral("code-context")), i18n("Insert variable"), this))
    , m_variableModel(new VariableItemModel(this))
    , m_listView(new QListView(this))
{
    setWindowTitle(i18n("Variables"));

    auto vbox = new QVBoxLayout(this);
    vbox->addWidget(m_listView);
    m_listView->setModel(m_variableModel);
    m_listView->setUniformItemSizes(true);

    auto lblDescription = new QLabel(i18n("Please select a variable."), this);
    auto lblCurrentValue = new QLabel(this);

    vbox->addWidget(lblDescription);
    vbox->addWidget(lblCurrentValue);

    // react to selection changes
    connect(m_listView, &QAbstractItemView::activated, [this, lblDescription, lblCurrentValue](const QModelIndex &index) {
        if (index.isValid()) {
            const auto & var = m_variables[index.row()];
            lblDescription->setText(var.description());
            if (var.isPrefixMatch()) {
                lblCurrentValue->setText(i18n("Current value: %1<value>", var.name()));
            } else {
                auto activeView = KTextEditor::Editor::instance()->application()->activeMainWindow()->activeView();
                const auto value = var.evaluate(var.name(), activeView);
                lblCurrentValue->setText(i18n("Current value: %1", value));
            }
        } else {
            lblDescription->setText(i18n("Please select a variable."));
            lblCurrentValue->clear();
        }
    });

    // insert text on activation
    connect(m_listView, &QAbstractItemView::doubleClicked, [this, lblDescription, lblCurrentValue](const QModelIndex &index) {
        if (index.isValid()) {
            const auto & var = m_variables[index.row()];
            const auto name = QStringLiteral("%{") + var.name() + QLatin1Char('}');

            if (parentWidget() && parentWidget()->window()) {
                auto currentWidget = parentWidget()->window()->focusWidget();
                if (auto lineEdit = qobject_cast<QLineEdit*>(currentWidget)) {
                    lineEdit->insert(name);
                } else if (auto textEdit = qobject_cast<QTextEdit*>(currentWidget)) {
                    textEdit->insertPlainText(name);
                }
            }
        }
    });

    // show dialog whenever the action is clicked
    connect(m_showAction, &QAction::triggered, [this](){
        show();
        activateWindow();
    });
}

void KateVariableExpansionDialog::addVariable(const KTextEditor::Variable& variable)
{
    Q_ASSERT(variable.isValid());
    m_variables.push_back(variable);

    m_variableModel->setVariables(m_variables);
}

int KateVariableExpansionDialog::isEmpty() const
{
    return m_variables.isEmpty();
}

void KateVariableExpansionDialog::addWidget(QWidget *widget)
{
    m_widgets.push_back(widget);
    widget->installEventFilter(this);

    connect(widget, &QObject::destroyed, this, &KateVariableExpansionDialog::onObjectDeleted);
}

void KateVariableExpansionDialog::onObjectDeleted(QObject* object)
{
    m_widgets.removeAll(object);
    if (m_widgets.isEmpty()) {
        deleteLater();
    }
}

bool KateVariableExpansionDialog::eventFilter(QObject *watched, QEvent *event)
{
    switch (event->type()) {
        case QEvent::FocusIn: {
            if (auto lineEdit = qobject_cast<QLineEdit*>(watched)) {
                lineEdit->addAction(m_showAction, QLineEdit::TrailingPosition);
            }
            break;
        }
        case QEvent::FocusOut: {
            if (auto lineEdit = qobject_cast<QLineEdit*>(watched)) {
                lineEdit->removeAction(m_showAction);
            }
            break;
        }
        case QEvent::ToolTip: {
            QString inputText;
            if (auto lineEdit = qobject_cast<QLineEdit*>(watched)) {
                inputText = lineEdit->text();
            }
            QString toolTip;
            if (!inputText.isEmpty()) {
                auto activeView = KTextEditor::Editor::instance()->application()->activeMainWindow()->activeView();
                KTextEditor::Editor::instance()->expandText(inputText, activeView, toolTip);
            }

            if (!toolTip.isEmpty()) {
                auto helpEvent = static_cast<QHelpEvent*>(event);
                QToolTip::showText(helpEvent->globalPos(), toolTip, qobject_cast<QWidget*>(watched));
                event->accept();
                return true;
            }
            break;
        }
        default: break;
    }

    auto parentWindow = parentWidget()->window();
    const bool keepVisible = isActiveWindow() || m_widgets.contains(parentWindow->focusWidget());
    if (!keepVisible) {
        hide();
    }

    return QDialog::eventFilter(watched, event);
}

// kate: space-indent on; indent-width 4; replace-tabs on;