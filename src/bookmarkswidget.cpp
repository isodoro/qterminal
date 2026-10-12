
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

#include <QDebug>
#include <QShortcut>
#include <QHBoxLayout>
#include <QFontMetrics>
#include <QResizeEvent>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonParseError>
#include <QListWidgetItem>
#include <QSignalBlocker>
#include <algorithm>
#include <QPoint>
#include <QInputDialog>
#include <QLineEdit>

#include "bookmarkswidget.h"
#include "properties.h"
#include "config.h"
#include "tabwidget.h"
#include "termwidgetholder.h"
#include "termwidget.h"

namespace
{
class ElidedTabButton : public QPushButton
{
public:
    explicit ElidedTabButton(const QString &text, QWidget *parent)
        : QPushButton(parent)
        , m_fullText(text)
    {
        setToolTip(m_fullText);
        updateElidedText();
    }

protected:
    void resizeEvent(QResizeEvent *event) override
    {
        QPushButton::resizeEvent(event);
        updateElidedText();
    }

private:
    QString m_fullText;

    void updateElidedText()
    {
        const int availableWidth = qMax(
            0,
            width()
                - contentsMargins().left()
                - contentsMargins().right()
                - 12);

        QPushButton::setText(
            fontMetrics().elidedText(
                m_fullText,
                Qt::ElideRight,
                availableWidth));
    }
};
}

class AbstractBookmarkItem
{
public:
    enum ItemType {
        Root = 0,
        Group = 1,
        Command = 2
    };

    AbstractBookmarkItem(ItemType type, AbstractBookmarkItem *parent = nullptr)
        : m_type(type),
          m_parent(parent)
    {
    }

    virtual ~AbstractBookmarkItem()
    {
        qDeleteAll(m_children);
    }

    ItemType type() { return m_type; }
    QString value() { return m_value; }
    QString display() { return m_display; }

    void addChild(AbstractBookmarkItem *item) { m_children << item; }
    int childCount() { return m_children.count(); }
    QList<AbstractBookmarkItem *> children() { return m_children; }
    AbstractBookmarkItem *child(int number) { return m_children.value(number); }
    AbstractBookmarkItem *parent() { return m_parent; }

    int childNumber() const
    {
        if (m_parent)
            return m_parent->children().indexOf(
                const_cast<AbstractBookmarkItem *>(this));

        return 0;
    }

protected:
    ItemType m_type;
    AbstractBookmarkItem *m_parent;
    QList<AbstractBookmarkItem *> m_children;
    QString m_value;
    QString m_display;
};

class BookmarkRootItem : public AbstractBookmarkItem
{
public:
    BookmarkRootItem()
        : AbstractBookmarkItem(AbstractBookmarkItem::Root)
    {
        m_value = m_display = QStringLiteral("root");
    }
};

class BookmarkCommandItem : public AbstractBookmarkItem
{
public:
    BookmarkCommandItem(const QString &name, const QString &command,
                        AbstractBookmarkItem *parent)
        : AbstractBookmarkItem(AbstractBookmarkItem::Command, parent)
    {
        m_value = command;
        m_display = name;
    }
};

class BookmarkGroupItem : public AbstractBookmarkItem
{
public:
    BookmarkGroupItem(const QString &name, AbstractBookmarkItem *parent)
        : AbstractBookmarkItem(AbstractBookmarkItem::Group, parent)
    {
        m_display = name;
    }
};

