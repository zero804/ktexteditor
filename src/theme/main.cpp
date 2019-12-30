#include <QApplication>
#include <QFile>
#include <QDebug>

#include <QTreeView>
#include <QJsonDocument>
#include <QSplitter>

#include <KSyntaxHighlighting/Repository>
#include <KSyntaxHighlighting/Theme>

#include "EditorColorModel.h"
#include "TextStyleModel.h"
#include "TextStyleTreeView.h"

int main(int argc, char * argv[])
{
    QApplication app(argc, argv);

    KSyntaxHighlighting::Repository repo;

    QByteArray themeData;
    QFile themeFile(QStringLiteral(":/org.kde.syntax-highlighting/themes/breeze-dark.theme"));
    if (themeFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        themeData = themeFile.readAll();
    }

    QJsonObject theme = QJsonDocument::fromJson(themeData).object();

    EditorColorModel colorModel;
    colorModel.setTheme(theme);
    auto * treeView = new QTreeView();
    treeView->setModel(&colorModel);
    treeView->expandAll();

    TextStyleModel model;
    model.setTheme(theme);

    TextStyleTreeView * tree = new TextStyleTreeView();
    tree->setModel(&model);
    tree->updatePalette();
    tree->expandAll();
    for (int i = 0; i < model.columnCount(); ++i) {
        tree->resizeColumnToContents(i);
    }
    for (int i = 0; i < model.rowCount(); ++i) {
        tree->setFirstColumnSpanned(i, QModelIndex(), true);
    }

    QSplitter * top = new QSplitter(Qt::Horizontal);
    top->addWidget(treeView);
    top->addWidget(tree);
    top->resize(800, 600);
    top->show();

    return app.exec();
}
