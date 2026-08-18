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

class ScenarioParameterConnectionObject : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString scenarioParameterName READ scenarioParameterName WRITE setScenarioParameterName NOTIFY scenarioParameterNameChanged)
    Q_PROPERTY(QString boxID READ boxID WRITE setBoxID NOTIFY boxIDChanged)
    Q_PROPERTY(QString boxParamName READ boxParamName WRITE setBoxParamName NOTIFY boxParamNameChanged)
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)

signals:
    void scenarioParameterNameChanged();
    void boxIDChanged();
    void boxParamNameChanged();
    void connectedChanged();

public:
    ScenarioParameterConnectionObject(Kernel::IKernelContext* context, const QString& scenarioParameterName, const QString& boxID, const QString& boxParamName, bool connected);
    ~ScenarioParameterConnectionObject() {};

    QString scenarioParameterName() const;
    QString boxID() const;
    QString boxParamName() const;
    bool connected() const;

    void setScenarioParameterName(const QString& scenarioParameterName);
    void setBoxID(const QString& boxID);
    void setBoxParamName(const QString& boxParamName);
    void setConnected(bool connected);

protected:
    Kernel::IKernelContext* m_context = nullptr;
    Kernel::CScenarioParameterConnection m_scenarioParameterConnection;
    bool m_connected = false;
};

}  // Designer
}  // OpenViBE