/**
 * SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#include <cstring>

#include <visualization-toolkit/ovviz_all.h>

#include "DesignerScenario.h"
#include "PlayerBridge.h"
#include "ParameterHelper.h"
#include "ScenarioClipboard.h"
#include "ScenarioHistory.h"
#include "ScenarioSerializer.h"
#include "defines.hpp"

#include "QmlEngineSingleton.h"
#include "WindowManager.h"

namespace OpenViBE {
namespace Designer {

class DesignerScenarioPrivate
{
public:
    DesignerScenarioPrivate(DesignerScenario* q, Kernel::IKernelContext* kernelCtx, Kernel::IScenario& scenario, CIdentifier scenarioID, PlayerBridge& player);
    ~DesignerScenarioPrivate();

    BoxObject* createBoxObject(Kernel::IBox* boxDetails);
    CommentObject* createCommentObject(Kernel::IComment* commentDetails);
    IOPort* createScenarioPort(IOPort::PortType type, int index);

    void fillBoxes();
    void fillLinks();
    void fillComments();
    void fillIOPorts();
    void fillScenarioIOConnections();
    void refillScenarioIOConnections();
    /** Creates every wrapper from the kernel scenario. */
    void fillModel();
    /** Destroys every wrapper. Must run before the kernel scenario is cleared: wrappers hold raw kernel pointers. */
    void clearModel();
    /** Drops every scenario-parameter connection through the public kernel API (see restoreState()). */
    void clearParameterConnections();
    void updateParameters();
    WindowManager* getWindowManager();
    QString attribute(const CIdentifier& attributeID) const;
    void setAttribute(const CIdentifier& attributeID, const QString& value);
    /** Sets a metabox metadata attribute from the UI: notifies QML and records the edit in the undo history. */
    void setMetadata(const CIdentifier& attributeID, const QString& value);

    DesignerScenario* q = nullptr;
    Kernel::IKernelContext* kernelCtx = nullptr;
    Kernel::IScenario& m_scenario;
    CIdentifier scenarioID = CIdentifier::undefined();
    CIdentifier m_windowManagerID = CIdentifier::undefined();
    VisualizationToolkit::IVisualizationContext* m_visualizationCtx = nullptr;
    PlayerBridge& player;
    QUrl filePath; // file path of the scenario to save or load

    CIdentifier playerID = CIdentifier::undefined();

    QList<BoxObject*> boxes;
    QMap<CIdentifier, BoxObject*> boxesMap;
    QList<LinkObject*> links;
    QList<CommentObject*> comments;
    Parameters::ParameterObjects m_parameters;
    QList<IOPort*> m_inputPorts;
    QList<IOPort*> m_outputPorts;
    QList<ScenarioIOConnection*> m_scenarioIOConnections;

    QList<ScenarioParameterConnectionObject*> m_parameterConnections; // Specific connections for a given scenario parameter

    bool linkCreationStarted = false;
    BoxObject* linkCreationSource = nullptr;
    int linkCreationSourcePort = -1;
    bool linkCreationSourceIsOutput = false;

    QTimer m_cpuUsageTimer;

    // Undo/redo: whole-scenario snapshots (compressed XML), see ScenarioHistory.
    ScenarioSerializer m_serializer;
    ScenarioHistory* m_history = nullptr;
    QByteArray m_lastState;        // snapshot matching the last recorded state
    QTimer m_commitTimer;          // deferred commit, see DesignerScenario::requestCommit()
    QString m_pendingLabel;
    bool m_restoring = false;      // no commit may be recorded while a snapshot is being applied
};

DesignerScenarioPrivate::DesignerScenarioPrivate(DesignerScenario* q, Kernel::IKernelContext* kernelCtx, Kernel::IScenario& scenario, CIdentifier scenarioID, PlayerBridge& player) :
    q(q),
    kernelCtx(kernelCtx),
    m_scenario(scenario),
    scenarioID(scenarioID),
    player(player),
    m_serializer(*kernelCtx)
{
}


DesignerScenarioPrivate::~DesignerScenarioPrivate()
{
    clearModel();
}

BoxObject* DesignerScenarioPrivate::createBoxObject(Kernel::IBox* boxDetails)
{
    BoxObject *box = new BoxObject(kernelCtx, boxDetails);
    QObject::connect(box, &BoxObject::edited, q, &DesignerScenario::requestCommit);
    boxesMap.insert(boxDetails->getIdentifier(), box);
    boxes.append(box);

    // Check if box is a display box
    auto pod = kernelCtx->getPluginManager().getPluginObjectDescCreating(boxDetails->getAlgorithmClassIdentifier());
    if (pod && pod->hasFunctionality(Plugins::EPluginFunctionality::Visualization))
    {
        getWindowManager()->addVisualizationBox(box);
    }
    return box;
}

CommentObject* DesignerScenarioPrivate::createCommentObject(Kernel::IComment* commentDetails)
{
    CommentObject *comment = new CommentObject(kernelCtx, commentDetails);
    QObject::connect(comment, &CommentObject::edited, q, &DesignerScenario::requestCommit);
    comments.append(comment);
    return comment;
}

IOPort* DesignerScenarioPrivate::createScenarioPort(IOPort::PortType type, int index)
{
    IOPort* port = new IOPort(kernelCtx, &m_scenario, type, index);
    QObject::connect(port, &IOPort::edited, q, &DesignerScenario::requestCommit);
    (type == IOPort::PortType::Input ? m_inputPorts : m_outputPorts).push_back(port);
    return port;
}

void DesignerScenarioPrivate::fillBoxes()
{
    // 1 add the boxes
    CIdentifier boxId = m_scenario.getNextBoxIdentifier(CIdentifier::undefined());
    while(boxId != CIdentifier::undefined())
    {
        // check isBox(boxId);
        createBoxObject(m_scenario.getBoxDetails(boxId));
        boxId = m_scenario.getNextBoxIdentifier(boxId);
    }
}

void DesignerScenarioPrivate::fillLinks()
{
    CIdentifier linkId = m_scenario.getNextLinkIdentifier(CIdentifier::undefined());
    while(linkId != CIdentifier::undefined())
    {
        // check isBox(boxId);

        Kernel::ILink *linkDetails = m_scenario.getLinkDetails(linkId);

        //get source and target BoxObject linked to this link
        CIdentifier sourceBoxId = linkDetails->getSourceBoxIdentifier();
        BoxObject *sourceBox = boxesMap.value(sourceBoxId);
        CIdentifier targetBoxId = linkDetails->getTargetBoxIdentifier();
        BoxObject *targetBox = boxesMap.value(targetBoxId);

        links.append(new LinkObject(kernelCtx, linkDetails, sourceBox, targetBox));
        linkId = m_scenario.getNextLinkIdentifier(linkId);
    }
}

void DesignerScenarioPrivate::fillComments()
{
    CIdentifier commentId = m_scenario.getNextCommentIdentifier(CIdentifier::undefined());
    while(commentId != CIdentifier::undefined())
    {
        createCommentObject(m_scenario.getCommentDetails(commentId));
        commentId = m_scenario.getNextCommentIdentifier(commentId);
    }
}

