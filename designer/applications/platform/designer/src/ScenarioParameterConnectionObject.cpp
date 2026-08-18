/**
 * SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#include "ScenarioParameterConnectionObject.h"

namespace OpenViBE {
namespace Designer {

    ScenarioParameterConnectionObject::ScenarioParameterConnectionObject(Kernel::IKernelContext* context, const QString& scenarioParameterName, const QString& boxID, const QString& boxParamName, bool connected)
    : m_context(context),
      m_scenarioParameterConnection(scenarioParameterName.toStdString(), CIdentifier(boxID.toStdString()), boxParamName.toStdString()),
      m_connected(connected)
{
}

QString ScenarioParameterConnectionObject::scenarioParameterName() const
{
    return m_scenarioParameterConnection.getScenarioParameterName().c_str();
}

QString ScenarioParameterConnectionObject::boxID() const
{
    return QString::fromStdString(m_scenarioParameterConnection.getBoxID().str());
}

QString ScenarioParameterConnectionObject::boxParamName() const
{
    return m_scenarioParameterConnection.getBoxParameterName().c_str();
}

void ScenarioParameterConnectionObject::setScenarioParameterName(const QString& scenarioParameterName)
{
    if (m_scenarioParameterConnection.getScenarioParameterName().c_str() == scenarioParameterName) {
        return;
    }
    m_scenarioParameterConnection.setScenarioParameterName(scenarioParameterName.toStdString());
    emit scenarioParameterNameChanged();
}

void ScenarioParameterConnectionObject::setBoxID(const QString& boxID)
{
    if (m_scenarioParameterConnection.getBoxID().str() == boxID.toStdString()) {
        return;
    }
    m_scenarioParameterConnection.setBoxID(CIdentifier(boxID.toStdString()));
    emit boxIDChanged();
}

void ScenarioParameterConnectionObject::setBoxParamName(const QString& boxParamName)
{
    if (m_scenarioParameterConnection.getBoxParameterName().c_str() == boxParamName) {
        return;
    }
    m_scenarioParameterConnection.setBoxParameterName(boxParamName.toStdString());
    emit boxParamNameChanged();
}

bool ScenarioParameterConnectionObject::connected() const
{
    return m_connected;
}

void ScenarioParameterConnectionObject::setConnected(bool connected)
{
    if (this->m_connected == connected) {
        return;
    }
    this->m_connected = connected;
    emit connectedChanged();
}

}  // Designer
}  // OpenViBE