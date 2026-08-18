/**
 * SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#pragma once

#include <openvibe/ov_all.h>
#include <QtCore>
#include <QtQml>

#include "BoxObject.h"
#include "CommentObject.h"
#include "LinkObject.h"
#include "ScenarioParameterConnectionObject.h"
#include "IOPort.h"
#include "ScenarioIOConnection.h"
#include "WindowManager.h"

namespace OpenViBE {
namespace Designer {

class PlayerBridge;

/**
 * @brief The DesignerScenario class is a wrapper of a Kernel::IScenario for the GUI.
 * It holds a list of boxes and links that are displayed in the GUI.
 * It is a QObject that can be accessed from QML.
*/
class DesignerScenario : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(QUrl filePath READ filePath WRITE setFilePath NOTIFY filePathChanged)

    Q_PROPERTY(QQmlListProperty<BoxObject> boxes READ boxes NOTIFY boxesChanged)
    Q_PROPERTY(QQmlListProperty<LinkObject> links READ links NOTIFY linksChanged)

    Q_PROPERTY(QStringList availableParameterTypes READ getAvailableParameterTypes NOTIFY parameterTypesChanged)
    Q_PROPERTY(QJSValue parameters READ parameters NOTIFY parametersChanged)

    Q_PROPERTY(QQmlListProperty<CommentObject> comments READ comments NOTIFY commentsChanged)

    Q_PROPERTY(QQmlListProperty<IOPort> inputPorts READ inputPorts NOTIFY inputPortsChanged)
    Q_PROPERTY(QQmlListProperty<IOPort> outputPorts READ outputPorts NOTIFY outputPortsChanged)

    Q_PROPERTY(QQmlListProperty<ScenarioIOConnection> scenarioIOConnections READ scenarioIOConnections NOTIFY scenarioIOConnectionsChanged)

    Q_PROPERTY(WindowManager* windowManager READ windowManager NOTIFY windowManagerChanged)

    Q_PROPERTY(double cpuUsage READ cpuUsage NOTIFY cpuUsageChanged)


public:
    DesignerScenario(Kernel::IKernelContext* context, Kernel::IScenario& scenario, const CIdentifier& scenarioID);
    ~DesignerScenario();

    Q_INVOKABLE void addBox(const QString& boxName, const QPoint& position);
    Q_INVOKABLE void addComment(const QString& text, const QPoint& origin);
    Q_INVOKABLE void clickPort(int portIdx,const QString& boxId, qreal x, qreal y, bool isOutput);
    Q_INVOKABLE void removeLinkOnPort(int portIdx, QString boxId, bool isOutput);
    Q_INVOKABLE void removeLink(int linkIdx);
    Q_INVOKABLE void removeBox(QString identifier);
    Q_INVOKABLE void removeComment(int commentIdx);
    Q_INVOKABLE void stopLinkCreation();
    Q_INVOKABLE void addParameter(const QString& name, const QString& type);
    Q_INVOKABLE void removeParameter(const QString& name);
    Q_INVOKABLE void updateParameterName(const QString& oldName, const QString& newName);
    Q_INVOKABLE QQmlListProperty<ScenarioParameterConnectionObject> getParameterConnectionsForParam(const QString& scenarioParameterName);
    Q_INVOKABLE void updateParameterConnection(const QString& scenarioParameterName, const QString& boxIdentifier, const QString& boxParameterName, bool connect);
    
    Q_INVOKABLE QList<QString> getSupportedInputTypes() const;
    Q_INVOKABLE void addInput(const QString& name, const QString& typeId);
    Q_INVOKABLE void removeInput(int portIndex);

    Q_INVOKABLE QList<QString> getSupportedOutputTypes() const;
    Q_INVOKABLE void addOutput(const QString& name, const QString& typeId);
    Q_INVOKABLE void removeOutput(int portIndex);

    Q_INVOKABLE void setScenarioInputLink(int portIndex, const QString& boxId, int boxInputIndex);
    Q_INVOKABLE void setScenarioOutputLink(int portIndex, const QString& boxId, int boxOutputIndex);
    Q_INVOKABLE void removeScenarioIOConnection(int portIndex, const QString& boxId, int boxPortIndex, bool isOutput);


    Kernel::IScenario& scenario() const;
    const CIdentifier& id() const;

    //create a player if it does not exist
    PlayerBridge& createOrGetPlayer();

    PlayerBridge& currentPlayer();
    void releasePlayer();

    bool isEmpty();
    uint64_t m_lastLoopTime = uint64_t(-1);

    QString name() const;
    QUrl filePath() const;
    QQmlListProperty<BoxObject> boxes();
    QQmlListProperty<CommentObject> comments();
    QQmlListProperty<LinkObject> links();
    QStringList getAvailableParameterTypes() const;
    QJSValue parameters() const;
    QQmlListProperty<IOPort> inputPorts();
    QQmlListProperty<IOPort> outputPorts();
    QQmlListProperty<ScenarioIOConnection> scenarioIOConnections();
    WindowManager* windowManager();
    double cpuUsage() const;

    void setName(const QString &name);
    void setFilePath(const QUrl &filePath);

signals:
    void boxesChanged();
    void commentsChanged();
    void linksChanged();
    void nameChanged(const QString&);
    void filePathChanged(const QUrl&);
    void startLinkCreation(qreal x, qreal y);
    void finishLinkCreation();
    void parameterTypesChanged();
    void parametersChanged();
    void newComment();
    void inputPortsChanged();
    void outputPortsChanged();
    void scenarioIOConnectionsChanged();
    void windowManagerChanged();
    void cpuUsageChanged();


protected:
    class DesignerScenarioPrivate *d;
};


}
}