void DesignerScenarioPrivate::fillIOPorts()
{
    for(size_t i=0; i < m_scenario.getInputCount(); i++)
    {
        createScenarioPort(IOPort::PortType::Input, int(i));
    }
    for(size_t i=0; i < m_scenario.getOutputCount(); i++)
    {
        createScenarioPort(IOPort::PortType::Output, int(i));
    }
}

void DesignerScenarioPrivate::fillScenarioIOConnections()
{
    for(int i=0; i < m_inputPorts.size(); i++)
    {
        CIdentifier boxId;
        size_t boxInputIdx = 0;
        if(m_scenario.getScenarioInputLink(size_t(i), boxId, boxInputIdx) && boxesMap.contains(boxId))
        {
            m_scenarioIOConnections.append(new ScenarioIOConnection(kernelCtx, m_inputPorts.at(i), boxesMap.value(boxId), int(boxInputIdx), false));
        }
    }
    for(int i=0; i < m_outputPorts.size(); i++)
    {
        CIdentifier boxId;
        size_t boxOutputIdx = 0;
        if(m_scenario.getScenarioOutputLink(size_t(i), boxId, boxOutputIdx) && boxesMap.contains(boxId))
        {
            m_scenarioIOConnections.append(new ScenarioIOConnection(kernelCtx, m_outputPorts.at(i), boxesMap.value(boxId), int(boxOutputIdx), true));
        }
    }
}

void DesignerScenarioPrivate::refillScenarioIOConnections()
{
    qDeleteAll(m_scenarioIOConnections);
    m_scenarioIOConnections.clear();
    fillScenarioIOConnections();
}

void DesignerScenarioPrivate::fillModel()
{
    fillBoxes();
    fillLinks();
    fillComments();
    fillIOPorts();
    fillScenarioIOConnections();
    updateParameters();
}

void DesignerScenarioPrivate::clearModel()
{
    // Dependents first: links and IO connections dereference boxes and ports.
    qDeleteAll(links);
    links.clear();
    qDeleteAll(m_scenarioIOConnections);
    m_scenarioIOConnections.clear();
    qDeleteAll(m_parameterConnections);
    m_parameterConnections.clear();

    // The window manager is already gone when this runs from the destructor
    WindowManager* windowManager = getWindowManager();
    for(auto* box : boxes)
    {
        if (windowManager)
        {
            windowManager->removeVisualizationBox(box);
        }
        box->detachFromKernelBox();
    }
    qDeleteAll(boxes);
    boxes.clear();
    boxesMap.clear();

    qDeleteAll(comments);
    comments.clear();
    qDeleteAll(m_inputPorts);
    m_inputPorts.clear();
    qDeleteAll(m_outputPorts);
    m_outputPorts.clear();

    // Never keep ParameterObjects across a kernel clear(): they hold raw ParameterTraits pointers.
    qDeleteAll(m_parameters);
    m_parameters.clear();
}

void DesignerScenarioPrivate::clearParameterConnections()
{
    for(size_t i=0; i < m_scenario.getSettingCount(); i++)
    {
        CString name;
        m_scenario.getSettingName(i, name);
        std::vector<Kernel::CScenarioParameterConnection*> connections;
        m_scenario.getParameterConnections(name.toASCIIString(), connections);
        for(auto* connection : connections)
        {
            m_scenario.removeParameterConnection(connection->getScenarioParameterName(), connection->getBoxID(), connection->getBoxParameterName());
        }
    }
}

void DesignerScenarioPrivate::updateParameters()
{
    setUIParameters(kernelCtx, &m_scenario, m_parameters);
    // Scenario parameter values are written straight into the kernel by the ParameterObjects.
    const QMetaMethod slot = q->metaObject()->method(q->metaObject()->indexOfSlot("onScenarioParameterEdited()"));
    connectParameterValueChanged(m_parameters, q, slot);
}

QString DesignerScenarioPrivate::attribute(const CIdentifier& attributeID) const
{
    QString value;
    if(m_scenario.hasAttribute(attributeID))
    {
        value = m_scenario.getAttributeValue(attributeID).toASCIIString();
    }
    return value;
}

void DesignerScenarioPrivate::setAttribute(const CIdentifier& attributeID, const QString& value)
{
    const CString cValue = value.toStdString().c_str();
    if(m_scenario.hasAttribute(attributeID))
    {
        m_scenario.setAttributeValue(attributeID, cValue);
    }
    else
    {
        m_scenario.addAttribute(attributeID, cValue);
    }
}

void DesignerScenarioPrivate::setMetadata(const CIdentifier& attributeID, const QString& value)
{
    if(attribute(attributeID) != value)
    {
        setAttribute(attributeID, value);
        emit q->metaboxMetadataChanged();
        q->requestCommit(DesignerScenario::tr("Edit metabox metadata"));
    }
}

WindowManager* DesignerScenarioPrivate::getWindowManager()
{
    return dynamic_cast<WindowManager*>(m_visualizationCtx->getWindowManager(m_windowManagerID));
}

DesignerScenario::DesignerScenario(Kernel::IKernelContext* context, Kernel::IScenario& scenario, const CIdentifier& scenarioID)
{
    d = new DesignerScenarioPrivate(this, context, scenario, scenarioID, *new PlayerBridge(context, nullptr));

    d->m_visualizationCtx = dynamic_cast<VisualizationToolkit::IVisualizationContext*>(d->kernelCtx->getPluginManager().createPluginObject(OVP_ClassId_Plugin_VisualizationCtx));
    d->m_windowManagerID = d->m_visualizationCtx->addWindowManager(std::make_unique<WindowManager>(*(d->kernelCtx), this));

    // Setup cpu usage timer
    d->m_cpuUsageTimer.setInterval(1000);
    connect(&d->m_cpuUsageTimer, &QTimer::timeout, this, &DesignerScenario::cpuUsageChanged);
    // a scenario is a metabox as soon as it exposes an input or an output
    connect(this, &DesignerScenario::inputPortsChanged, this, &DesignerScenario::isMetaboxChanged);
    connect(this, &DesignerScenario::outputPortsChanged, this, &DesignerScenario::isMetaboxChanged);

    d->fillModel();

    // Undo/redo history. The baseline is the state right after creation/loading, so the first user
    // action is undoable too.
    const int historyLimit = int(context->getConfigurationManager().expandAsUInteger("${Designer_UndoRedoStackSize}", 64));
    d->m_history = new ScenarioHistory(historyLimit, [this](const QByteArray& snapshot) { return restoreState(snapshot); }, this);
    connect(d->m_history, &ScenarioHistory::changed, this, &DesignerScenario::historyChanged);
    d->m_commitTimer.setSingleShot(true);
    connect(&d->m_commitTimer, &QTimer::timeout, this, [this]() { commitChange(d->m_pendingLabel); });
    d->m_lastState = captureState();

    emit inputPortsChanged();
    emit outputPortsChanged();
}

