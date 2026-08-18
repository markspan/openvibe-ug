// SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

#include "ScenarioIOConnection.h"

#include "BoxObject.h"
#include "IOPort.h"

namespace OpenViBE {
namespace Designer {

ScenarioIOConnection::ScenarioIOConnection(Kernel::IKernelContext* context, IOPort* scenarioPort, BoxObject *targetBox, int targetPortIdx, bool isOutput)
    : m_context(context),
      m_scenarioPort(scenarioPort),
      m_targetBox(targetBox),
      m_targetPortIdx(targetPortIdx),
      m_isOutput(isOutput)
{
  
  connect(m_targetBox, &BoxObject::centerChanged, this, &ScenarioIOConnection::targetPositionChanged);
  connect(m_targetBox, &BoxObject::portOffsetsChanged, this, &ScenarioIOConnection::targetPositionChanged);
}

ScenarioIOConnection::~ScenarioIOConnection()
{
}

QPoint ScenarioIOConnection::targetPosition() const
{
    QPointF offset;
    if (m_isOutput){
      offset = m_targetBox->m_outputPortOffsets.at(m_targetPortIdx);
    } else {
      offset = m_targetBox->m_inputPortOffsets.at(m_targetPortIdx);
    }
    return m_targetBox->center() + offset.toPoint();
}

bool ScenarioIOConnection::isOutput() const
{
  return m_isOutput;
}

QString ScenarioIOConnection::dataType() const
{
    return m_scenarioPort->dataType();
}

int ScenarioIOConnection::scenarioPortIdx() const
{
    return m_scenarioPort->index();
}

QString ScenarioIOConnection::targetBoxId() const
{
    return m_targetBox->identifier();
}

int ScenarioIOConnection::targetPortIdx() const
{
    return m_targetPortIdx;
}

}
}