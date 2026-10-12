
/***************************************************************************
 *   Copyright (C) 2014 by Petr Vanek                                      *
 *   petr@scribus.info                                                     *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program.  If not, see <http://www.gnu.org/licenses/>. *
 ***************************************************************************/

#ifndef BOOKMARKSWIDGET_H
#define BOOKMARKSWIDGET_H

#include "ui_bookmarkswidget.h"

class AbstractBookmarkItem;
class BookmarksModel;
class TabWidget;
class TermWidget;

class BookmarksWidget : public QWidget, Ui::BookmarksWidget
{
    Q_OBJECT

public:
    BookmarksWidget(QWidget *parent = nullptr, TabWidget *tabs = nullptr);
    ~BookmarksWidget() override;

    void setup();

signals:
    void callCommand(const QString &cmd);

private:
    BookmarksModel *m_model;
    TabWidget *m_tabs;
    QList<QPushButton *> m_variableTabButtons;
    QList<TermWidget *> m_variableTerminals;
    int m_variableTabIndex = -1;
    TermWidget *m_selectedVariableTerminal = nullptr;

    void updateVariableTabButtons();
    void updateVariablePaneSelector();
    void requestManagedVariables(TermWidget *terminal);

private slots:
    void handleCommand(const QModelIndex &index);
    void filter(const QString &str);
    void handleManagedVariablesResponse(const QString &response);
    void handleManagedVariablesChanged();
    void handleTerminalsChanged();
};

class BookmarksModel : public QAbstractItemModel
{
    Q_OBJECT

public:
    BookmarksModel(QObject *parent = nullptr);
    ~BookmarksModel() override;

    void setup();

    QVariant data(const QModelIndex &index, int role) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    QModelIndexList allChildRows(const QModelIndex &parent) const;

private:
    AbstractBookmarkItem *getItem(const QModelIndex &index) const;
    AbstractBookmarkItem *m_root;
};

#endif

