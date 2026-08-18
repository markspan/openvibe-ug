// SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include <openvibe/ov_all.h>
#include <QtCore>
#include <QPointF>
#include <QJSValue>

namespace OpenViBE {  
namespace Designer {

class BoxObject;

class LinkObject : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QPoint sourcePosition READ sourcePosition NOTIFY sourcePositionChanged)
    Q_PROPERTY(QPoint targetPosition READ targetPosition NOTIFY targetPositionChanged)
    Q_PROPERTY(QString sourceDataType READ sourceDataType NOTIFY sourceDataTypeChanged)
    Q_PROPERTY(bool isLinked READ isLinked CONSTANT)

signals:
    void sourcePositionChanged();
    void targetPositionChanged();
    void sourceDataTypeChanged();

public:
    LinkObject(Kernel::IKernelContext* context, Kernel::ILink *link, BoxObject *sourceBox, BoxObject *targetBox);
    virtual ~LinkObject();

    QPoint sourcePosition() const;
    QPoint targetPosition() const;
    QString sourceDataType() const;
    bool isLinked() const;

protected:
    friend class DesignerScenario;
    Kernel::IKernelContext* m_context = nullptr;
    Kernel::ILink *m_link = nullptr;
    BoxObject *m_sourceBox = nullptr;
    BoxObject *m_targetBox = nullptr;
};

}
}