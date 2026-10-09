/**
 * SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#pragma once

#include <functional>

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

    // Undo/redo history (see ScenarioHistory)
    Q_PROPERTY(bool canUndo READ canUndo NOTIFY historyChanged)
    Q_PROPERTY(bool canRedo READ canRedo NOTIFY historyChanged)
    Q_PROPERTY(QString undoText READ undoText NOTIFY historyChanged)
    Q_PROPERTY(QString redoText READ redoText NOTIFY historyChanged)
    Q_PROPERTY(bool modified READ modified NOTIFY historyChanged)
    
    // Metabox: a scenario exposing inputs or outputs can be saved as a metabox and used as a box in other scenarios.
    // The metadata below is stored as scenario attributes and shown in the box palette once the metabox is saved.
    Q_PROPERTY(bool isMetabox READ isMetabox NOTIFY isMetaboxChanged)
    Q_PROPERTY(QString metaboxId READ metaboxId NOTIFY metaboxMetadataChanged)
    Q_PROPERTY(QString metaboxName READ metaboxName WRITE setMetaboxName NOTIFY metaboxMetadataChanged)
    Q_PROPERTY(QString author READ author WRITE setAuthor NOTIFY metaboxMetadataChanged)
    Q_PROPERTY(QString company READ company WRITE setCompany NOTIFY metaboxMetadataChanged)
    Q_PROPERTY(QString category READ category WRITE setCategory NOTIFY metaboxMetadataChanged)
    Q_PROPERTY(QString version READ version WRITE setVersion NOTIFY metaboxMetadataChanged)
    Q_PROPERTY(QString shortDescription READ shortDescription WRITE setShortDescription NOTIFY metaboxMetadataChanged)
    Q_PROPERTY(QString detailedDescription READ detailedDescription WRITE setDetailedDescription NOTIFY metaboxMetadataChanged)


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

    // ---- Undo / redo ----
    Q_INVOKABLE void undo();
    Q_INVOKABLE void redo();
    /**
     * Records the current scenario state as the outcome of a user action. Synchronous; QML uses it
     * at the end of a drag. C++ mutation paths prefer requestCommit() so that a cascade of kernel
     * callbacks inside one event-loop turn yields a single history entry.
     */
    Q_INVOKABLE void commitChange(const QString& label);
    Q_INVOKABLE void removeSelection(const QStringList& boxIds, const QStringList& commentIds);

    // ---- Clipboard (system clipboard, see ScenarioClipboard) ----
    Q_INVOKABLE void copySelection(const QStringList& boxIds, const QStringList& commentIds);
    Q_INVOKABLE void cutSelection(const QStringList& boxIds, const QStringList& commentIds);
    /** Pastes at `anchor` (scene coordinates) and returns the identifiers of the pasted boxes. */
    Q_INVOKABLE QStringList pasteFromClipboard(const QPoint& anchor);

    /**
     * Deferred commitChange(): (re)starts a single-shot timer; the last requested label names the step.
     * `delayMs` > 0 debounces continuous edits (setting sliders, typing).
     */
    void requestCommit(const QString& label, int delayMs = 0);
    void flushPendingCommit();
    /** Marks the current state as the saved one (clears the dirty marker). */
    void markSaved();

    bool canUndo() const;
    bool canRedo() const;
    QString undoText() const;
    QString redoText() const;
    bool modified() const;
    /**
     * @brief Gives the scenario a new random metabox identifier.
     * Useful when a metabox file is copied to become a new, distinct metabox.
     */
    Q_INVOKABLE void regenerateMetaboxId();

    /**
     * @brief Makes sure the scenario carries the attributes the metabox manager requires to load it as a metabox:
     * a valid metabox identifier (random one if missing) and a non-empty name (the scenario name if missing).
     * Called before saving the scenario as a metabox.
     */
    void ensureMetaboxAttributes();

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

    bool isMetabox() const;
    QString metaboxId() const;
    QString metaboxName() const;
    QString author() const;
    QString company() const;
    QString category() const;
    QString version() const;
    QString shortDescription() const;
    QString detailedDescription() const;

    void setMetaboxName(const QString& value);
    void setAuthor(const QString& value);
    void setCompany(const QString& value);
    void setCategory(const QString& value);
    void setVersion(const QString& value);
    void setShortDescription(const QString& value);
    void setDetailedDescription(const QString& value);

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
    void isMetaboxChanged();
    void metaboxMetadataChanged();

    /**
     * The QML wrappers (boxes, links, comments, ports...) are about to be destroyed and rebuilt:
     * drop any reference to them (BoxSettings panel, selection...).
     */
    void aboutToReset();
    /** The wrappers have been rebuilt from the kernel scenario; identifiers are unchanged. */
    void modelReset();
    void historyChanged();

protected:
    /** Replaces the kernel scenario content with `snapshot` (compressed XML) and rebuilds the wrappers. */
    bool restoreState(const QByteArray& snapshot);
    /**
     * Destroys every wrapper, runs `kernelMutation` (optional) on the bare kernel scenario, then
     * recreates the wrappers and notifies QML.
     */
    void rebuildModel(const std::function<void()>& kernelMutation = {});
    void notifyModelReset();
    QByteArray captureState() const;
    Q_SLOT void onScenarioParameterEdited();

    class DesignerScenarioPrivate *d;
};


}
}