class BookmarkFileGroupItem : public BookmarkGroupItem
{
    QHash<QString, AbstractBookmarkItem *> m_map;
    QStringList m_pos;

public:
    BookmarkFileGroupItem(AbstractBookmarkItem *parent, const QString &fname)
        : BookmarkGroupItem(QObject::tr("Synchronized Bookmarks"), parent)
    {
        QFile f(fname);
        if (!f.open(QIODevice::ReadOnly))
            return;

        QXmlStreamReader xml;
        xml.setDevice(&f);

        while (true)
        {
            xml.readNext();

            switch (xml.tokenType())
            {
            case QXmlStreamReader::StartElement:
            {
                AbstractBookmarkItem *parentItem =
                    m_map.contains(xmlPos()) ? m_map[xmlPos()] : this;
                const QString tag = xml.name().toString();

                if (tag == QLatin1String("group"))
                {
                    const QString name =
                        xml.attributes().value(QLatin1String("name")).toString();
                    m_pos.append(name);

                    auto *item = new BookmarkGroupItem(name, parentItem);
                    parentItem->addChild(item);
                    m_map[xmlPos()] = item;
                }
                else if (tag == QLatin1String("command"))
                {
                    const QString name =
                        xml.attributes().value(QLatin1String("name")).toString();
                    const QString command =
                        xml.attributes().value(QLatin1String("value")).toString();

                    auto *item =
                        new BookmarkCommandItem(name, command, parentItem);
                    parentItem->addChild(item);
                }
                break;
            }
            case QXmlStreamReader::EndElement:
                if (xml.name() == QLatin1String("group"))
                    m_pos.removeLast();
                break;

            case QXmlStreamReader::Invalid:
                qDebug() << "XML error:" << xml.errorString()
                         << xml.lineNumber() << xml.columnNumber();
                m_map.clear();
                return;

            case QXmlStreamReader::EndDocument:
                m_map.clear();
                return;

            default:
                break;
            }
        }
    }

    QString xmlPos()
    {
        return m_pos.join(QLatin1Char('.'));
    }
};

BookmarksModel::BookmarksModel(QObject *parent)
    : QAbstractItemModel(parent),
      m_root(nullptr)
{
    setup();
}

void BookmarksModel::setup()
{
    delete m_root;
    m_root = new BookmarkRootItem();
    m_root->addChild(
        new BookmarkFileGroupItem(m_root, Properties::Instance()->bookmarksFile));

    beginResetModel();
    endResetModel();
}

BookmarksModel::~BookmarksModel()
{
    delete m_root;
    m_root = nullptr;
}

int BookmarksModel::columnCount(const QModelIndex & /*parent*/) const
{
    return 2;
}

QVariant BookmarksModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid())
        return QVariant();

    switch (role)
    {
    case Qt::DisplayRole:
    case Qt::EditRole:
        return index.column() == 0
            ? getItem(index)->display()
            : getItem(index)->value();

    case Qt::FontRole:
    {
        QFont font;
        if (static_cast<AbstractBookmarkItem *>(index.internalPointer())->type()
            == AbstractBookmarkItem::Group)
        {
            font.setBold(true);
        }
        return font;
    }

    default:
        return QVariant();
    }
}

AbstractBookmarkItem *BookmarksModel::getItem(const QModelIndex &index) const
{
    if (index.isValid())
    {
        auto *item =
            static_cast<AbstractBookmarkItem *>(index.internalPointer());
        if (item)
            return item;
    }

    return m_root;
}

QVariant BookmarksModel::headerData(int /*section*/,
                                    Qt::Orientation /*orientation*/,
                                    int /*role*/) const
{
    return QVariant();
}

QModelIndex BookmarksModel::index(int row, int column,
                                  const QModelIndex &parent) const
{
    if (parent.isValid() && parent.column() != 0)
        return QModelIndex();

    AbstractBookmarkItem *parentItem = getItem(parent);
    AbstractBookmarkItem *childItem = parentItem->child(row);

    if (childItem)
        return createIndex(row, column, childItem);

    return QModelIndex();
}

QModelIndex BookmarksModel::parent(const QModelIndex &index) const
{
    if (!index.isValid())
        return QModelIndex();

    AbstractBookmarkItem *childItem = getItem(index);
    AbstractBookmarkItem *parentItem = childItem->parent();

    if (parentItem == m_root)
        return QModelIndex();

    return createIndex(parentItem->childNumber(), 0, parentItem);
}

int BookmarksModel::rowCount(const QModelIndex &parent) const
{
    AbstractBookmarkItem *parentItem = getItem(parent);
    return parentItem->childCount();
}

QModelIndexList BookmarksModel::allChildRows(const QModelIndex &parent) const
{
    QModelIndexList list;

    for (int row = 0; row < rowCount(parent); ++row)
    {
        const QModelIndex child = index(row, 0, parent);
        list << child;

        if (hasChildren(child))
            list << allChildRows(child);
    }

    return list;
}

