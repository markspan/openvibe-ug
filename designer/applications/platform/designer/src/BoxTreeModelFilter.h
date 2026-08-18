/**
 * SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#pragma once

#include <QtCore>

namespace OpenViBE {  
namespace Designer {

class BoxTreeModelFilter: public QSortFilterProxyModel {
    Q_OBJECT
public:
    BoxTreeModelFilter(QObject *parent = nullptr) : QSortFilterProxyModel(parent) {}
    ~BoxTreeModelFilter() = default;

protected:
    bool filterAcceptsColumn(int sourceColumn, const QModelIndex &sourceParent) const override {
        if(sourceColumn == 0) {
            return true;
        }
        return false;
    }
};

}
}