DesignerScenario::~DesignerScenario()
{
    d->m_visualizationCtx->removeWindowManager(d->m_windowManagerID);

    delete d;
    d = nullptr;
}

void DesignerScenario::addBox(const QString& boxPluginIDStr, const QPoint& position)
{
    // 1 Add box
    CIdentifier boxID;
    Kernel::IBox *boxDetails = nullptr;

    if (boxPluginIDStr.startsWith(METABOX_DRAG_PREFIX))
    {
        // Metaboxes share the same algorithm class identifier: the palette gives the metabox identifier instead,
        // and the box prototype comes from the descriptor built by the metabox manager from the metabox scenario
        const CIdentifier metaboxID(boxPluginIDStr.mid(int(strlen(METABOX_DRAG_PREFIX))).toStdString());
        const auto* boxAlgorithmDesc = dynamic_cast<const Plugins::IBoxAlgorithmDesc*>(d->kernelCtx->getMetaboxManager().getMetaboxObjectDesc(metaboxID));
        if (boxAlgorithmDesc == nullptr)
        {
            d->kernelCtx->getLogManager() << Kernel::LogLevel_Error << "Failed to find the metabox " << metaboxID << "\n";
        }
        else if (!d->m_scenario.addBox(boxID, *boxAlgorithmDesc, CIdentifier::undefined()))
        {
            d->kernelCtx->getLogManager() << Kernel::LogLevel_Error << "Failed to add the metabox " << metaboxID << " to the scenario\n";
        }
        else
        {
            boxDetails = d->m_scenario.getBoxDetails(boxID);
            // the scheduler expands the metabox at play time from this attribute
            boxDetails->addAttribute(OVP_AttributeId_Metabox_ID, metaboxID.toString());
        }
    }
    else
    {
        const CIdentifier boxAlgorithmClassID(boxPluginIDStr.toStdString());
        if (boxAlgorithmClassID == CIdentifier::undefined())
        {
            d->kernelCtx->getLogManager() << Kernel::LogLevel_Error << "Failed to find the box algorithm class identifier for " << boxPluginIDStr.toStdString() << "\n";
        }
        else if (!d->m_scenario.addBox(boxID, boxAlgorithmClassID, CIdentifier::undefined()))
        {
            d->kernelCtx->getLogManager() << Kernel::LogLevel_Error << "Failed to add the box " << boxAlgorithmClassID << " to the scenario\n";
        }
        else
        {
            boxDetails = d->m_scenario.getBoxDetails(boxID);
        }
    }

    if (boxDetails)
    {
        // 2 create BoxObject (registers it in the window manager if it is a display box)
        BoxObject *box = d->createBoxObject(boxDetails);
        box->setCenter(position);

        emit boxesChanged();
        requestCommit(tr("Add box"));
    }
}

Kernel::IScenario& DesignerScenario::scenario() const 
{
    return d->m_scenario;
}

const CIdentifier& DesignerScenario::id() const 
{
    return d->scenarioID;
}

PlayerBridge& DesignerScenario::currentPlayer() 
{
    return d->player;
}

bool DesignerScenario::isEmpty()
{
    //is empty if no boxes, no metadatas, no comments
    return (d->m_scenario.getNextBoxIdentifier(CIdentifier::undefined()) == CIdentifier::undefined()) &&
           (d->m_scenario.getNextMetadataIdentifier(CIdentifier::undefined()) == CIdentifier::undefined()) &&
           (d->m_scenario.getNextCommentIdentifier(CIdentifier::undefined()) == CIdentifier::undefined());
}

PlayerBridge& DesignerScenario::createOrGetPlayer()
{
    if (d->player.isNull())
    {
        // create a snapshot so settings override does not modify the scenario !
        //d->m_scenario.SnapshotCB(false);

        // set filename attribute to scenario so delayed configuration can be used
        if (d->m_scenario.hasAttribute(OV_AttributeId_ScenarioFilename))
        {
            d->m_scenario.setAttributeValue(OV_AttributeId_ScenarioFilename, d->filePath.toLocalFile().toStdString().c_str());
        }
        else
        {
            d->m_scenario.addAttribute(OV_AttributeId_ScenarioFilename, d->filePath.toLocalFile().toStdString().c_str());
        }

        d->kernelCtx->getPlayerManager().createPlayer(d->playerID);
        d->player.setPlayer(&d->kernelCtx->getPlayerManager().getPlayer(d->playerID));
        if (!d->player.setScenario(d->scenarioID))
        {
            d->kernelCtx->getPlayerManager().releasePlayer(d->playerID);
            d->player.setPlayer(nullptr);
            d->playerID = CIdentifier::undefined();
            d->player.clear();
            d->kernelCtx->getLogManager() << Kernel::LogLevel_Error << "The scenario could not be set to the player.\n";
        }
        else
        {
            // Sets window manager ID to runtime config. This way, visualization boxes can access it.
            // Needs to be set between setScenario() and initialize().
            d->player.setWindowManager(d->m_windowManagerID);

            if (d->player.initialize() != Kernel::EPlayerReturnCodes::Success)
            {
                d->kernelCtx->getLogManager() << Kernel::LogLevel_Error << "The player could not be initialized.\n";
                d->kernelCtx->getPlayerManager().releasePlayer(d->playerID);
                d->player.setPlayer(nullptr);
                d->playerID = CIdentifier::undefined();
                d->player.clear();
            }
        }
    }

    if (!d->player.isNull())
    {
        // Start cpu usage timer
        d->m_cpuUsageTimer.start();
    }

    emit historyChanged(); // undo/redo are locked while a player exists
    return d->player;
}

void DesignerScenario::releasePlayer()
{
    d->m_cpuUsageTimer.stop();

    if(d->player.isNull())
    {
        d->kernelCtx->getLogManager() << Kernel::LogLevel_Warning << "Trying to release non-initialized player\n";
    }
    else
    {
        d->kernelCtx->getErrorManager().releaseErrors();

        if (!d->player.uninitialize())
        {
            d->kernelCtx->getLogManager() << Kernel::LogLevel_Error << "Failed to uninitialize the player" << "\n"; 
        }
        d->player.setPlayer(nullptr);

        if(!d->kernelCtx->getPlayerManager().releasePlayer(d->playerID))
        {
            d->kernelCtx->getLogManager() << Kernel::LogLevel_Error << "Failed to release the player" << "\n";
        }
        d->playerID = CIdentifier::undefined();

        emit cpuUsageChanged();
        emit historyChanged();
    }
}

QString DesignerScenario::name() const
{
    return d->m_scenario.getName().toASCIIString();
}

QUrl DesignerScenario::filePath() const
{
    return d->filePath;
}

void DesignerScenario::setName(const QString &name)
{
    QString currentName = d->m_scenario.getName().toASCIIString();
    if (currentName != name)
    {
        d->m_scenario.setName(name.toStdString().c_str());
        emit nameChanged(name);
    }
}

void DesignerScenario::setFilePath(const QUrl& filePath)
{
    QUrl currentPath = d->filePath;
    if (currentPath != filePath)
    {
        d->filePath = filePath;

        //set the name of the scenario to the file name
        QFileInfo fileInfo(filePath.toLocalFile());
        this->setName(fileInfo.baseName());

        emit filePathChanged(filePath);
    }
}

