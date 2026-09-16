/**
 * SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#include <functional>

#include <QtCore>

#pragma once

namespace OpenViBE {  
namespace Designer {

class BoxTreeItem;

/**
 * @brief The BoxTreeModel class is a tree model of all the available boxes.
 * It is a QAbstractItemModel that can be accessed from QML.
 * the name of the box will be displayed
 * The author and description are used as tooltips
 * The category is used to sort the boxes
 * The boxAlgorithmClassID is used to create a box in the scenario
*/
class BoxTreeModel : public QAbstractItemModel
{
    Q_OBJECT
    friend class OVDesigner;
    friend struct OVDesignerPrivate;

public:
    BoxTreeModel(QObject *parent = nullptr);
    ~BoxTreeModel();

    /**
     * @brief Empties the tree and fills it again, notifying the views (and proxies) bound to the model.
     * @param populate Callback adding the entries through addData().
     */
    void rebuild(const std::function<void()>& populate);

    enum MyRoles {
        DragRole = Qt::UserRole + 1,
    };

    QHash<int, QByteArray> roleNames() const override;

    QVariant data(const QModelIndex &index, int role) const override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;
    QModelIndex index(int row, int column,
                      const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &index) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;

    int entryCount( const QModelIndex & index = QModelIndex()) const;

private:
    void addData(const QString& name, const QString& author, const QString& category, const QString& description, const QString &boxAlgorithmClassID);

    // columns of every item: name, author, description, boxAlgorithmClassID (drag payload)
    inline static const QList<QVariant> kHeaderData = {"Name", "Author", "Description", "boxAlgorithmClassID"};
    BoxTreeItem *rootItem = nullptr;
    QList<QString> m_categories;
};

}
}
