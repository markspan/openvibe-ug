// SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include <openvibe/ov_all.h>
#include <QtCore>

namespace OpenViBE {  
namespace Designer {

class BoxObject;
class IOPort;

class ScenarioIOConnection : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QPoint targetPosition READ targetPosition NOTIFY targetPositionChanged)
    Q_PROPERTY(bool isOutput READ isOutput CONSTANT)
    Q_PROPERTY(QString dataType READ dataType CONSTANT)

    Q_PROPERTY(int scenarioPortIdx READ scenarioPortIdx CONSTANT)
    Q_PROPERTY(QString targetBoxId READ targetBoxId CONSTANT)
    Q_PROPERTY(int targetPortIdx READ targetPortIdx CONSTANT)

signals:
    void targetPositionChanged();

public:
    ScenarioIOConnection(Kernel::IKernelContext* context, IOPort* scenarioPort, BoxObject *targetBox, int targetPortIdx, bool isOutput);
    virtual ~ScenarioIOConnection();

    QPoint targetPosition() const;
    bool isOutput() const;
    QString dataType() const;

    int scenarioPortIdx() const;
    QString targetBoxId() const;
    int targetPortIdx() const;

protected:
    friend class DesignerScenario;
    Kernel::IKernelContext* m_context = nullptr;
    IOPort* m_scenarioPort = nullptr;
    BoxObject *m_targetBox = nullptr;
    int m_targetPortIdx = 0;
    bool m_isOutput = false;
};

}
}