#if 0
bool BookmarksModel::setData(const QModelIndex &index, const QVariant &value,
                             int role)
{
    if (role != Qt::EditRole)
        return false;

    AbstractBookmarkItem *item = getItem(index);
    bool result = item->setData(index.column(), value);

    if (result)
        emit dataChanged(index, index);

    return result;
}
#endif

BookmarksWidget::BookmarksWidget(QWidget *parent, TabWidget *tabs)
    : QWidget(parent)
    , m_model(new BookmarksModel(this))
    , m_tabs(tabs)
{
    setupUi(this);
    // Display the active pane without allowing manual selection.
    variablePaneComboBox->setFocusPolicy(Qt::NoFocus);
    variablePaneComboBox->setAttribute(
        Qt::WA_TransparentForMouseEvents);
    variablePaneComboBox->setMinimumContentsLength(6);
    variablePaneComboBox->setMaximumWidth(100);
    variablePaneComboBox->setStyleSheet(
        QStringLiteral(
            "QComboBox {"
            " border: none;"
            " background: transparent;"
            " padding: 0px;"
            "}"
            "QComboBox::drop-down {"
            " border: none;"
            " width: 0px;"
            "}"
            "QComboBox::down-arrow {"
            " image: none;"
            "}"
        )
    );
    listVariablesButton->setFixedWidth(26);
    editVariableButton->setFixedWidth(26);
    addVariableButton->setFixedWidth(26);
    deleteVariableButton->setFixedWidth(26);
    // Temporarily hide the variable tabs and variable list.
    variablesSectionLabel->setVisible(false);
    variableTabsScrollArea->setVisible(false);
    managedVariablesList->setVisible(false);
    managedVariablesList->setSelectionMode(QAbstractItemView::NoSelection);
    managedVariablesList->setFocusPolicy(Qt::NoFocus);
    managedVariablesList->setCursor(Qt::ArrowCursor);
    managedVariablesList->setAttribute(Qt::WA_TransparentForMouseEvents);

    if (m_tabs)
    {
        connect(m_tabs, &QTabWidget::currentChanged,
                this, [this](int) {
            updateVariableTabButtons();
        });

        connect(m_tabs, &TabWidget::tabNameChanged,
                this, [this](int) {
            updateVariableTabButtons();
        });
    }

    treeView->setModel(m_model);
    treeView->header()->hide();
    setFocusProxy(filterEdit);

    connect(treeView, &QTreeView::activated,
            this, &BookmarksWidget::handleCommand);

    connect(filterEdit, &QLineEdit::textChanged,
            this, &BookmarksWidget::filter);

    connect(expandAllButton, &QPushButton::clicked,
            treeView, &QTreeView::expandAll);

    connect(collapseAllButton, &QPushButton::clicked,
            treeView, &QTreeView::collapseAll);

    connect(variablePaneComboBox,
            QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, [this](int index) {
        if (index < 0 || index >= m_variableTerminals.size())
            return;

        m_selectedVariableTerminal = m_variableTerminals.at(index);
        requestManagedVariables(m_selectedVariableTerminal);
    });

    connect(listVariablesButton, &QPushButton::clicked,
            this, [this] {
        if (!m_tabs)
            return;

        const int tabIndex = m_tabs->currentIndex();

        if (tabIndex < 0 || tabIndex >= m_tabs->count())
            return;

        auto *holder = qobject_cast<TermWidgetHolder *>(
            m_tabs->widget(tabIndex));

        if (!holder)
            return;

        TermWidget *terminal = holder->currentTerminal();

        if (!terminal || !terminal->impl())
            return;

        const int paneIndex = m_variableTerminals.indexOf(terminal);

        if (paneIndex < 0)
            return;

        {
            const QSignalBlocker blocker(variablePaneComboBox);
            variablePaneComboBox->setCurrentIndex(paneIndex);
        }

        m_selectedVariableTerminal = terminal;

        QJsonObject request;
        request.insert(
            QStringLiteral("op"), QStringLiteral("ui_list"));

        terminal->impl()->enqueueManagedVariableRequest(request);
        terminal->setFocus(Qt::OtherFocusReason);
    });

    connect(addVariableButton, &QPushButton::clicked,
            this, [this] {
        if (!m_tabs)
            return;

        const int tabIndex = m_tabs->currentIndex();

        if (tabIndex < 0 || tabIndex >= m_tabs->count())
            return;

        auto *holder = qobject_cast<TermWidgetHolder *>(
            m_tabs->widget(tabIndex));

        if (!holder)
            return;

        TermWidget *terminal = holder->currentTerminal();

        if (!terminal || !terminal->impl())
            return;

        const int paneIndex = m_variableTerminals.indexOf(terminal);

        if (paneIndex < 0)
            return;

        {
            const QSignalBlocker blocker(variablePaneComboBox);
            variablePaneComboBox->setCurrentIndex(paneIndex);
        }

        m_selectedVariableTerminal = terminal;

        QJsonObject request;
        request.insert(
            QStringLiteral("op"), QStringLiteral("ui_add"));

        terminal->impl()->enqueueManagedVariableRequest(request);
        terminal->setFocus(Qt::OtherFocusReason);
    });

        connect(editVariableButton, &QPushButton::clicked,
                this, [this] {
            if (!m_tabs)
                return;

            const int tabIndex = m_tabs->currentIndex();

            if (tabIndex < 0 || tabIndex >= m_tabs->count())
                return;

            auto *holder = qobject_cast<TermWidgetHolder *>(
                m_tabs->widget(tabIndex));

            if (!holder)
                return;

            TermWidget *terminal = holder->currentTerminal();

            if (!terminal || !terminal->impl())
                return;

            const int paneIndex = m_variableTerminals.indexOf(terminal);

            if (paneIndex < 0)
                return;

            {
                const QSignalBlocker blocker(variablePaneComboBox);
                variablePaneComboBox->setCurrentIndex(paneIndex);
            }

            m_selectedVariableTerminal = terminal;

            QJsonObject request;
            request.insert(
                QStringLiteral("op"), QStringLiteral("ui_edit"));

            terminal->impl()->enqueueManagedVariableRequest(request);
            terminal->setFocus(Qt::OtherFocusReason);
        });
    connect(deleteVariableButton, &QPushButton::clicked,
            this, [this] {
        if (!m_tabs)
            return;

        const int tabIndex = m_tabs->currentIndex();

        if (tabIndex < 0 || tabIndex >= m_tabs->count())
            return;

        auto *holder = qobject_cast<TermWidgetHolder *>(
            m_tabs->widget(tabIndex));

        if (!holder)
            return;

        TermWidget *terminal = holder->currentTerminal();

        if (!terminal || !terminal->impl())
            return;

        const int paneIndex = m_variableTerminals.indexOf(terminal);

        if (paneIndex < 0)
            return;

        {
            const QSignalBlocker blocker(variablePaneComboBox);
            variablePaneComboBox->setCurrentIndex(paneIndex);
        }

        m_selectedVariableTerminal = terminal;

        QJsonObject request;
        request.insert(
            QStringLiteral("op"), QStringLiteral("ui_delete"));

        terminal->impl()->enqueueManagedVariableRequest(request);
        terminal->setFocus(Qt::OtherFocusReason);
    });

    QShortcut *clearFilter =
        new QShortcut(QKeySequence(Qt::Key_Escape), this);

    connect(clearFilter, &QShortcut::activated, this, [this] {
        filterEdit->clear();
    });

    updateVariableTabButtons();
}