QQmlListProperty<BoxObject> DesignerScenario::boxes()
{
    d->kernelCtx->getLogManager() << Kernel::LogLevel_Debug << "there are " << int(d->boxes.size()) << " boxes in the scenario \n";

    return QQmlListProperty<BoxObject>(this, &d->boxes);
}

QQmlListProperty<LinkObject> DesignerScenario::links()
{
    d->kernelCtx->getLogManager() << Kernel::LogLevel_Debug << "there are " << int(d->links.size()) << " links in the scenario \n";

    return QQmlListProperty<LinkObject>(this, &d->links);
}

void DesignerScenario::clickPort(int portIdx,const QString& boxId, qreal x, qreal y, bool isOutput)
{
    // UG fork: a second click on a port of the same kind as the first (another output, or the very same port again) starts a new
    // link from there, rather than trying to link two outputs, two inputs, or a box to itself
    if(d->linkCreationStarted && isOutput == d->linkCreationSourceIsOutput)
    {
        this->stopLinkCreation();
    }

    // UG fork: every way out of the second click ends link creation, so that a failed attempt does not leave it
    // pending (upstream left the link dangling after an error)
    if(d->linkCreationStarted)
    {
        for(auto *box : d->boxes)
        {
            QString current_id = box->id().toString().toASCIIString();
            if(current_id == boxId)
            {
                //create the OpenVibe Link
                //first check if the connection is allowed
                CIdentifier newLinkId;
                CIdentifier boxSrcTypeID;
                CIdentifier boxDstTypeID;
                int boxSrcPort, boxDstPort;
                Kernel::IBox* srcBox = nullptr;
                Kernel::IBox* dstBox = nullptr;
                if(d->linkCreationSourceIsOutput)
                {
                    srcBox = d->m_scenario.getBoxDetails(d->linkCreationSource->id());
                    dstBox = d->m_scenario.getBoxDetails(box->id());
                    boxSrcTypeID = d->linkCreationSource->id();
                    boxDstTypeID = box->id();
                    boxSrcPort = d->linkCreationSourcePort;
                    boxDstPort = portIdx;
                }
                else
                {
                    srcBox = d->m_scenario.getBoxDetails(box->id());
                    dstBox = d->m_scenario.getBoxDetails(d->linkCreationSource->id());
                    boxSrcTypeID = box->id();
                    boxDstTypeID = d->linkCreationSource->id();
                    boxSrcPort = portIdx;
                    boxDstPort = d->linkCreationSourcePort;
                }
                
                if(!srcBox)
                {
                    d->kernelCtx->getLogManager() << Kernel::LogLevel_Error << "Failed to find the box details for " << boxSrcTypeID.toString() << " as a Source \n";
                    this->stopLinkCreation();
                    return;
                }
                if(!dstBox)
                {
                    d->kernelCtx->getLogManager() << Kernel::LogLevel_Error << "Failed to find the box details for " << boxDstTypeID.toString() << " as a Target \n";
                    this->stopLinkCreation();
                    return;
                }

                CIdentifier srcTypeID;
                CIdentifier dstTypeID;
                srcBox->getOutputType(boxSrcPort, srcTypeID);
                dstBox->getInputType(boxDstPort, dstTypeID);
                
                //no deprecated input or output
                bool hasDeprecatedIO = false;
                srcBox->getInterfacorDeprecatedStatus(Kernel::Output, boxSrcPort, hasDeprecatedIO);
                if(hasDeprecatedIO)
                {
                    d->kernelCtx->getLogManager() << Kernel::LogLevel_Warning << "Cannot connect from deprecated Output :" << boxSrcTypeID.toString() << " \n";
                    this->stopLinkCreation();
                    return;
                }
                dstBox->getInterfacorDeprecatedStatus(Kernel::Input, boxDstPort, hasDeprecatedIO);
                if(hasDeprecatedIO)
                {
                    d->kernelCtx->getLogManager() << Kernel::LogLevel_Warning << "Cannot connect to deprecated Input : " << boxDstTypeID.toString()  << " \n";
                    this->stopLinkCreation();
                    return;
                }

                //Do the connection
                if ((d->kernelCtx->getTypeManager().isDerivedFromStream(srcTypeID, dstTypeID)
                         || d->kernelCtx->getConfigurationManager().expandAsBoolean("${Designer_AllowUpCastConnection}", false)))
                {
                    if(!d->m_scenario.connect(newLinkId, boxSrcTypeID, boxSrcPort, boxDstTypeID, boxDstPort, CIdentifier::undefined()))
                    {
                        d->kernelCtx->getLogManager() << Kernel::LogLevel_Error << "Failed to create link between " << d->linkCreationSource->name().toStdString()
                                                      << " and " << boxId.toStdString() << "\n";
                        this->stopLinkCreation();
                        return;
                    }
                }
                else
                {
                    d->kernelCtx->getLogManager() << Kernel::LogLevel_Error << "Failed to create link between " << d->linkCreationSource->name().toStdString()
                                                      << " and " << boxId.toStdString() << "\n";
                    this->stopLinkCreation();
                    return;
                }
                
                //the method connect may delete an existing link when creating a new one (e.g. if destination port was already connected to another box)!!
                //we should update all our links if the number of links in the scenario has changed
                int nbLinks = 0;
                CIdentifier linkId = d->m_scenario.getNextLinkIdentifier(CIdentifier::undefined());
                while(linkId != CIdentifier::undefined())
                {
                    nbLinks++;
                    linkId = d->m_scenario.getNextLinkIdentifier(linkId);
                }

                if((d->links.size() + 1) != nbLinks)
                {
                    qDeleteAll(d->links);
                    d->links.clear();
                    d->fillLinks();
                }
                else
                {
                    Kernel::ILink *iLink = d->m_scenario.getLinkDetails(newLinkId);

                    //create the link object
                    LinkObject *link = nullptr;
                    if(d->linkCreationSourceIsOutput)
                    {
                        link = new LinkObject(d->kernelCtx, iLink, d->linkCreationSource, box);
                    }
                    else
                    {
                        link = new LinkObject(d->kernelCtx, iLink, box, d->linkCreationSource);
                    }
                    d->links.append(link);
                    
                    emit linksChanged();
                }

                //link creation is ok and finished
                //stop the link creation mode
                this->stopLinkCreation();
                requestCommit(tr("Connect"));
                return;
            }
        }
        d->kernelCtx->getLogManager() << Kernel::LogLevel_Error << "Failed to find the box " << boxId.toStdString() << " as a Target \n";
        this->stopLinkCreation();
        return;
    }

    for(auto *box : d->boxes)
    {
        QString current_id = box->id().toString().toASCIIString();
        if(current_id == boxId)
        {
            d->linkCreationStarted = true;
            d->linkCreationSource = box;
            d->linkCreationSourcePort = portIdx;
            d->linkCreationSourceIsOutput = isOutput;
            emit startLinkCreation(x, y); // here x,y are box+port coordinates ? 
            return;
        }
    }
    d->kernelCtx->getLogManager() << Kernel::LogLevel_Error << "Failed to find the box " << boxId.toStdString() << "as a Source \n";
    emit finishLinkCreation();
}

