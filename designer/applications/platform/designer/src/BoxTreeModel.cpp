/**
 * SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
 * see : https://doc.qt.io/qt-6.5/qtwidgets-itemviews-simpletreemodel-example.html
*/

#include "BoxTreeItem.h"
#include "BoxTreeModel.h"

namespace OpenViBE {  
namespace Designer {

BoxTreeModel::BoxTreeModel(QObject *parent)
    : QAbstractItemModel(parent)
{
    QList<QVariant> headerData = {"Name", "Author", "Description", "boxAlgorithmClassID"};
    rootItem = new BoxTreeItem(headerData);
}

void BoxTreeModel::addData( const QString& name, const QString& author, const QString& category, const QString& description, const QString &boxAlgorithmClassID)
{
    BoxTreeItem *parent = rootItem;

    if(!m_categories.contains(category)) {
        //new category
        m_categories.append(category);

        //find the parent: category are in the form toto/tata/titi
        //start with root and go down finding or creating children
        QStringList categoryList = category.split("/");    
        for(const QString &categoryName : categoryList) {
            BoxTreeItem *child = nullptr;
            for(int i = 0; i < parent->childCount(); ++i) {
                if(parent->child(i)->data(0).toString() == categoryName) {
                    child = parent->child(i);
                    break;
                }
            }

            if(child == nullptr) {
                //the category does not exist, let's create it
                QList<QVariant> categoryData = {categoryName, "", "", ""};
                child = new BoxTreeItem(categoryData, parent);
                parent->appendChild(child);
            }

            parent = child;
        }
    } else {
        // the category exist, let's find the corresponding parent
        QStringList categoryList = category.split("/");    
        for(const QString &categoryName : categoryList) {
            for(int i = 0; i < parent->childCount(); ++i) {
                if(parent->child(i)->data(0).toString() == categoryName) {
                    parent = parent->child(i);
                    break;
                }
            }
        }
    }

    //fill the columns data : name, author, description, boxAlgorithmClassID
    //and add the boxItem to the parent
    QList<QVariant> pluginData = {name, author, description, boxAlgorithmClassID};
    BoxTreeItem *boxItem = new BoxTreeItem(pluginData, parent);
    parent->appendChild(boxItem);
}

BoxTreeModel::~BoxTreeModel()
{
    delete rootItem;
}


QHash<int, QByteArray> BoxTreeModel::roleNames() const
{
    QHash<int, QByteArray> roles;
    roles[Qt::DisplayRole] = "display";
    roles[Qt::ToolTipRole] = "tooltip";
    roles[DragRole] = "drag";
    return roles;
}


QModelIndex BoxTreeModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent)) {
        return QModelIndex();
    }

    BoxTreeItem *parentItem;

    if (!parent.isValid()) {
        parentItem = rootItem;
    } else {
        parentItem = static_cast<BoxTreeItem*>(parent.internalPointer());
    }

    BoxTreeItem *childItem = parentItem->child(row);
    if (childItem) {
        return createIndex(row, column, childItem);
    }

    return QModelIndex();
}

QModelIndex BoxTreeModel::parent(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return QModelIndex();
    }

    BoxTreeItem *childItem = static_cast<BoxTreeItem*>(index.internalPointer());
    BoxTreeItem *parentItem = childItem->parentItem();

    if (parentItem == rootItem) {
        return QModelIndex();
    }

    return createIndex(parentItem->row(), 0, parentItem);
}

int BoxTreeModel::rowCount(const QModelIndex &parent) const
{
    BoxTreeItem *parentItem;
    if (parent.column() > 0){
        return 0;
    }

    if (!parent.isValid()) {
        parentItem = rootItem;
    } else {
        parentItem = static_cast<BoxTreeItem*>(parent.internalPointer());
    }

    return parentItem->childCount();
}

int BoxTreeModel::columnCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return static_cast<BoxTreeItem*>(parent.internalPointer())->columnCount();
    }
    return rootItem->columnCount();
}

QVariant BoxTreeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid()) {
        return QVariant();
    }

    BoxTreeItem *item = static_cast<BoxTreeItem*>(index.internalPointer());
    QString tooltip;

    switch (role) {
        case Qt::DisplayRole:
            return item->data(index.column());
        case Qt::ToolTipRole:
            tooltip = item->data(2).toString() + " made By " + item->data(1).toString();
            return tooltip;
        case DragRole:
            return item->data(3); // boxAlgorithmClassID
        default:
            break;
    }

    return QVariant();
}

Qt::ItemFlags BoxTreeModel::flags(const QModelIndex &index) const
{
    if (!index.isValid()) {
        return Qt::NoItemFlags;
    }

    return QAbstractItemModel::flags(index);
}

QVariant BoxTreeModel::headerData(int section, Qt::Orientation orientation,
                               int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        return rootItem->data(section);
    }
    
    return QVariant();
}

int BoxTreeModel::entryCount( const QModelIndex & index ) const
{
    int count = 0;
    int rowCount = this->rowCount(index);
    count += rowCount;
    for( int r = 0; r < rowCount; ++r ) {
        count += entryCount( this->index(r,0,index) );
    }
    
    return count;
}
}
}