void BookmarksWidget::updateVariableTabButtons()
{
    if (!m_tabs)
        return;

    while (!m_variableTabButtons.isEmpty())
    {
        QPushButton *button = m_variableTabButtons.takeLast();
        variableTabsLayout->removeWidget(button);
        delete button;
    }

        m_variableTabIndex = m_tabs->currentIndex();

    for (int i = 0; i < m_tabs->count(); ++i)
    {
        auto *holder = qobject_cast<TermWidgetHolder *>(m_tabs->widget(i));

        if (holder)
        {
            connect(holder,
                    &TermWidgetHolder::terminalsChanged,
                    this,
                    &BookmarksWidget::handleTerminalsChanged,
                    Qt::UniqueConnection);

            connect(holder,
                    &TermWidgetHolder::termFocusChanged,
                    this,
                    &BookmarksWidget::handleTerminalsChanged,
                    Qt::UniqueConnection);
        }

        auto *button = new ElidedTabButton(
            m_tabs->tabText(i), variableTabsWidget);

        button->setCheckable(true);
        button->setChecked(i == m_variableTabIndex);
        button->setMinimumWidth(0);
        button->setSizePolicy(
            QSizePolicy::Expanding, QSizePolicy::Fixed);
        button->setMaximumHeight(26);

        variableTabsLayout->addWidget(button, 1);
        m_variableTabButtons.append(button);

        button->setAttribute(Qt::WA_TransparentForMouseEvents);
        button->setFocusPolicy(Qt::NoFocus);
    }

    variableTabsWidget->setMinimumWidth(0);
    variableTabsWidget->setSizePolicy(
        QSizePolicy::Ignored, QSizePolicy::Fixed);

    updateVariablePaneSelector();
}