void DesignerScenario::removeLinkOnPort(int portIdx, QString boxId, bool isOutput)
{
    bool found = false;
    for(int i=0; i < d->links.length() && !found; ++i)
    {
        auto *link = d->links.at(i);
        BoxObject *box = nullptr;
        int port = -1;
        if(isOutput)
        {
            box = link->m_sourceBox;
            port = link->m_link->getSourceBoxOutputIndex();
        }
        else
        {
            box = link->m_targetBox;
            port = link->m_link->getTargetBoxInputIndex();
        }
        QString current_id = box->id().toString().toASCIIString();

        if((current_id == boxId) && (port == portIdx))
        {
            //remove the link from the scenario
            this->removeLink(i);
            found = true;
        }
    }
    if(!found)
    {
        // there is no link to remove, do nothing!
        d->kernelCtx->getLogManager() << Kernel::LogLevel_Warning << "Failed to find the link to remove\n";
    }
}

void DesignerScenario::removeLink(int linkIdx)
{
    LinkObject *link = d->links.takeAt(linkIdx);
    if(!d->m_scenario.disconnect(link->m_link->getIdentifier()))
    {
        d->kernelCtx->getLogManager() << Kernel::LogLevel_Error << "Failed to remove link between " << link->m_sourceBox->name().toStdString()
                                      << " and " << link->m_targetBox->name().toStdString() << "\n";
    }

    //stop any link creation previously started
    this->stopLinkCreation();

    //remove the link from the list
    delete link;
    emit linksChanged();
    requestCommit(tr("Remove link"));
}

void DesignerScenario::stopLinkCreation()
{
    d->linkCreationStarted = false;
    d->linkCreationSource = nullptr;
    d->linkCreationSourcePort = -1;
    d->linkCreationSourceIsOutput = false;
    emit finishLinkCreation();
}

void DesignerScenario::removeBox(QString identifier)
{
    BoxObject *box = nullptr;
    for(auto* b : d->boxes)
    {
        if(b->id().toString() == identifier)
        {
            box = b;
            break;
        }
    }

    if (box)
    {
        // Remove box from visualization boxes
        // Won't be found if not visualization box
        d->getWindowManager()->removeVisualizationBox(box);

        // Remove the box
        const CIdentifier boxId = box->id();
        const QString boxName = box->name();
        box->detachFromKernelBox();
        if(!d->m_scenario.removeBox(boxId))
        {
            d->kernelCtx->getLogManager() << Kernel::LogLevel_Error << "Failed to remove box " << boxName.toStdString() << "\n";
        }

        //remove all linkObjects connected to this box
        auto it = d->links.begin();
        while(it != d->links.end())
        {
            LinkObject *link = *it;
            if((link->m_sourceBox == box) || (link->m_targetBox == box))
            {
                it = d->links.erase(it);
                delete link;
            }
            else
            {
                ++it;
            }
        }

        auto it2 = d->m_scenarioIOConnections.begin();
        while (it2 != d->m_scenarioIOConnections.end())
        {
            ScenarioIOConnection *connection = *it2;
            if (connection->m_targetBox == box)
            {
                it2 = d->m_scenarioIOConnections.erase(it2);
                delete connection;
            }
            else
            {
                ++it2;
            }
        }

        //remove the box from the list
        d->boxes.remove(d->boxes.indexOf(box));
        d->boxesMap.remove(boxId);
        delete box;
        emit boxesChanged();
        emit linksChanged();
        emit scenarioIOConnectionsChanged();
        requestCommit(tr("Remove box"));
    }
}

QJSValue DesignerScenario::parameters() const
{
    QJSValue jsParams;
    QQmlEngine *jsEngine = QmlEngineSingleton::getInstance().engine();
    if(!jsEngine)
    {
        qWarning() << Q_FUNC_INFO << "Could not obtain QQmlEngine for object.\n";
    }
    else
    {
        int nbParams = d->m_parameters.size();
        jsParams = jsEngine->newArray(nbParams);
        for (int i = 0; i < nbParams; ++i)
        {
            jsParams.setProperty(i, jsEngine->newQObject(d->m_parameters[i]));
        }
    }
    return jsParams;
}

QStringList DesignerScenario::getAvailableParameterTypes() const
{
    QStringList types;
    std::vector<std::string> availableTypes;
    d->m_scenario.getAvailableParameterTypes(availableTypes);
    for (const auto& type : availableTypes)
    {
        types << QString::fromStdString(type);
    }
    return types;
}

void DesignerScenario::addParameter(const QString& name, const QString& type)
{
    d->m_scenario.addParameter(name.toStdString(), type.toStdString());

    d->updateParameters();
    emit parametersChanged();
    requestCommit(tr("Add scenario parameter"));
}

void DesignerScenario::removeParameter(const QString& name)
{
    d->m_scenario.removeParameter(name.toStdString());
    d->updateParameters();
    emit parametersChanged();
    requestCommit(tr("Remove scenario parameter"));
}

void DesignerScenario::updateParameterName(const QString& oldName, const QString& newName)
{
    d->m_scenario.updateParameterName(oldName.toStdString(), newName.toStdString());
    emit parametersChanged();
    requestCommit(tr("Rename scenario parameter"));
}

QQmlListProperty<ScenarioParameterConnectionObject> DesignerScenario::getParameterConnectionsForParam(const QString& scenarioParameterName)
{

    // Todo: retrieve connections form scenario in a single call. This will bring consistency in the order of appearance in the UI.
    std::vector<Kernel::CScenarioParameterConnection*> parameterConnections;
    d->m_scenario.getParameterConnections(scenarioParameterName.toStdString(), parameterConnections);

    for (auto& parameterConnection : d->m_parameterConnections)
    {
        delete parameterConnection;
    }
    d->m_parameterConnections.clear();

    for (auto& parameterConnection : parameterConnections)
    {
        d->m_parameterConnections.append(new ScenarioParameterConnectionObject(d->kernelCtx,
                                                                               parameterConnection->getScenarioParameterName().c_str(),
                                                                               parameterConnection->getBoxID().str().c_str(),
                                                                               parameterConnection->getBoxParameterName().c_str(),
                                                                               true));
    }

    std::vector<Kernel::CScenarioParameterConnection> parameterConnectionCandidates;
    d->m_scenario.getParameterConnectionCandidates(scenarioParameterName.toStdString(), parameterConnectionCandidates);
    for (auto& parameterConnection : parameterConnectionCandidates)
    {
        d->m_parameterConnections.append(new ScenarioParameterConnectionObject(d->kernelCtx,
                                                                               parameterConnection.getScenarioParameterName().c_str(),
                                                                               parameterConnection.getBoxID().str().c_str(),
                                                                               parameterConnection.getBoxParameterName().c_str(),
                                                                               false));
    }

    return QQmlListProperty<ScenarioParameterConnectionObject>(this, &d->m_parameterConnections);
}

