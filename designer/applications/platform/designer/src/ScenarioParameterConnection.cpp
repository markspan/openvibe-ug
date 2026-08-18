/**
 * SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#include "ScenarioParameterConnection.h"

namespace OpenViBE {
namespace Designer {

ScenarioParameterConnection::ScenarioParameterConnection(Kernel::IKernelContext* context, const QString& scenarioParameterName, const QString& boxID, const QString& boxParamName, uint64_t connectionID)
    : m_context(context)
    , m_scenarioParameterName(scenarioParameterName)
    , m_boxID(boxID)
    , m_boxParamName(boxParamName)
    , m_connectionID(connectionID)
{
}

QString ScenarioParameterConnection::scenarioParameterName() const
{
    return m_scenarioParameterName;
}

QString ScenarioParameterConnection::boxID() const
{
    return m_boxID;
}

QString ScenarioParameterConnection::boxParamName() const
{
    return m_boxParamName;
}

uint64_t ScenarioParameterConnection::connectionID() const
{
    return m_connectionID;
}

void ScenarioParameterConnection::setScenarioParameterName(const QString& scenarioParameterName)
{
    if (m_scenarioParameterName == scenarioParameterName) {
        return;
    }
    m_scenarioParameterName = scenarioParameterName;
    emit scenarioParameterNameChanged();
}

void ScenarioParameterConnection::setBoxID(const QString& boxID)
{
    if (m_boxID == boxID) {
        return;
    }
    m_boxID = boxID;
    emit boxIDChanged();
}

void ScenarioParameterConnection::setBoxParamName(const QString& boxParamName)
{
    if (m_boxParamName == boxParamName) {
        return;
    }
    m_boxParamName = boxParamName;
    emit boxParamNameChanged();
}

void ScenarioParameterConnection::setconnectionID(uint64_t connectionID)
{
    if (m_connectionID == connectionID) {
        return;
    }
    m_connectionID = connectionID;
    emit connectionIDChanged();
}

}  // Designer
}  // OpenViBE