void BookmarksWidget::handleTerminalsChanged()
{
    updateVariablePaneSelector();
}

void BookmarksWidget::updateVariablePaneSelector()
{
    TermWidget *previousSelectedTerminal = m_selectedVariableTerminal;

    m_variableTerminals.clear();
    managedVariablesList->clear();
    m_selectedVariableTerminal = nullptr;
    {
        const QSignalBlocker blocker(variablePaneComboBox);
        variablePaneComboBox->clear();

        if (m_tabs &&
            m_variableTabIndex >= 0 &&
            m_variableTabIndex < m_tabs->count())
        {
            auto *holder = qobject_cast<TermWidgetHolder *>(
                m_tabs->widget(m_variableTabIndex));

            if (holder)
            {
                QList<TermWidget *> terminals =
                    holder->findChildren<TermWidget *>();

                // Sort panes by their on-screen position.
                std::sort(
                    terminals.begin(),
                    terminals.end(),
                    [holder](TermWidget *a, TermWidget *b)
                    {
                        const QPoint aPos = a->mapTo(holder, QPoint(0, 0));
                        const QPoint bPos = b->mapTo(holder, QPoint(0, 0));

                        if (aPos.x() != bPos.x())
                            return aPos.x() < bPos.x();

                        return aPos.y() < bPos.y();
                    });

                int paneNumber = 0;

                for (TermWidget *terminal : terminals)
                {
                    if (!terminal || !terminal->impl())
                        continue;

                    ++paneNumber;
                    m_variableTerminals.append(terminal);

                    variablePaneComboBox->addItem(
                        tr("Pane %1").arg(paneNumber));

                    connect(
                        terminal->impl(),
                        &TermWidgetImpl::managedVariablesResponse,
                        this,
                        &BookmarksWidget::handleManagedVariablesResponse,
                        Qt::UniqueConnection);

                    connect(
                        terminal->impl(),
                        &TermWidgetImpl::managedVariablesChanged,
                        this,
                        &BookmarksWidget::handleManagedVariablesChanged,
                        Qt::UniqueConnection);
                }

                const int selectedIndex =
                    m_variableTerminals.indexOf(
                        holder->currentTerminal());

                if (selectedIndex >= 0)
                    variablePaneComboBox->setCurrentIndex(selectedIndex);
                else if (!m_variableTerminals.isEmpty())
                    variablePaneComboBox->setCurrentIndex(0);
            }
        }
    }

    if (m_variableTerminals.isEmpty())
    {
        m_selectedVariableTerminal = nullptr;
        managedVariablesList->clear();
        return;
    }

    const int index = variablePaneComboBox->currentIndex();

    if (index >= 0 && index < m_variableTerminals.size())
    {
        m_selectedVariableTerminal = m_variableTerminals.at(index);

        if (m_selectedVariableTerminal != previousSelectedTerminal)
            requestManagedVariables(m_selectedVariableTerminal);
    }
}

void BookmarksWidget::requestManagedVariables(TermWidget *terminal)
{
    if (!terminal || !terminal->impl())
        return;

    m_selectedVariableTerminal = terminal;

    QJsonObject request;
    request.insert(QStringLiteral("op"), QStringLiteral("list"));

    terminal->impl()->enqueueManagedVariableRequest(request);
}