void DesignerScenario::updateParameterConnection(const QString& scenarioParameterName, const QString& boxIdentifier, const QString& boxParameterName, bool connect)
{
    CIdentifier boxID = CIdentifier(boxIdentifier.toStdString());
    if (connect)
    {
        d->m_scenario.addParameterConnection(scenarioParameterName.toStdString(), boxID, boxParameterName.toStdString().c_str());
    }
    else
    {
        d->m_scenario.removeParameterConnection(scenarioParameterName.toStdString(), boxID, boxParameterName.toStdString().c_str());
    }
    requestCommit(connect ? tr("Connect scenario parameter") : tr("Disconnect scenario parameter"));
}

QQmlListProperty<CommentObject> DesignerScenario::comments()
{
    return QQmlListProperty<CommentObject>(this, &d->comments);
}

void DesignerScenario::addComment(const QString& text, const QPoint& origin)
{
    // 1 create OpenViBE comment
    CIdentifier commentID;
    if(!d->m_scenario.addComment(commentID, CIdentifier::undefined()))
    {
        d->kernelCtx->getLogManager() << Kernel::LogLevel_Error << "Failed to add comment " << text.toStdString() << "\n";
    }
    else
    {
        // 2 create CommentObject. The text goes to the kernel first so that the wrapper does not report
        // it as a separate "Edit comment" step.
        Kernel::IComment* commentDetails = d->m_scenario.getCommentDetails(commentID);
        commentDetails->setText(text.toStdString().c_str());
        CommentObject *comment = d->createCommentObject(commentDetails);
        comment->setCenter(origin);

        emit commentsChanged();
        requestCommit(tr("Add comment"));
    }
}

void DesignerScenario::removeComment(int commentIdx)
{
    CommentObject *comment = d->comments.takeAt(commentIdx);
    if(!d->m_scenario.removeComment(comment->id()))
    {
        d->kernelCtx->getLogManager() << Kernel::LogLevel_Error << "Failed to remove comment " << comment->text().toStdString() << "\n";
    }

    //remove the comment from the list
    delete comment;
    emit commentsChanged();
    requestCommit(tr("Remove comment"));
}

QQmlListProperty<IOPort> DesignerScenario::inputPorts()
{
    return QQmlListProperty<IOPort>(this, &d->m_inputPorts);
}

QQmlListProperty<IOPort> DesignerScenario::outputPorts()
{
    return QQmlListProperty<IOPort>(this, &d->m_outputPorts);
}

QQmlListProperty<ScenarioIOConnection> DesignerScenario::scenarioIOConnections()
{
    return QQmlListProperty<ScenarioIOConnection>(this, &d->m_scenarioIOConnections);
}

QList<QString> DesignerScenario::getSupportedInputTypes() const
{
    QList<QString> supportedTypes;
    std::vector<std::pair<CIdentifier, CString>> sortedTypes = d->kernelCtx->getTypeManager().getSortedTypes();
    for (const auto& type : sortedTypes)
    {
        if (d->kernelCtx->getTypeManager().isStream(type.first) && d->m_scenario.hasInputSupport(type.first))
        {
            supportedTypes.push_back(QString::fromStdString(type.second.toASCIIString()));
        }
    }

    return supportedTypes;
}

void DesignerScenario::addInput(const QString& name, const QString& typeId)
{
    CIdentifier cidTypeId = d->kernelCtx->getTypeManager().getTypeFromName(typeId.toStdString().c_str());
    CIdentifier inputId = d->m_scenario.getUnusedInputIdentifier();
    if(!d->m_scenario.addInput(name.toStdString().c_str(), cidTypeId, inputId, true))
    {
        d->kernelCtx->getLogManager() << Kernel::LogLevel_Warning << "Could not add input port to box " << d->m_scenario.getName() << "\n";
    }
    else
    {
        //A Listener may have been called in which case, the input port list has been updated
        if(d->m_inputPorts.size() != d->m_scenario.getInputCount())
        {
            d->createScenarioPort(IOPort::PortType::Input, int(d->m_inputPorts.size()));
            emit inputPortsChanged();
        }
        requestCommit(tr("Add scenario input"));
    }
}

void DesignerScenario::removeInput(int portIndex)
{
    d->m_scenario.removeInput(portIndex);

    //calling IBox::removeInput deletes all the links and recreate them..
    //so we need to update all links
    for(auto* port: d->m_inputPorts)
    {
        delete port;
    }
    d->m_inputPorts.clear();

    for(size_t i=0; i < d->m_scenario.getInputCount(); i++)
    {
        d->createScenarioPort(IOPort::PortType::Input, int(i));
    }
    d->refillScenarioIOConnections(); // they pointed at the old ports
    emit inputPortsChanged();
    emit scenarioIOConnectionsChanged();
    requestCommit(tr("Remove scenario input"));
}

QList<QString> DesignerScenario::getSupportedOutputTypes() const
{
    QList<QString> supportedTypes;
    std::vector<std::pair<CIdentifier, CString>> sortedTypes = d->kernelCtx->getTypeManager().getSortedTypes();
    for (const auto& type : sortedTypes)
    {
        if (d->kernelCtx->getTypeManager().isStream(type.first) && d->m_scenario.hasOutputSupport(type.first))
        {
            supportedTypes.push_back(QString::fromStdString(type.second.toASCIIString()));
        }
    }

    return supportedTypes;
}

void DesignerScenario::addOutput(const QString& name, const QString& typeId)
{
    CIdentifier cidTypeId = d->kernelCtx->getTypeManager().getTypeFromName(typeId.toStdString().c_str());
    CIdentifier outputId = d->m_scenario.getUnusedOutputIdentifier();
    if(!d->m_scenario.addOutput(name.toStdString().c_str(), cidTypeId, outputId, true))
    {
        d->kernelCtx->getLogManager() << Kernel::LogLevel_Warning << "Could not add output port to box " << d->m_scenario.getName() << "\n";
    }
    else
    {
        //A Listener may have been called in which case, the output port list has been updated
        if(d->m_outputPorts.size() != d->m_scenario.getOutputCount())
        {
            d->createScenarioPort(IOPort::PortType::Output, int(d->m_outputPorts.size()));
            emit outputPortsChanged();
        }
        requestCommit(tr("Add scenario output"));
    }
}

void DesignerScenario::removeOutput(int portIndex)
{
    d->m_scenario.removeOutput(portIndex);

    //calling IBox::removeOutput deletes all the links and recreate them..
    //so we need to update all links
    for(auto* port: d->m_outputPorts)
    {
        delete port;
    }
    d->m_outputPorts.clear();

    for(size_t i=0; i < d->m_scenario.getOutputCount(); i++)
    {
        d->createScenarioPort(IOPort::PortType::Output, int(i));
    }
    d->refillScenarioIOConnections(); // they pointed at the old ports
    emit outputPortsChanged();
    emit scenarioIOConnectionsChanged();
    requestCommit(tr("Remove scenario output"));
}

