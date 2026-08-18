/**
 * SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#include <QtCore>

namespace OpenViBE {  
namespace Designer {

class BoxTreeItem
{
public:
    //Data are {"Name", "Author", "Description", "boxAlgorithmClassID"};
    explicit BoxTreeItem(const QList<QVariant> &data, BoxTreeItem *parentItem = nullptr);
    ~BoxTreeItem();

    void appendChild(BoxTreeItem *child);

    BoxTreeItem *child(int row);
    int childCount() const;
    int columnCount() const;
    QVariant data(int column) const;
    int row() const;
    BoxTreeItem *parentItem();

private:
    QList<BoxTreeItem*> m_childItems;
    QList<QVariant> m_itemData;
    BoxTreeItem *m_parentItem;
};

}
}