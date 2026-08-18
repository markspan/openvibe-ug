/**
 * SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/

/**
 * Class ScenarioParameterConnection
 * 
 * This class stores the information of a link between a parameter of the scenario and a box parameter.
 * It is a Q_OBJECT as it is used in QML.
 * 
 * The properties are:
 * - scenarioParameterName: the name of the scenario parameter
 * - boxID: the ID of a box
 * - boxParamName: the name of a parameter of the box
 * - connectionID: the ID of the parameter connection. This value is provided by the ParameterTraits::connect() method,
 *                    and it is used to cancel the connection if needed.
 */

#pragma once

#include <openvibe/ov_all.h>
#include <QtCore>

namespace OpenViBE {  
namespace Designer {

class ScenarioParameterConnection : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString scenarioParameterName READ scenarioParameterName WRITE setScenarioParameterName NOTIFY scenarioParameterNameChanged)
    Q_PROPERTY(QString boxID READ boxID WRITE setBoxID NOTIFY boxIDChanged)
    Q_PROPERTY(QString boxParamName READ boxParamName WRITE setBoxParamName NOTIFY boxParamNameChanged)
    Q_PROPERTY(uint64_t connectionID READ connectionID WRITE setconnectionID NOTIFY connectionIDChanged)

signals:
    void scenarioParameterNameChanged();
    void boxIDChanged();
    void boxParamNameChanged();
    void connectionIDChanged();

public:
    ScenarioParameterConnection(Kernel::IKernelContext* context, const QString& scenarioParameterName, const QString& boxID, const QString& boxParamName, uint64_t checkedIndex);
    ~ScenarioParameterConnection() {};

    QString scenarioParameterName() const;
    QString boxID() const;
    QString boxParamName() const;
    uint64_t connectionID() const;

    void setScenarioParameterName(const QString& scenarioParameterName);
    void setBoxID(const QString& boxID);
    void setBoxParamName(const QString& boxParamName);
    void setconnectionID(uint64_t connectionID);

protected:
    Kernel::IKernelContext* m_context = nullptr;
    QString m_scenarioParameterName = "";
    QString m_boxID = "";
    QString m_boxParamName = "";
    uint64_t m_connectionID = 0;
};

}  // Designer
}  // OpenViBE