WindowManager* DesignerScenario::windowManager()
{
    return d->getWindowManager();
}

double DesignerScenario::cpuUsage() const
{
    return d->player.getCPUUsage();
}

bool DesignerScenario::isMetabox() const
{
    return d->m_scenario.isMetabox();
}

QString DesignerScenario::metaboxId() const { return d->attribute(OVP_AttributeId_Metabox_ID); }
QString DesignerScenario::metaboxName() const { return d->attribute(OV_AttributeId_Scenario_Name); }
QString DesignerScenario::author() const { return d->attribute(OV_AttributeId_Scenario_Author); }
QString DesignerScenario::company() const { return d->attribute(OV_AttributeId_Scenario_Company); }
QString DesignerScenario::category() const { return d->attribute(OV_AttributeId_Scenario_Category); }
QString DesignerScenario::version() const { return d->attribute(OV_AttributeId_Scenario_Version); }
QString DesignerScenario::shortDescription() const { return d->attribute(OV_AttributeId_Scenario_ShortDescription); }
QString DesignerScenario::detailedDescription() const { return d->attribute(OV_AttributeId_Scenario_DetailedDescription); }

void DesignerScenario::setMetaboxName(const QString& value) { d->setMetadata(OV_AttributeId_Scenario_Name, value); }
void DesignerScenario::setAuthor(const QString& value) { d->setMetadata(OV_AttributeId_Scenario_Author, value); }
void DesignerScenario::setCompany(const QString& value) { d->setMetadata(OV_AttributeId_Scenario_Company, value); }
void DesignerScenario::setCategory(const QString& value) { d->setMetadata(OV_AttributeId_Scenario_Category, value); }
void DesignerScenario::setVersion(const QString& value) { d->setMetadata(OV_AttributeId_Scenario_Version, value); }
void DesignerScenario::setShortDescription(const QString& value) { d->setMetadata(OV_AttributeId_Scenario_ShortDescription, value); }
void DesignerScenario::setDetailedDescription(const QString& value) { d->setMetadata(OV_AttributeId_Scenario_DetailedDescription, value); }

void DesignerScenario::regenerateMetaboxId()
{
    d->setMetadata(OVP_AttributeId_Metabox_ID, CIdentifier::random().str().c_str());
}

void DesignerScenario::ensureMetaboxAttributes()
{
    bool changed = false;

    CIdentifier currentMetaboxId;
    if(!currentMetaboxId.fromString(metaboxId().toStdString()) || currentMetaboxId == CIdentifier::undefined())
    {
        d->setAttribute(OVP_AttributeId_Metabox_ID, CIdentifier::random().str().c_str());
        changed = true;
    }

    if(metaboxName().isEmpty())
    {
        d->setAttribute(OV_AttributeId_Scenario_Name, name());
        changed = true;
    }

    if(changed)
    {
        emit metaboxMetadataChanged();
    }
}

void DesignerScenario::setScenarioInputLink(int portIndex, const QString& boxId, int boxInputIndex)
{
    CIdentifier cidBoxId = CIdentifier(boxId.toStdString().c_str());
    if (d->m_scenario.setScenarioInputLink(portIndex, cidBoxId, boxInputIndex))
    {
        // Check and clear any conflicting existing connections
        auto it = d->m_scenarioIOConnections.begin();
        while (it != d->m_scenarioIOConnections.end())
        {
            ScenarioIOConnection *connection = *it;
            if (connection->isOutput() == false // only check inputs
                && (connection->scenarioPortIdx() == portIndex  // scenario port index already used
                    || (connection->targetBoxId() == boxId  // or box port already connected
                        && connection->targetPortIdx() == boxInputIndex)
                ))
            {
                it = d->m_scenarioIOConnections.erase(it);
                delete connection;
            }
            else
            {
                ++it;
            }
        }

        // Add new connection to model
        d->m_scenarioIOConnections.append(new ScenarioIOConnection(d->kernelCtx, d->m_inputPorts.at(portIndex), d->boxesMap.value(cidBoxId), boxInputIndex, false));
        emit scenarioIOConnectionsChanged();
        requestCommit(tr("Connect scenario input"));
    }
}

void DesignerScenario::setScenarioOutputLink(int portIndex, const QString& boxId, int boxOutputIndex)
{
    CIdentifier cidBoxId = CIdentifier(boxId.toStdString().c_str());
    if (d->m_scenario.setScenarioOutputLink(portIndex, cidBoxId, boxOutputIndex))
    {

        // Check and clear any conflicting existing connection
        auto it = d->m_scenarioIOConnections.begin();
        while (it != d->m_scenarioIOConnections.end())
        {
            ScenarioIOConnection *connection = *it;
            if (connection->isOutput() == true // only check outputs
                && (connection->scenarioPortIdx() == portIndex  // scenario port index already used
                    || (connection->targetBoxId() == boxId  // or box port already connected
                        && connection->targetPortIdx() == boxOutputIndex)
                ))
            {
                it = d->m_scenarioIOConnections.erase(it);
                delete connection;
            }
            else
            {
                ++it;
            }
        }

        // Add new connection to model
        d->m_scenarioIOConnections.append(new ScenarioIOConnection(d->kernelCtx, d->m_outputPorts.at(portIndex), d->boxesMap.value(cidBoxId), boxOutputIndex, true));
        emit scenarioIOConnectionsChanged();
        requestCommit(tr("Connect scenario output"));
    }
}

void DesignerScenario::removeScenarioIOConnection(int portIndex, const QString& boxId, int boxPortIndex, bool isOutput)
{
    bool res = false;
    CIdentifier cidBoxId = CIdentifier(boxId.toStdString().c_str());
    if (isOutput)
    {
        res = d->m_scenario.removeScenarioOutputLink(portIndex, cidBoxId, boxPortIndex);
    }
    else
    {
        res = d->m_scenario.removeScenarioInputLink(portIndex, cidBoxId, boxPortIndex);
    }
    if (res)
    {
        auto it = std::find_if(d->m_scenarioIOConnections.begin(), d->m_scenarioIOConnections.end(), [portIndex, boxId, boxPortIndex, isOutput](const auto& c) {
            return c->scenarioPortIdx() == portIndex
                && c->targetPortIdx() == boxPortIndex 
                && c->targetBoxId() == boxId
                && c->isOutput() == isOutput;
        });
        if (it != d->m_scenarioIOConnections.end())
        {
            delete *it;
            d->m_scenarioIOConnections.erase(it);
        }
        emit scenarioIOConnectionsChanged();
        requestCommit(tr("Disconnect scenario I/O"));
    }
    else
    {
        d->kernelCtx->getLogManager() << Kernel::LogLevel_Error << "Failed to remove scenario IO connection " << portIndex << " of box " << boxId.toStdString() << "\n";
    }
}

// ------------------------------------------------------------------------------------------------
// Undo / redo
// ------------------------------------------------------------------------------------------------

