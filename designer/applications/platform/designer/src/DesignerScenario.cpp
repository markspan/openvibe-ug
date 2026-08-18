/**
 * SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#include <visualization-toolkit/ovviz_all.h>

#include "DesignerScenario.h"
#include "PlayerBridge.h"
#include "ParameterHelper.h"

#include "QmlEngineSingleton.h"
#include "WindowManager.h"

namespace OpenViBE {
namespace Designer {

class DesignerScenarioPrivate {
public:
    DesignerScenarioPrivate(Kernel::IKernelContext* kernelCtx, Kernel::IScenario& scenario, CIdentifier scenarioID, PlayerBridge& player);
    ~DesignerScenarioPrivate();

    void fillBoxes();
    void fillLinks();
    void updateParameters();
    WindowManager* getWindowManager();

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
};

DesignerScenarioPrivate::DesignerScenarioPrivate(Kernel::IKernelContext* kernelCtx, Kernel::IScenario& scenario, CIdentifier scenarioID, PlayerBridge& player) :
    kernelCtx(kernelCtx),
    m_scenario(scenario),
    scenarioID(scenarioID),
    player(player)
{
    updateParameters();
}


DesignerScenarioPrivate::~DesignerScenarioPrivate()
{
    boxesMap.clear();
    qDeleteAll(boxes);
    boxes.clear();
    qDeleteAll(links);
    links.clear();
    qDeleteAll(comments);
    comments.clear();
    qDeleteAll(m_parameterConnections);
    m_parameterConnections.clear();
    qDeleteAll(m_scenarioIOConnections);
    m_scenarioIOConnections.clear();
}

void DesignerScenarioPrivate::fillBoxes()
{
    // 1 add the boxes
    CIdentifier boxId = m_scenario.getNextBoxIdentifier(CIdentifier::undefined());
    while(boxId != CIdentifier::undefined()) {
        // check isBox(boxId);
        Kernel::IBox *boxDetails = m_scenario.getBoxDetails(boxId);
        BoxObject *box = new BoxObject(kernelCtx, boxDetails);
        boxesMap.insert(boxId, box);
        boxes.append(box);
        // Check if box is a display box
        auto pod = kernelCtx->getPluginManager().getPluginObjectDesc(boxDetails->getAlgorithmClassIdentifier());
        if (pod && pod->hasFunctionality(Plugins::EPluginFunctionality::Visualization)) {
            getWindowManager()->addVisualizationBox(box);
        }

        boxId = m_scenario.getNextBoxIdentifier(boxId);
    }
}

void DesignerScenarioPrivate::fillLinks()
{
    if(this->boxes.isEmpty()) {
        this->fillBoxes();
    }

    CIdentifier linkId = m_scenario.getNextLinkIdentifier(CIdentifier::undefined());
    while(linkId != CIdentifier::undefined()) {
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

void DesignerScenarioPrivate::updateParameters()
{
    setUIParameters(kernelCtx, &m_scenario, m_parameters);
}

WindowManager* DesignerScenarioPrivate::getWindowManager()
{
    return dynamic_cast<WindowManager*>(m_visualizationCtx->getWindowManager(m_windowManagerID));
}

DesignerScenario::DesignerScenario(Kernel::IKernelContext* context, Kernel::IScenario& scenario, const CIdentifier& scenarioID)
{
    d = new DesignerScenarioPrivate(context, scenario, scenarioID, *new PlayerBridge(context, nullptr));

    d->m_visualizationCtx = dynamic_cast<VisualizationToolkit::IVisualizationContext*>(d->kernelCtx->getPluginManager().createPluginObject(OVP_ClassId_Plugin_VisualizationCtx));
    d->m_windowManagerID = d->m_visualizationCtx->addWindowManager(std::make_unique<WindowManager>(*(d->kernelCtx), this));

    // Setup cpu usage timer
    d->m_cpuUsageTimer.setInterval(1000);
    connect(&d->m_cpuUsageTimer, &QTimer::timeout, this, &DesignerScenario::cpuUsageChanged);
    // fill input/output lists
    for(size_t i=0; i < scenario.getInputCount(); i++) {
        d->m_inputPorts.push_back(new IOPort(context, &scenario, IOPort::PortType::Input, i));
    }
    for(size_t i=0; i < scenario.getOutputCount(); i++) {
        d->m_outputPorts.push_back(new IOPort(context, &scenario, IOPort::PortType::Output, i));
    }

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
    CIdentifier boxAlgorithmClassID(boxPluginIDStr.toStdString());
    Kernel::IBox *boxDetails = nullptr;
    const Plugins::IPluginObjectDesc* pluginDesc;

    if (boxAlgorithmClassID == CIdentifier::undefined()) {
        d->kernelCtx->getLogManager() << Kernel::LogLevel_Error << "Failed to find the box algorithm class identifier for " << boxPluginIDStr.toStdString() << "\n";
        return;
    }

    if (boxAlgorithmClassID == OVP_ClassId_BoxAlgorithm_Metabox) {
        // Todo: handle metaboxes!
        d->kernelCtx->getLogManager() << Kernel::LogLevel_Error << "Metaboxes are not yet supported\n";
        return;
    } else {
        d->m_scenario.addBox(boxID, boxAlgorithmClassID, CIdentifier::undefined());
        boxDetails = d->m_scenario.getBoxDetails(boxID);
        CIdentifier id = boxDetails->getAlgorithmClassIdentifier();
        pluginDesc = d->kernelCtx->getPluginManager().getPluginObjectDescCreating(id);
    }

    // 2 create BoxObject
    BoxObject *box = new BoxObject(d->kernelCtx, boxDetails);
    box->setCenter(position);
    d->boxesMap.insert(boxID, box);
    d->boxes.append(box);

    // 3 Check if box is a display box
    if (pluginDesc->hasFunctionality(Plugins::EPluginFunctionality::Visualization)) {
        d->getWindowManager()->addVisualizationBox(box);
        //emit displayBoxesChanged();
    }

    emit boxesChanged();
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
    if (d->player.isNull()) {
        // create a snapshot so settings override does not modify the scenario !
        //d->m_scenario.SnapshotCB(false);

        // set filename attribute to scenario so delayed configuration can be used
        if (d->m_scenario.hasAttribute(OV_AttributeId_ScenarioFilename)) {
            d->m_scenario.setAttributeValue(OV_AttributeId_ScenarioFilename, d->filePath.toLocalFile().toStdString().c_str());
        }
        else {
            d->m_scenario.addAttribute(OV_AttributeId_ScenarioFilename, d->filePath.toLocalFile().toStdString().c_str());
        }

        d->kernelCtx->getPlayerManager().createPlayer(d->playerID);
        d->player.setPlayer(&d->kernelCtx->getPlayerManager().getPlayer(d->playerID));
        if (!d->player.setScenario(d->scenarioID)) {
            d->kernelCtx->getPlayerManager().releasePlayer(d->playerID);
            d->player.setPlayer(nullptr);
            d->playerID = CIdentifier::undefined();
            d->player.clear();
            d->kernelCtx->getLogManager() << Kernel::LogLevel_Error << "The scenario could not be set to the player.\n";
        } else {
            // Sets window manager ID to runtime config. This way, visualization boxes can access it.
            // Needs to be set between setScenario() and initialize().
            d->player.setWindowManager(d->m_windowManagerID); 

            if (d->player.initialize() != Kernel::EPlayerReturnCodes::Success) {
                d->kernelCtx->getLogManager() << Kernel::LogLevel_Error << "The player could not be initialized.\n";
                d->kernelCtx->getPlayerManager().releasePlayer(d->playerID);
                d->player.setPlayer(nullptr);
                d->playerID = CIdentifier::undefined();
                d->player.clear();
            }
        }
    }

    if (!d->player.isNull()) {
        // Start cpu usage timer
        d->m_cpuUsageTimer.start();
    }

     return d->player;
}

void DesignerScenario::releasePlayer()
{
    d->m_cpuUsageTimer.stop();

    if(d->player.isNull()) {
        d->kernelCtx->getLogManager() << Kernel::LogLevel_Warning << "Trying to release non-initialized player\n";
        return;
    }

    d->kernelCtx->getErrorManager().releaseErrors();

    if (!d->player.uninitialize()) {
        d->kernelCtx->getLogManager() << Kernel::LogLevel_Error << "Failed to uninitialize the player" << "\n"; 
    }
    d->player.setPlayer(nullptr);

    if(!d->kernelCtx->getPlayerManager().releasePlayer(d->playerID)) {
        d->kernelCtx->getLogManager() << Kernel::LogLevel_Error << "Failed to release the player" << "\n";
    }
    d->playerID = CIdentifier::undefined();

    emit cpuUsageChanged();
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
    if (currentName != name) {
        d->m_scenario.setName(name.toStdString().c_str());
        emit nameChanged(name);
    }
}

void DesignerScenario::setFilePath(const QUrl& filePath)
{
    QUrl currentPath = d->filePath;
    if (currentPath != filePath) {
        d->filePath = filePath;

        //set the name of the scenario to the file name
        QFileInfo fileInfo(filePath.toLocalFile());
        this->setName(fileInfo.baseName());

        emit filePathChanged(filePath);
    }
}

QQmlListProperty<BoxObject> DesignerScenario::boxes()
{
    //first time we need the boxes, we fill the list
    if(d->boxes.isEmpty()) {
        d->fillBoxes();
    }
    d->kernelCtx->getLogManager() << Kernel::LogLevel_Info << "there are " << int(d->boxes.size()) << " boxes in the scenario \n";

    return QQmlListProperty<BoxObject>(this, &d->boxes);
}

QQmlListProperty<LinkObject> DesignerScenario::links()
{
    if(d->links.isEmpty()) {
        d->fillLinks();
    }
    d->kernelCtx->getLogManager() << Kernel::LogLevel_Info << "there are " << int(d->links.size()) << " links in the scenario \n";

    return QQmlListProperty<LinkObject>(this, &d->links);
}

void DesignerScenario::clickPort(int portIdx,const QString& boxId, qreal x, qreal y, bool isOutput)
{
    if(d->linkCreationStarted) {
        for(auto *box : d->boxes) {
            QString current_id = box->id().toString().toASCIIString();
            if(current_id == boxId) {
                //create the OpenVibe Link
                //first check if the connection is allowed
                CIdentifier newLinkId;
                CIdentifier boxSrcTypeID;
                CIdentifier boxDstTypeID;
                int boxSrcPort, boxDstPort;
                Kernel::IBox* srcBox = nullptr;
                Kernel::IBox* dstBox = nullptr;
                if(d->linkCreationSourceIsOutput) {
                    srcBox = d->m_scenario.getBoxDetails(d->linkCreationSource->id());
                    dstBox = d->m_scenario.getBoxDetails(box->id());
                    boxSrcTypeID = d->linkCreationSource->id();
                    boxDstTypeID = box->id();
                    boxSrcPort = d->linkCreationSourcePort;
                    boxDstPort = portIdx;
                } else {
                    srcBox = d->m_scenario.getBoxDetails(box->id());
                    dstBox = d->m_scenario.getBoxDetails(d->linkCreationSource->id());
                    boxSrcTypeID = box->id();
                    boxDstTypeID = d->linkCreationSource->id();
                    boxSrcPort = portIdx;
                    boxDstPort = d->linkCreationSourcePort;
                }
                
                if(!srcBox) {
                    d->kernelCtx->getLogManager() << Kernel::LogLevel_Error << "Failed to find the box details for " << boxSrcTypeID.toString() << " as a Source \n";
                    return;
                }
                if(!dstBox) {
                    d->kernelCtx->getLogManager() << Kernel::LogLevel_Error << "Failed to find the box details for " << boxDstTypeID.toString() << " as a Target \n";
                    return;
                }

                CIdentifier srcTypeID;
                CIdentifier dstTypeID;
                srcBox->getOutputType(boxSrcPort, srcTypeID);
                dstBox->getInputType(boxDstPort, dstTypeID);
                
                //no deprecated input or output
                bool hasDeprecatedIO = false;
                srcBox->getInterfacorDeprecatedStatus(Kernel::Output, boxSrcPort, hasDeprecatedIO);
                if(hasDeprecatedIO) {
                    d->kernelCtx->getLogManager() << Kernel::LogLevel_Warning << "Cannot connect from deprecated Output :" << boxSrcTypeID.toString() << " \n";
                    return;
                }
                dstBox->getInterfacorDeprecatedStatus(Kernel::Input, boxDstPort, hasDeprecatedIO);
                if(hasDeprecatedIO) {
                    d->kernelCtx->getLogManager() << Kernel::LogLevel_Warning << "Cannot connect to deprecated Input : " << boxDstTypeID.toString()  << " \n";
                    return;
                }

                //Do the connection
                if ((d->kernelCtx->getTypeManager().isDerivedFromStream(srcTypeID, dstTypeID)
                         || d->kernelCtx->getConfigurationManager().expandAsBoolean("${Designer_AllowUpCastConnection}", false))) {
                    if(!d->m_scenario.connect(newLinkId, boxSrcTypeID, boxSrcPort, boxDstTypeID, boxDstPort, CIdentifier::undefined())) {
                        d->kernelCtx->getLogManager() << Kernel::LogLevel_Error << "Failed to create link between " << d->linkCreationSource->name().toStdString()
                                                      << " and " << boxId.toStdString() << "\n";
                        return;
                    }
                }
                else {
                    d->kernelCtx->getLogManager() << Kernel::LogLevel_Error << "Failed to create link between " << d->linkCreationSource->name().toStdString()
                                                      << " and " << boxId.toStdString() << "\n";
                    return;
                }
                
                //the method connect may delete an existing link when creating a new one (e.g. if destination port was already connected to another box)!!
                //we should update all our links if the number of links in the scenario has changed
                int nbLinks = 0;
                CIdentifier linkId = d->m_scenario.getNextLinkIdentifier(CIdentifier::undefined());
                while(linkId != CIdentifier::undefined()) {
                    nbLinks++;
                    linkId = d->m_scenario.getNextLinkIdentifier(linkId);
                }

                if((d->links.size() + 1) != nbLinks) {
                    qDeleteAll(d->links);
                    d->links.clear();
                    d->fillLinks();
                } else {
                    Kernel::ILink *iLink = d->m_scenario.getLinkDetails(newLinkId);

                    //create the link object
                    LinkObject *link = nullptr;
                    if(d->linkCreationSourceIsOutput) {
                        link = new LinkObject(d->kernelCtx, iLink, d->linkCreationSource, box);
                    } else {
                        link = new LinkObject(d->kernelCtx, iLink, box, d->linkCreationSource);
                    }
                    d->links.append(link);
                    
                    emit linksChanged();
                }

                //link creation is ok and finished
                //stop the link creation mode
                this->stopLinkCreation();
                return;
            }
        }
        d->kernelCtx->getLogManager() << Kernel::LogLevel_Error << "Failed to find the box " << boxId.toStdString() << " as a Target \n";
        emit finishLinkCreation();
        return;
    }

    for(auto *box : d->boxes) {
        QString current_id = box->id().toString().toASCIIString();
        if(current_id == boxId) {
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
    for(int i=0; i < d->links.length(); ++i) {
        auto *link = d->links.at(i);
        BoxObject *box = nullptr;
        int port = -1;
        if(isOutput) {
            box = link->m_sourceBox;
            port = link->m_link->getSourceBoxOutputIndex();
        } else {
            box = link->m_targetBox;
            port = link->m_link->getTargetBoxInputIndex();
        }
        QString current_id = box->id().toString().toASCIIString();

        if((current_id == boxId) && (port == portIdx)) {
            //remove the link from the scenario
            this->removeLink(i);
            return;
        }
    }
    // there is no link to remove, do nothing!
    d->kernelCtx->getLogManager() << Kernel::LogLevel_Warning << "Failed to find the link to remove\n";
}

void DesignerScenario::removeLink(int linkIdx)
{
    LinkObject *link = d->links.takeAt(linkIdx);
    if(!d->m_scenario.disconnect(link->m_link->getIdentifier())) {
        d->kernelCtx->getLogManager() << Kernel::LogLevel_Error << "Failed to remove link between " << link->m_sourceBox->name().toStdString()
                                      << " and " << link->m_targetBox->name().toStdString() << "\n";
    }

    //stop any link creation previously started
    this->stopLinkCreation();

    //remove the link from the list
    delete link;
    emit linksChanged();
    return;
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
    for(auto* b : d->boxes) {
        if(b->id().toString() == identifier) {
            box = b;
            break;
        }
    }

    if(!box) {
        return;
    }

    // Remove box from visualization boxes
    // Won't be found if not visualization box
    d->getWindowManager()->removeVisualizationBox(box);

    // Remove the box
    if(!d->m_scenario.removeBox(box->id())) {
        d->kernelCtx->getLogManager() << Kernel::LogLevel_Error << "Failed to remove box " << box->name().toStdString() << "\n";
    }

    //remove all linkObjects connected to this box
    auto it = d->links.begin();
    while(it != d->links.end()) {
        LinkObject *link = *it;
        if((link->m_sourceBox == box) || (link->m_targetBox == box)) {
            it = d->links.erase(it);
            delete link;
        } else {
            ++it;
        }
    }

    auto it2 = d->m_scenarioIOConnections.begin();
    while (it2 != d->m_scenarioIOConnections.end()) {
        ScenarioIOConnection *connection = *it2;
        if (connection->m_targetBox == box) {
            it2 = d->m_scenarioIOConnections.erase(it2);
            delete connection;
        } else {
            ++it2;
        }
    }

    //remove the box from the list
    d->boxes.remove(d->boxes.indexOf(box));
    delete box;
    emit boxesChanged();
    emit linksChanged();
    emit scenarioIOConnectionsChanged();
    return;

}

QJSValue DesignerScenario::parameters() const
{
    QQmlEngine *jsEngine = QmlEngineSingleton::getInstance().engine();
    if(!jsEngine) {
        qWarning() << Q_FUNC_INFO << "Could not obtain QQmlEngine for object.\n";
        return QJSValue();
    }

    int nbParams = d->m_parameters.size();
    QJSValue jsParams = jsEngine->newArray(nbParams);
    for (int i = 0; i < nbParams; ++i) {
        jsParams.setProperty(i, jsEngine->newQObject(d->m_parameters[i]));
    }

    return jsParams;
}

QStringList DesignerScenario::getAvailableParameterTypes() const {
    QStringList types;
    std::vector<std::string> availableTypes;
    d->m_scenario.getAvailableParameterTypes(availableTypes);
    for (const auto& type : availableTypes) {
        types << QString::fromStdString(type);
    }
    return types;
}

void DesignerScenario::addParameter(const QString& name, const QString& type)
{
    d->m_scenario.addParameter(name.toStdString(), type.toStdString());

    d->updateParameters();
    emit parametersChanged();
}

void DesignerScenario::removeParameter(const QString& name)
{
    d->m_scenario.removeParameter(name.toStdString());
    d->updateParameters();
    emit parametersChanged();
}

void DesignerScenario::updateParameterName(const QString& oldName, const QString& newName)
{
    d->m_scenario.updateParameterName(oldName.toStdString(), newName.toStdString());
    emit parametersChanged();
}

QQmlListProperty<ScenarioParameterConnectionObject> DesignerScenario::getParameterConnectionsForParam(const QString& scenarioParameterName)
{

    // Todo: retrieve connections form scenario in a single call. This will bring consistency in the order of appearance in the UI.
    std::vector<Kernel::CScenarioParameterConnection*> parameterConnections;
    d->m_scenario.getParameterConnections(scenarioParameterName.toStdString(), parameterConnections);

    for (auto& parameterConnection : d->m_parameterConnections) {
        delete parameterConnection;
    }
    d->m_parameterConnections.clear();

    for (auto& parameterConnection : parameterConnections) {
        d->m_parameterConnections.append(new ScenarioParameterConnectionObject(d->kernelCtx,
                                                                               parameterConnection->getScenarioParameterName().c_str(),
                                                                               parameterConnection->getBoxID().str().c_str(),
                                                                               parameterConnection->getBoxParameterName().c_str(),
                                                                               true));
    }

    std::vector<Kernel::CScenarioParameterConnection> parameterConnectionCandidates;
    d->m_scenario.getParameterConnectionCandidates(scenarioParameterName.toStdString(), parameterConnectionCandidates);
    for (auto& parameterConnection : parameterConnectionCandidates) {
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
    if (connect) {
        d->m_scenario.addParameterConnection(scenarioParameterName.toStdString(), boxID, boxParameterName.toStdString().c_str());
    } else {
        d->m_scenario.removeParameterConnection(scenarioParameterName.toStdString(), boxID, boxParameterName.toStdString().c_str());
    }
}

QQmlListProperty<CommentObject> DesignerScenario::comments()
{
    if(d->comments.isEmpty()) {
        CIdentifier commentId = d->m_scenario.getNextCommentIdentifier(CIdentifier::undefined());
        while(commentId != CIdentifier::undefined()) {
            Kernel::IComment *comment = d->m_scenario.getCommentDetails(commentId);

            d->comments.append(new CommentObject(d->kernelCtx, comment));//  comment->getText().toASCIIString());
            commentId = d->m_scenario.getNextCommentIdentifier(commentId);
        }
    }

    return QQmlListProperty<CommentObject>(this, &d->comments);

}

void DesignerScenario::addComment(const QString& text, const QPoint& origin)
{
    // 1 create OpenViBE comment
    CIdentifier commentID;
    if(!d->m_scenario.addComment(commentID, CIdentifier::undefined())) {
        d->kernelCtx->getLogManager() << Kernel::LogLevel_Error << "Failed to add comment " << text.toStdString() << "\n";
        return;
    }
    Kernel::IComment *commentDetails = d->m_scenario.getCommentDetails(commentID);
    CommentObject *comment = new CommentObject(d->kernelCtx, commentDetails);

    comment->setText(text);
    comment->setCenter(origin);

    // 2 create CommentObject
    d->comments.append(comment);

    emit commentsChanged();
}

void DesignerScenario::removeComment(int commentIdx)
{
    CommentObject *comment = d->comments.takeAt(commentIdx);
    if(!d->m_scenario.removeComment(comment->id())) {
        d->kernelCtx->getLogManager() << Kernel::LogLevel_Error << "Failed to remove comment " << comment->text().toStdString() << "\n";
    }

    //remove the comment from the list
    delete comment;
    emit commentsChanged();
    return;
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
    for (const auto& type : sortedTypes) {
        if (d->kernelCtx->getTypeManager().isStream(type.first) && d->m_scenario.hasInputSupport(type.first)) {
            supportedTypes.push_back(QString::fromStdString(type.second.toASCIIString()));
        }
    }

    return supportedTypes;
}

void DesignerScenario::addInput(const QString& name, const QString& typeId)
{
    CIdentifier cidTypeId = d->kernelCtx->getTypeManager().getTypeFromName(typeId.toStdString().c_str());
    CIdentifier inputId = d->m_scenario.getUnusedInputIdentifier();
    if(!d->m_scenario.addInput(name.toStdString().c_str(), cidTypeId, inputId, true)) {
        d->kernelCtx->getLogManager() << Kernel::LogLevel_Warning << "Could not add input port to box " << d->m_scenario.getName() << "\n";
        return;
    }

    //A Listener may have been called in which case, the input port list has been updated
    if(d->m_inputPorts.size() == d->m_scenario.getInputCount()) {
        return;
    }

    d->m_inputPorts.push_back(new IOPort(d->kernelCtx, &d->m_scenario, IOPort::PortType::Input, d->m_inputPorts.size()));
    emit inputPortsChanged();
}

void DesignerScenario::removeInput(int portIndex)
{
    d->m_scenario.removeInput(portIndex);

    //calling IBox::removeInput deletes all the links and recreate them..
    //so we need to update all links
    for(auto* port: d->m_inputPorts) {
        delete port;
    }
    d->m_inputPorts.clear();

    for(size_t i=0; i < d->m_scenario.getInputCount(); i++) {
        d->m_inputPorts.push_back(new IOPort(d->kernelCtx, &d->m_scenario, IOPort::PortType::Input, i));
    }
    emit inputPortsChanged();
}

QList<QString> DesignerScenario::getSupportedOutputTypes() const
{
    QList<QString> supportedTypes;
    std::vector<std::pair<CIdentifier, CString>> sortedTypes = d->kernelCtx->getTypeManager().getSortedTypes();
    for (const auto& type : sortedTypes) {
        if (d->kernelCtx->getTypeManager().isStream(type.first) && d->m_scenario.hasOutputSupport(type.first)) {
            supportedTypes.push_back(QString::fromStdString(type.second.toASCIIString()));
        }
    }

    return supportedTypes;
}

void DesignerScenario::addOutput(const QString& name, const QString& typeId)
{
    CIdentifier cidTypeId = d->kernelCtx->getTypeManager().getTypeFromName(typeId.toStdString().c_str());
    CIdentifier outputId = d->m_scenario.getUnusedOutputIdentifier();
    if(!d->m_scenario.addOutput(name.toStdString().c_str(), cidTypeId, outputId, true)) {
        d->kernelCtx->getLogManager() << Kernel::LogLevel_Warning << "Could not add output port to box " << d->m_scenario.getName() << "\n";
        return;
    }

    //A Listener may have been called in which case, the output port list has been updated
    if(d->m_outputPorts.size() == d->m_scenario.getOutputCount()) {
        return;
    }

    d->m_outputPorts.push_back(new IOPort(d->kernelCtx, &d->m_scenario, IOPort::PortType::Output, d->m_outputPorts.size()));
    emit outputPortsChanged();
}

void DesignerScenario::removeOutput(int portIndex)
{
    d->m_scenario.removeOutput(portIndex);

    //calling IBox::removeOutput deletes all the links and recreate them..
    //so we need to update all links
    for(auto* port: d->m_outputPorts) {
        delete port;
    }
    d->m_outputPorts.clear();

    for(size_t i=0; i < d->m_scenario.getOutputCount(); i++) {
        d->m_outputPorts.push_back(new IOPort(d->kernelCtx, &d->m_scenario, IOPort::PortType::Output, i));
    }
    emit outputPortsChanged();
}

WindowManager* DesignerScenario::windowManager() {
    return d->getWindowManager();
}

double DesignerScenario::cpuUsage() const {
    return d->player.getCPUUsage();
}

void DesignerScenario::setScenarioInputLink(int portIndex, const QString& boxId, int boxInputIndex) {
    CIdentifier cidBoxId = CIdentifier(boxId.toStdString().c_str());
    if (d->m_scenario.setScenarioInputLink(portIndex, cidBoxId, boxInputIndex)) {
        // Check and clear any conflicting existing connections
        auto it = d->m_scenarioIOConnections.begin();
        while (it != d->m_scenarioIOConnections.end()) {
            ScenarioIOConnection *connection = *it;
            if (connection->isOutput() == false // only check inputs
                && (connection->scenarioPortIdx() == portIndex  // scenario port index already used
                    || (connection->targetBoxId() == boxId  // or box port already connected
                        && connection->targetPortIdx() == boxInputIndex)
                )) {
                it = d->m_scenarioIOConnections.erase(it);
                delete connection;
            } else {
                ++it;
            }
        }

        // Add new connection to model
        d->m_scenarioIOConnections.append(new ScenarioIOConnection(d->kernelCtx, d->m_inputPorts.at(portIndex), d->boxesMap.value(cidBoxId), boxInputIndex, false));
        emit scenarioIOConnectionsChanged();
    }
}

void DesignerScenario::setScenarioOutputLink(int portIndex, const QString& boxId, int boxOutputIndex) {
    CIdentifier cidBoxId = CIdentifier(boxId.toStdString().c_str());
    if (d->m_scenario.setScenarioOutputLink(portIndex, cidBoxId, boxOutputIndex)) {

        // Check and clear any conflicting existing connection
        auto it = d->m_scenarioIOConnections.begin();
        while (it != d->m_scenarioIOConnections.end()) {
            ScenarioIOConnection *connection = *it;
            if (connection->isOutput() == true // only check outputs
                && (connection->scenarioPortIdx() == portIndex  // scenario port index already used
                    || (connection->targetBoxId() == boxId  // or box port already connected
                        && connection->targetPortIdx() == boxOutputIndex)
                )) {
                it = d->m_scenarioIOConnections.erase(it);
                delete connection;
            } else {
                ++it;
            }
        }

        // Add new connection to model
        d->m_scenarioIOConnections.append(new ScenarioIOConnection(d->kernelCtx, d->m_outputPorts.at(portIndex), d->boxesMap.value(cidBoxId), boxOutputIndex, true));
        emit scenarioIOConnectionsChanged();
    }
}

void DesignerScenario::removeScenarioIOConnection(int portIndex, const QString& boxId, int boxPortIndex, bool isOutput) {
    bool res = false;
    CIdentifier cidBoxId = CIdentifier(boxId.toStdString().c_str());
    if (isOutput) {
        res = d->m_scenario.removeScenarioOutputLink(portIndex, cidBoxId, boxPortIndex);
    } else {
        res = d->m_scenario.removeScenarioInputLink(portIndex, cidBoxId, boxPortIndex);
    }
    if (res) {
        auto it = std::find_if(d->m_scenarioIOConnections.begin(), d->m_scenarioIOConnections.end(), [portIndex, boxId, boxPortIndex, isOutput](const auto& c) {
            return c->scenarioPortIdx() == portIndex
                && c->targetPortIdx() == boxPortIndex 
                && c->targetBoxId() == boxId
                && c->isOutput() == isOutput;
        });
        if (it != d->m_scenarioIOConnections.end()) {
            delete *it;
            d->m_scenarioIOConnections.erase(it);
        }
        emit scenarioIOConnectionsChanged();
    } else {
        d->kernelCtx->getLogManager() << Kernel::LogLevel_Error << "Failed to remove scenario IO connection " << portIndex << " of box " << boxId.toStdString() << "\n";
    }
}

}
}
