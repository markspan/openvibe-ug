/**
 * SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
 * see : https://doc.qt.io/qt-6.5/qtwidgets-itemviews-simpletreemodel-example.html
*/

#include "BoxTreeItem.h"

namespace OpenViBE {  
namespace Designer {

BoxTreeItem::BoxTreeItem(const QList<QVariant> &data, BoxTreeItem *parent)
    : m_itemData(data), m_parentItem(parent)
{}

BoxTreeItem::~BoxTreeItem()
{
    qDeleteAll(m_childItems);
}

void BoxTreeItem::appendChild(BoxTreeItem *item)
{
    m_childItems.append(item);
}

BoxTreeItem *BoxTreeItem::child(int row)
{
    if (row < 0 || row >= m_childItems.size()) {
        return nullptr;
    }
    return m_childItems.at(row);
}

int BoxTreeItem::childCount() const
{
    return m_childItems.count();
}

int BoxTreeItem::row() const
{
    if (m_parentItem) {
        return m_parentItem->m_childItems.indexOf(const_cast<BoxTreeItem*>(this));
    }
    return 0;
}

int BoxTreeItem::columnCount() const
{
    return m_itemData.count();
}

QVariant BoxTreeItem::data(int column) const
{
    if (column < 0 || column >= m_itemData.size()) {
        return QVariant();
    }
    return m_itemData.at(column);
}

BoxTreeItem *BoxTreeItem::parentItem()
{
    return m_parentItem;
}

}
}