bool DesignerScenario::canUndo() const { return d->m_history->canUndo() && d->player.isNull(); }
bool DesignerScenario::canRedo() const { return d->m_history->canRedo() && d->player.isNull(); }
QString DesignerScenario::undoText() const { return d->m_history->undoText(); }
QString DesignerScenario::redoText() const { return d->m_history->redoText(); }
bool DesignerScenario::modified() const { return !d->m_history->isClean(); }

QByteArray DesignerScenario::captureState() const
{
    const QByteArray xml = d->m_serializer.toXml(d->m_scenario);
    return xml.isEmpty() ? QByteArray() : qCompress(xml);
}

void DesignerScenario::commitChange(const QString& label)
{
    const QString text = label; // `label` may alias m_pendingLabel (flushPendingCommit)
    d->m_commitTimer.stop();
    d->m_pendingLabel.clear();

    if (!d->m_restoring && d->player.isNull())
    {
        const QByteArray after = captureState();
        // No-op edits and pure layout feedback (box sizes...) never reach the history.
        if (!after.isEmpty() && after != d->m_lastState)
        {
            d->m_history->push(d->m_lastState, after, text);
            d->m_lastState = after;
        }
    }
}

void DesignerScenario::onScenarioParameterEdited()
{
    requestCommit(tr("Change scenario parameter"), 300);
}

void DesignerScenario::requestCommit(const QString& label, int delayMs)
{
    if (!d->m_restoring)
    {
        // Kernel listeners report their side effects (port renames...) while an operation runs; the
        // operation itself reports last, so the latest label names the step.
        d->m_pendingLabel = label;
        d->m_commitTimer.start(delayMs);
    }
}

void DesignerScenario::flushPendingCommit()
{
    if (d->m_commitTimer.isActive())
    {
        commitChange(d->m_pendingLabel);
    }
}

void DesignerScenario::markSaved()
{
    flushPendingCommit();
    d->m_history->setClean();
}

void DesignerScenario::undo()
{
    if (canUndo())
    {
        flushPendingCommit();
        d->m_history->undo();
    }
}

void DesignerScenario::redo()
{
    if (canRedo())
    {
        flushPendingCommit();
        d->m_history->redo();
    }
}

void DesignerScenario::removeSelection(const QStringList& boxIds, const QStringList& commentIds)
{
    if (d->player.isNull() && (!boxIds.isEmpty() || !commentIds.isEmpty()))
    {
        // Wrappers hold raw kernel pointers: tear them all down before touching the kernel.
        rebuildModel([this, &boxIds, &commentIds]() {
            for (const QString& id : commentIds)
            {
                const CIdentifier commentId(id.toStdString());
                if (d->m_scenario.isComment(commentId) && !d->m_scenario.removeComment(commentId))
                {
                    d->kernelCtx->getLogManager() << Kernel::LogLevel_Error << "Failed to remove comment " << id.toStdString() << "\n";
                }
            }
            for (const QString& id : boxIds)
            {
                const CIdentifier boxId(id.toStdString());
                // The kernel drops the links attached to the box as well.
                if (d->m_scenario.isBox(boxId) && !d->m_scenario.removeBox(boxId))
                {
                    d->kernelCtx->getLogManager() << Kernel::LogLevel_Error << "Failed to remove box " << id.toStdString() << "\n";
                }
            }
        });
        requestCommit(tr("Delete"));
    }
}

// ------------------------------------------------------------------------------------------------
// Clipboard
// ------------------------------------------------------------------------------------------------

namespace {
QList<CIdentifier> toIdentifiers(const QStringList& ids)
{
    QList<CIdentifier> result;
    for (const QString& id : ids)
    {
        result.append(CIdentifier(id.toStdString()));
    }
    return result;
}
}

void DesignerScenario::copySelection(const QStringList& boxIds, const QStringList& commentIds)
{
    flushPendingCommit();
    ScenarioClipboard::instance().copy(*d->kernelCtx, d->m_serializer, d->m_scenario, toIdentifiers(boxIds), toIdentifiers(commentIds));
}

void DesignerScenario::cutSelection(const QStringList& boxIds, const QStringList& commentIds)
{
    if (d->player.isNull())
    {
        flushPendingCommit();
        if (ScenarioClipboard::instance().copy(*d->kernelCtx, d->m_serializer, d->m_scenario, toIdentifiers(boxIds), toIdentifiers(commentIds)))
        {
            removeSelection(boxIds, commentIds);
            // removeSelection() queued a "Delete" step for the same change
            d->m_pendingLabel = tr("Cut");
        }
    }
}

QStringList DesignerScenario::pasteFromClipboard(const QPoint& anchor)
{
    QStringList pastedBoxIds;
    if (d->player.isNull() && ScenarioClipboard::instance().hasContent())
    {
        flushPendingCommit();

        const ScenarioClipboard::Pasted pasted = ScenarioClipboard::instance().paste(*d->kernelCtx, d->m_serializer, d->m_scenario, anchor);
        if (!pasted.boxes.isEmpty() || !pasted.comments.isEmpty())
        {
            for (const CIdentifier& id : pasted.boxes)
            {
                pastedBoxIds.append(QString::fromStdString(id.str()));
            }

            // Rebuilding registers the pasted visualization boxes with the window manager as well
            rebuildModel();
            commitChange(tr("Paste"));
        }
    }
    return pastedBoxIds;
}

void DesignerScenario::rebuildModel(const std::function<void()>& kernelMutation)
{
    emit aboutToReset();
    this->stopLinkCreation();
    d->clearModel();
    if (kernelMutation)
    {
        kernelMutation();
    }
    d->fillModel();
    notifyModelReset();
}

void DesignerScenario::notifyModelReset()
{
    emit boxesChanged();
    emit linksChanged();
    emit commentsChanged();
    emit inputPortsChanged();
    emit outputPortsChanged();
    emit scenarioIOConnectionsChanged();
    emit parameterTypesChanged();
    emit parametersChanged();
    emit modelReset();
}

bool DesignerScenario::restoreState(const QByteArray& snapshot)
{
    bool res = false;
    const QByteArray xml = qUncompress(snapshot);
    if (xml.isEmpty())
    {
        d->kernelCtx->getLogManager() << Kernel::LogLevel_Error << "Cannot restore the scenario: empty snapshot\n";
    }
    else
    {
        d->m_restoring = true;
        d->m_commitTimer.stop();
        d->m_pendingLabel.clear();

        emit aboutToReset();
        this->stopLinkCreation();
        d->clearModel();
        d->clearParameterConnections();

        res = d->m_serializer.fromXml(xml, d->m_scenario);
        if (res)
        {
            d->m_lastState = snapshot;
        }
        else
        {
            // Never leave the scenario empty: fall back to the last known good state.
            d->kernelCtx->getLogManager() << Kernel::LogLevel_Error << "Failed to restore the scenario snapshot, keeping the previous state\n";
            const QByteArray previous = qUncompress(d->m_lastState);
            if (!previous.isEmpty())
            {
                d->m_serializer.fromXml(previous, d->m_scenario);
            }
        }

        d->fillModel();
        notifyModelReset();

        d->m_restoring = false;
    }
    return res;
}

}
}