BookmarksWidget::~BookmarksWidget() = default;

void BookmarksWidget::setup()
{
    m_model->setup();

    treeView->setRootIndex(m_model->index(0, 0));
    treeView->expandAll();
    treeView->resizeColumnToContents(0);
    treeView->resizeColumnToContents(1);
}

void BookmarksWidget::handleCommand(const QModelIndex &index)
{
    auto *item =
        static_cast<AbstractBookmarkItem *>(index.internalPointer());

    if (!item || item->type() != AbstractBookmarkItem::Command)
        return;

    emit callCommand(item->value() + QLatin1Char('\n'));
}

static bool filterBookmarkBranch(
    QTreeView *treeView,
    const BookmarksModel *model,
    const QModelIndex &parent,
    const QString &filterText,
    bool revealAll,
    bool &containsMatch)
{
    bool anyVisible = false;
    containsMatch = false;

    for (int row = 0; row < model->rowCount(parent); ++row)
    {
        const QModelIndex index = model->index(row, 0, parent);

        auto *item =
            static_cast<AbstractBookmarkItem *>(index.internalPointer());

        if (!item)
        {
            treeView->setRowHidden(row, parent, true);
            continue;
        }

        const bool isGroup =
            item->type() == AbstractBookmarkItem::Group;

        const bool selfMatches =
            item->display().contains(filterText, Qt::CaseInsensitive)
            || (!isGroup
                && item->value().contains(filterText, Qt::CaseInsensitive));

        const bool revealChildren = revealAll || (isGroup && selfMatches);

        bool descendantsMatch = false;

        if (isGroup)
        {
            filterBookmarkBranch(
                treeView, model, index, filterText,
                revealChildren, descendantsMatch);
        }

        const bool visible =
            revealAll || selfMatches || descendantsMatch;

        treeView->setRowHidden(row, parent, !visible);

        if (isGroup && !revealAll && (selfMatches || descendantsMatch))
            treeView->expand(index);

        anyVisible = anyVisible || visible;
        containsMatch = containsMatch || selfMatches || descendantsMatch;
    }

    return anyVisible;
}

void BookmarksWidget::filter(const QString &str)
{
    treeView->clearSelection();

    bool containsMatch = false;

    filterBookmarkBranch(
        treeView,
        m_model,
        treeView->rootIndex(),
        str,
        str.isEmpty(),
        containsMatch);
}

void BookmarksWidget::handleManagedVariablesResponse(
    const QString &response)
{
    auto *source = qobject_cast<TermWidgetImpl *>(sender());

    if (!m_selectedVariableTerminal ||
        !m_selectedVariableTerminal->impl() ||
        source != m_selectedVariableTerminal->impl())
    {
        return;
    }

    QJsonParseError parseError;
    const QJsonDocument document =
        QJsonDocument::fromJson(response.toUtf8(), &parseError);

    if (parseError.error != QJsonParseError::NoError ||
        !document.isObject())
    {
        qWarning() << "Invalid managed-variable response:"
                   << parseError.errorString();
        return;
    }

    const QJsonObject object = document.object();

    if (object.value(QStringLiteral("op")).toString()
        != QStringLiteral("result"))
    {
        return;
    }

    const QJsonValue variablesValue =
        object.value(QStringLiteral("variables"));

    if (!variablesValue.isArray())
    {
        qWarning() << "Managed-variable response has no variables array";
        return;
    }

    managedVariablesList->clear();

    int variableNumber = 1;

    for (const QJsonValue &value : variablesValue.toArray())
    {
        if (!value.isString())
            continue;

        const QString variable = value.toString();

        if (!variable.isEmpty())
        {
            managedVariablesList->addItem(
                QStringLiteral("%1. %2").arg(variableNumber++).arg(variable));
        }
    }
}

void BookmarksWidget::handleManagedVariablesChanged()
{
    auto *source = qobject_cast<TermWidgetImpl *>(sender());

    if (!source)
        return;

    for (TermWidget *terminal : m_variableTerminals)
    {
        if (!terminal || !terminal->impl())
            continue;

        if (terminal->impl() == source)
        {
            if (terminal == m_selectedVariableTerminal)
                requestManagedVariables(terminal);

            return;
        }
    }
}
