/**
 * SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/ 

#pragma once

#include <ov_common_defines.h>
#include <openvibe/CIdentifier.hpp>

namespace OpenViBE {
namespace Kernel {

/**
 * Class CScenarioParameterConnection
 * 
 * This class represents a link between a parameter of the scenario and a box parameter.
 * 
 * The attributes it holds are:
 * - scenarioParameterName: the name of the scenario parameter
 * - boxID: the ID of a box
 * - boxParameterName: the name of a parameter of the box
 * - connectionID: the ID of the parameter connection. This value is provided by the ParameterTraits::connect() method,
 *                    and it is used to cancel the connection if needed.
 */
class CScenarioParameterConnection
{
public:
    CScenarioParameterConnection(const std::string& scenarioParameterName, const CIdentifier& boxID, const std::string& boxParameterName):
        m_scenarioParameterName(scenarioParameterName),
        m_boxID(boxID),
        m_boxParameterName(boxParameterName) { }
    CScenarioParameterConnection(const CScenarioParameterConnection& connection):
        m_scenarioParameterName(connection.getScenarioParameterName()),
        m_boxID(connection.getBoxID()),
        m_boxParameterName(connection.getBoxParameterName()) { }
        
    ~CScenarioParameterConnection() { }

    const std::string& getScenarioParameterName() const { return m_scenarioParameterName; }
    const CIdentifier& getBoxID() const { return m_boxID; }
    const std::string& getBoxParameterName() const { return m_boxParameterName; }

    void setScenarioParameterName(const std::string& scenarioParameterName) { m_scenarioParameterName = scenarioParameterName; }
    void setBoxID(const CIdentifier& boxID) { m_boxID = boxID; }
    void setBoxParameterName(const std::string& boxParameterName) { m_boxParameterName = boxParameterName; }

private:
    std::string m_scenarioParameterName;
    CIdentifier m_boxID;
    std::string m_boxParameterName;
};

}  // namespace Kernel
}  // namespace OpenViBE