#include "ovkCScenario.h"
#include "ovkCScenarioSettingKeywordParserCallback.h"

#include "ovkTBox.hpp"
#include "ovkCBoxUpdater.h"
#include "ovkCComment.h"
#include "ovkCMetadata.h"
#include "ovkCLink.h"

#include "../ovkCObjectVisitorContext.h"
#include "../../tools/ovk_setting_checker.h"

#include <iostream>
#include <cstdlib>
#include <algorithm>

#define OV_AttributeId_Box_Disabled                         OpenViBE::CIdentifier(0x341D3912, 0x1478DE86)
#define OVD_AttributeId_SettingOverrideFilename             OpenViBE::CIdentifier(0x8D21FF41, 0xDF6AFE7E)

// The following is a hack, can be removed once there is a copy constructor for scenarios, boxes, etc
#include <ovp_global_defines.h>
//___________________________________________________________________//
//                                                                   //
namespace OpenViBE {
namespace Kernel {

namespace {
template <class T>
struct STestTrue
{
    bool operator()(typename std::map<CIdentifier, T>::const_iterator /*it*/) const { return true; }
};

struct STestEqSourceBox
{
    explicit STestEqSourceBox(const CIdentifier& boxId) : m_BoxId(boxId) { }
    bool operator()(const std::map<CIdentifier, CLink*>::const_iterator& it) const { return it->second->getSourceBoxIdentifier() == m_BoxId; }
    const CIdentifier& m_BoxId;
};

struct STestEqSourceBoxOutput
{
    STestEqSourceBoxOutput(const CIdentifier& boxId, const size_t index) : m_BoxId(boxId), m_OutputIdx(index) { }

    bool operator()(const std::map<CIdentifier, CLink*>::const_iterator& it) const
    {
        return it->second->getSourceBoxIdentifier() == m_BoxId && it->second->getSourceBoxOutputIndex() == m_OutputIdx;
    }

    const CIdentifier& m_BoxId;
    size_t m_OutputIdx;
};

struct STestEqTargetBox
{
    explicit STestEqTargetBox(const CIdentifier& boxId) : m_BoxId(boxId) { }
    bool operator()(const std::map<CIdentifier, CLink*>::const_iterator& it) const { return it->second->getTargetBoxIdentifier() == m_BoxId; }
    const CIdentifier& m_BoxId;
};

struct STestEqTargetBoxInput
{
    STestEqTargetBoxInput(const CIdentifier& boxId, const size_t index) : m_BoxId(boxId), m_InputIdx(index) { }

    bool operator()(const std::map<CIdentifier, CLink*>::const_iterator& it) const
    {
        return it->second->getTargetBoxIdentifier() == m_BoxId && it->second->getTargetBoxInputIndex() == m_InputIdx;
    }

    const CIdentifier& m_BoxId;
    size_t m_InputIdx;
};

template <class T, class TTest>
CIdentifier getNextID(const std::map<CIdentifier, T>& elementMap, const CIdentifier& previousID, const TTest& testFunctor)
{
    typename std::map<CIdentifier, T>::const_iterator it;

    if (previousID == CIdentifier::undefined()) { it = elementMap.begin(); }
    else
    {
        it = elementMap.find(previousID);
        if (it == elementMap.end()) { return CIdentifier::undefined(); }
        ++it;
    }

    while (it != elementMap.end())
    {
        if (testFunctor(it)) { return it->first; }
        ++it;
    }

    return CIdentifier::undefined();
}

/*
template <class T, class TTest>
CIdentifier getNextID(const std::map<CIdentifier, T*>& elementMap, const CIdentifier& previousID, const TTest& testFunctor)
{
    typename std::map<CIdentifier, T*>::const_iterator it;

    if(previousID==CIdentifier::undefined())
    {
        it=elementMap.begin();
    }
    else
    {
        it=elementMap.find(previousID);
        if(it==elementMap.end()) { return CIdentifier::undefined(); }
        ++it;
    }

    while(it!=elementMap.end())
    {
        if(testFunctor(it)) { return it->first; }
        ++it;
    }

    return CIdentifier::undefined();
}
*/
}  // namespace

//___________________________________________________________________//
//                                                                   //

CScenario::CScenario(const IKernelContext& ctx, const CIdentifier& identifier) : TBox<IScenario>(ctx), m_firstMetadataID(CIdentifier::undefined())
{
    // Some operations on boxes manipulate the owner scenario, for example removing inputs
    // by default we set the scenario as owning itself to avoid segfaults
    this->setOwnerScenario(this);
    this->m_identifier = identifier;
}

//___________________________________________________________________//
//                                                                   //

bool CScenario::clear()
{
    this->getLogManager() << LogLevel_Info << "Clearing scenario\n";

    for (auto& box : m_boxes) { delete box.second; }
    m_boxes.clear();

    for (auto& comment : m_comments) { delete comment.second; }
    m_comments.clear();

    for (auto& metadata : m_metadatas) { delete metadata.second; }
    m_metadatas.clear();
    m_firstMetadataID = CIdentifier::undefined();
    m_nextMetadataID.clear();

    for (auto& link : m_links) { delete link.second; }
    m_links.clear();

    m_outdatedBoxes.clear();

    while (this->getSettingCount()) { this->removeSetting(0); }
    while (this->getInputCount()) { this->removeScenarioInput(0); }
    while (this->getOutputCount()) { this->removeScenarioOutput(0); }

    this->removeAllAttributes();

    return true;
}

bool CScenario::removeScenarioInput(const size_t index)
{
    OV_ERROR_UNLESS_KRF(index < this->getInputCount(), "Input index = [" << index << "] is out of range (max index = [" << (this->getInputCount() - 1) << "])",
                        ErrorType::OutOfBound);

    this->removeInput(index);

    // Remove the link within the scenario to this input
    if (index < m_scenarioInputLinks.size()) { m_scenarioInputLinks.erase(m_scenarioInputLinks.begin() + index); }

    return true;
}

bool CScenario::removeScenarioOutput(const size_t index)
{
    OV_ERROR_UNLESS_KRF(index < this->getOutputCount(),
                        "Output index = [" << index << "] is out of range (max index = [" << (this->getOutputCount() - 1) << "])",
                        Kernel::ErrorType::OutOfBound);

    this->removeOutput(index);

    // Remove the link within the scenario to this output
    if (index < m_scenarioOutputLinks.size()) { m_scenarioOutputLinks.erase(m_scenarioOutputLinks.begin() + index); }

    return true;
}

bool CScenario::merge(const IScenario& scenario, IScenarioMergeCallback* scenarioMergeCallback, const bool mergeSettings, const bool preserveIDs)
{
    std::map<CIdentifier, CIdentifier> oldToNewIdMap;

    // Copy boxes
    {
        CIdentifier* listID = nullptr;
        size_t nbElems      = 0;
        scenario.getBoxIdentifierList(&listID, &nbElems);
        for (size_t i = 0; i < nbElems; ++i)
        {
            CIdentifier boxID = listID[i];
            const IBox* box   = scenario.getBoxDetails(boxID);
            CIdentifier newID;
            CIdentifier suggestedNewID = preserveIDs ? box->getIdentifier() : CIdentifier::undefined();
            this->addBox(newID, *box, suggestedNewID);
            oldToNewIdMap[boxID] = newID;

            if (scenarioMergeCallback) { scenarioMergeCallback->process(boxID, newID); }
        }
        scenario.releaseIdentifierList(listID);
    }

    // Copy links
    {
        CIdentifier* listID = nullptr;
        size_t nbElems      = 0;
        scenario.getLinkIdentifierList(&listID, &nbElems);
        for (size_t i = 0; i < nbElems; ++i)
        {
            CIdentifier linkID = listID[i];
            const ILink* link  = scenario.getLinkDetails(linkID);
            CIdentifier newID;
            this->connect(newID, oldToNewIdMap[link->getSourceBoxIdentifier()],
                          link->getSourceBoxOutputIndex(), oldToNewIdMap[link->getTargetBoxIdentifier()],
                          link->getTargetBoxInputIndex(), CIdentifier::undefined());

            if (scenarioMergeCallback) { scenarioMergeCallback->process(linkID, newID); }
        }
        scenario.releaseIdentifierList(listID);
    }
    // Copy comments

    // Copy metadata
    {
        CIdentifier* listID = nullptr;
        size_t nbElems      = 0;
        scenario.getMetadataIdentifierList(&listID, &nbElems);
        for (size_t i = 0; i < nbElems; ++i)
        {
            CIdentifier metadataID    = listID[i];
            const IMetadata* metadata = scenario.getMetadataDetails(metadataID);
            CIdentifier newID;
            CIdentifier suggestedNewID = preserveIDs ? metadataID : CIdentifier::undefined();
            this->addMetadata(newID, suggestedNewID);
            IMetadata* newMetadata = this->getMetadataDetails(newID);
            newMetadata->initializeFromExistingMetadata(*metadata);
        }
        scenario.releaseIdentifierList(listID);
    }

    // Copy settings if requested

    const size_t previousSettingCount = this->getSettingCount();

    if (mergeSettings)
    {
        for (size_t i = 0; i < scenario.getSettingCount(); ++i)
        {
            CIdentifier typeID;
            CString name;
            CString defaultValue;
            CString value;
            bool isModifiable;
            CIdentifier id;
            scenario.getSettingType(i, typeID);
            scenario.getSettingName(i, name);
            scenario.getSettingDefaultValue(i, defaultValue);
            scenario.getSettingValue(i, value);
            scenario.getSettingMod(i, isModifiable);
            scenario.getInterfacorIdentifier(Setting, i, id);

            this->addSetting(name, typeID, defaultValue, size_t(-1), isModifiable, id, true);
            this->setSettingValue(previousSettingCount + i, value);
        }

        // In this case we also merge the attributes
        CIdentifier attributeIdentifier;
        while ((attributeIdentifier = scenario.getNextAttributeIdentifier(attributeIdentifier)) != CIdentifier::undefined())
        {
            CString attributeValue = scenario.getAttributeValue(attributeIdentifier);
            this->addAttribute(attributeIdentifier, attributeValue);
        }
    }

    return true;
}

//___________________________________________________________________//
//                                                                   //

CIdentifier CScenario::getNextBoxIdentifier(const CIdentifier& previousID) const
{
    return getNextID<CBox*, STestTrue<CBox*>>(m_boxes, previousID, STestTrue<CBox*>());
}

const IBox* CScenario::getBoxDetails(const CIdentifier& boxID) const
{
    const auto it = m_boxes.find(boxID);
    OV_ERROR_UNLESS_KRN(it != m_boxes.end(), "Box [" << boxID.str() << "] is not part of the scenario", Kernel::ErrorType::ResourceNotFound);
    return it->second;
}

bool CScenario::isBox(const CIdentifier& boxID) const { return m_boxes.count(boxID) == 1; }

IBox* CScenario::getBoxDetails(const CIdentifier& boxID)
{
    // this->getLogManager() << Kernel::LogLevel_Debug << "Getting box details from scenario\n";
    const auto it = m_boxes.find(boxID);
    OV_ERROR_UNLESS_KRN(it != m_boxes.end(), "Box [" << boxID.str() << "] is not part of the scenario", Kernel::ErrorType::ResourceNotFound);
    return it->second;
}

bool CScenario::addBox(CIdentifier& boxID, const CIdentifier& suggestedBoxID)
{
    boxID     = getUnusedIdentifier(suggestedBoxID);
    CBox* box = new CBox(this->getKernelContext());
    box->setOwnerScenario(this);
    box->setIdentifier(boxID);

    m_boxes[boxID] = box;
    return true;
}

bool CScenario::addBox(CIdentifier& boxID, const IBox& box, const CIdentifier& suggestedBoxID)
{
    if (!addBox(boxID, suggestedBoxID))
    {
        // error is handled in addBox
        return false;
    }

    IBox* newBox = getBoxDetails(boxID);
    if (!newBox)
    {
        // error is handled in getBoxDetails
        return false;
    }

    return newBox->initializeFromExistingBox(box);
}

bool CScenario::addBox(CIdentifier& boxID, const CIdentifier& boxAlgorithmID, const CIdentifier& suggestedBoxID)
{
    if (!addBox(boxID, suggestedBoxID))
    {
        // error is handled in addBox
        return false;
    }

    IBox* newBox = getBoxDetails(boxID);
    if (!newBox)
    {
        // error is handled in getBoxDetails
        return false;
    }

    return newBox->initializeFromAlgorithmClassIdentifier(boxAlgorithmID);
}

bool CScenario::addBox(CIdentifier& boxID, const Plugins::IBoxAlgorithmDesc& boxAlgorithmDesc, const CIdentifier& suggestedBoxID)
{
    if (!addBox(boxID, suggestedBoxID))
    {
        // error is handled in addBox
        return false;
    }

    IBox* newBox = getBoxDetails(boxID);
    if (!newBox)
    {
        // error is handled in getBoxDetails
        return false;
    }

    return dynamic_cast<CBox*>(newBox)->initializeFromBoxAlgorithmDesc(boxAlgorithmDesc);
}

bool CScenario::removeBox(const CIdentifier& boxID)
{
    // Finds the box according to its identifier
    auto itBox = m_boxes.find(boxID);

    OV_ERROR_UNLESS_KRF(itBox != m_boxes.end(), "Box [" << boxID.str() << "] is not part of the scenario", Kernel::ErrorType::ResourceNotFound);

    // Find all the links that are used by this box
    auto itLink = m_links.begin();
    while (itLink != m_links.end())
    {
        auto itLinkCurrent = itLink;
        ++itLink;

        if (itLinkCurrent->second->getSourceBoxIdentifier() == boxID || itLinkCurrent->second->getTargetBoxIdentifier() == boxID)
        {
            // Deletes this link
            delete itLinkCurrent->second;

            // Removes link from the link list
            m_links.erase(itLinkCurrent);
        }
    }

    clearBoxParameterConnections(boxID);

    // Deletes the box itself
    delete itBox->second;

    // Removes box from the box list
    m_boxes.erase(itBox);

    return true;
}

//___________________________________________________________________//

CIdentifier CScenario::getNextCommentIdentifier(const CIdentifier& previousID) const
{
    return getNextID<CComment*, STestTrue<CComment*>>(m_comments, previousID, STestTrue<CComment*>());
}

const IComment* CScenario::getCommentDetails(const CIdentifier& commentID) const
{
    const auto it = m_comments.find(commentID);
    OV_ERROR_UNLESS_KRN(it != m_comments.end(), "Comment [" << commentID.str() << "] is not part of the scenario", Kernel::ErrorType::ResourceNotFound);
    return it->second;
}

bool CScenario::isComment(const CIdentifier& commentID) const { return m_comments.find(commentID) != m_comments.end(); }

IComment* CScenario::getCommentDetails(const CIdentifier& commentID)
{
    const auto it = m_comments.find(commentID);
    OV_ERROR_UNLESS_KRN(it != m_comments.end(), "Comment [" << commentID.str() << "] is not part of the scenario", Kernel::ErrorType::ResourceNotFound);
    return it->second;
}

bool CScenario::addComment(CIdentifier& commentID, const CIdentifier& suggestedCommentID)
{
    commentID            = getUnusedIdentifier(suggestedCommentID);
    CComment* newComment = new CComment(this->getKernelContext(), *this);
    newComment->setIdentifier(commentID);

    m_comments[commentID] = newComment;
    return true;
}

bool CScenario::addComment(CIdentifier& commentID, const IComment& comment, const CIdentifier& suggestedCommentID)
{
    if (!addComment(commentID, suggestedCommentID))
    {
        // error is handled in addComment above
        return false;
    }

    IComment* newComment = getCommentDetails(commentID);
    if (!newComment)
    {
        // error is handled in getCommentDetails
        return false;
    }

    return newComment->initializeFromExistingComment(comment);
}

bool CScenario::removeComment(const CIdentifier& commentID)
{
    // Finds the comment according to its identifier
    auto it = m_comments.find(commentID);
    OV_ERROR_UNLESS_KRF(it != m_comments.end(), "Comment [" << commentID.str() << "] is not part of the scenario", Kernel::ErrorType::ResourceNotFound);
    delete it->second;      // Deletes the comment itself
    m_comments.erase(it);   // Removes comment from the comment list
    return true;
}

void CScenario::getAvailableParameterTypes(std::vector<std::string>& types) {
    types.push_back("bool");
    types.push_back("int");
    types.push_back("float");
    types.push_back("string");
    types.push_back("path");
    types.push_back("stimulation");
}

void CScenario::addParameter(const std::string& name, const std::string& type, const std::any& value) {
    this->getLogManager() << Kernel::LogLevel_Info << "Adding parameter " << name << " of type " << type << "\n";
    CString cValue = "";
    CIdentifier cType = CIdentifier::undefined();

    if (type == "bool") {
        cType = OV_TypeId_Boolean;
        cValue = std::to_string(std::any_cast<bool>(value)).c_str();
    } else if (type == "int") {
        cType = OV_TypeId_Integer;
        cValue = std::to_string(std::any_cast<int>(value)).c_str();
    } else if (type == "float") {
        cType = OV_TypeId_Float;
        cValue = std::to_string(std::any_cast<double>(value)).c_str();
    } else if (type == "string") {
        cType = OV_TypeId_String;
        cValue = std::any_cast<std::string>(value).c_str();
    } else if (type == "path") {
        cType = OV_TypeId_Filename;
        cValue = std::any_cast<std::string>(value).c_str();
    } else if (type == "stimulation") {
        cType = OV_TypeId_Stimulation;
        cValue = std::any_cast<std::string>(value).c_str();
    } else {
        this->getLogManager() << Kernel::LogLevel_Warning << "unsupported type: " << type << " not supported yet\n";
        return;
    }

    this->addSetting(name.c_str(), cType, cValue, -1, false, CIdentifier::undefined() ,true);

}

void CScenario::addParameter(const std::string& name, const std::string& type) {
    std::any value;
    if (type == "bool") {
        value = false;
    } else if (type == "int") {
        value = 0;
    } else if (type == "float") {
        value = 0.0;
    } else if (type == "string") {
        value = std::string("");
    } else if (type == "path") {
        value = std::string("");
    } else if (type == "stimulation") {
        value = std::string("OVTK_StimulationId_Label_00");
    } else {
        this->getLogManager() << Kernel::LogLevel_Warning << "unsupported type: " << type << " not supported yet";
        return;
    }

    this->addParameter(name, type, value);
}

void CScenario::removeParameter(const std::string& parameterName) {
    size_t index;
    if (!this->getInterfacorIndex(EBoxInterfacorType::Setting, parameterName.c_str(), index)) {
        this->getLogManager() << Kernel::LogLevel_Error << "CScenario::removeParameter: parameter " << parameterName << " not found";
    }

    this->removeSetting(index);
    clearParameterConnections(parameterName);
}

void CScenario::updateParameterName(const std::string& oldName, const std::string& newName) {
    size_t index;
    if (!this->getInterfacorIndex(EBoxInterfacorType::Setting, oldName.c_str(), index)) {
        this->getLogManager() << Kernel::LogLevel_Error << "CScenario::updateParameterName: parameter " << oldName << " not found";
    }

    this->setSettingName(index, newName.c_str());
    updateParameterConnections(oldName, newName);
}

void CScenario::addParameterConnection(const std::string& parameterName, const CIdentifier& boxID, const std::string& boxParameterName) {
    // Check connection is not already in parameterConenctions
    for (auto c : this->m_parameterConnections) {
        if (c->getScenarioParameterName() == parameterName && c->getBoxID() == boxID && c->getBoxParameterName() == boxParameterName) {
            return;
        }
    }

    // Find parameter from name
    Parameters::ParameterTraits* scenarioParam = nullptr;
    this->getSettingValue(parameterName.c_str(), scenarioParam);

    if(!scenarioParam) {
        this->getLogManager() << Kernel::LogLevel_Error << "Failed to find parameter " << parameterName << "\n";
        return;
    }

    // Retrive box from ID
    IBox* box = this->getBoxDetails(boxID);
    if(!box) {
        this->getLogManager() << Kernel::LogLevel_Error << "Failed to find box with ID " << boxID.toString() << "\n";
        return;
    }
    Parameters::ParameterTraits* boxParam = nullptr;
    box->getSettingValue(boxParameterName.c_str(), boxParam);


    this->getLogManager() << Kernel::LogLevel_Debug << "Connecting parameter " << parameterName << " to box parameter " << boxParameterName << "\n";
    scenarioParam->connectTo(boxParam);
    m_parameterConnections.push_back(new CScenarioParameterConnection(parameterName, boxID, boxParameterName));

    // Force first sync
    scenarioParam->sync();
}

void CScenario::removeParameterConnection(const std::string& parameterName, const CIdentifier& boxID, const std::string& boxParameterName) {
    // Find parameter from name
    Parameters::ParameterTraits* scenarioParam = nullptr;
    this->getSettingValue(parameterName.c_str(), scenarioParam);

    if(!scenarioParam) {
        this->getLogManager() << Kernel::LogLevel_Error << "Failed to find parameter " << parameterName << "\n";
        return;
    }

    // Retrive box from ID
    IBox* box = this->getBoxDetails(boxID);
    if(!box) {
        this->getLogManager() << Kernel::LogLevel_Error << "Failed to find box with ID " << boxID.toString() << "\n";
        return;
    }

    this->getLogManager() << Kernel::LogLevel_Debug << "Disconnecting parameter " << parameterName << " from box parameter " << boxParameterName << "\n";
    Parameters::ParameterTraits* boxParam;
    box->getSettingValue(boxParameterName.c_str(), boxParam);
    scenarioParam->disconnectFrom(boxParam);

    clearParameterConnection(parameterName, boxID, boxParameterName);
}

void CScenario::clearParameterConnection(const std::string& parameterName, const CIdentifier& boxID, const std::string& boxParameterName) {
    auto it = m_parameterConnections.begin();
    while (it != m_parameterConnections.end()) {
        auto connection = *it;
        if (connection->getScenarioParameterName() == parameterName
        && connection->getBoxID() == boxID
        && connection->getBoxParameterName() == boxParameterName) {
            // clear connection
            m_parameterConnections.erase(it);
            delete connection;
            connection = nullptr;
            break;
        }
        ++it;
    }
}

void CScenario::clearParameterConnections(const std::string& parameterName) {
    std::vector<std::tuple<std::string, CIdentifier, std::string>> connectionsToRemove;
    for (auto itParamConnection = m_parameterConnections.begin(); itParamConnection != m_parameterConnections.end(); itParamConnection++){ 
        if ((*itParamConnection)->getScenarioParameterName() == parameterName){
            connectionsToRemove.push_back(std::make_tuple((*itParamConnection)->getScenarioParameterName(), (*itParamConnection)->getBoxID(), (*itParamConnection)->getBoxParameterName()));
        }
    }
    for (auto& connection : connectionsToRemove) {
        // Just need to clear the list. Actual connections will be cleared upon box destruction
        clearParameterConnection(std::get<0>(connection), std::get<1>(connection), std::get<2>(connection));
    }
}

void CScenario::clearBoxParameterConnections(const CIdentifier& boxID) {
    std::vector<std::tuple<std::string, CIdentifier, std::string>> connectionsToRemove;
    for (auto itParamConnection = m_parameterConnections.begin(); itParamConnection != m_parameterConnections.end(); itParamConnection++){ 
        if ((*itParamConnection)->getBoxID() == boxID){
            connectionsToRemove.push_back(std::make_tuple((*itParamConnection)->getScenarioParameterName(), (*itParamConnection)->getBoxID(), (*itParamConnection)->getBoxParameterName()));
        }
    }
    for (auto& connection : connectionsToRemove) {
        // Just need to clear the list. Actual connections will be cleared upon box destruction
        clearParameterConnection(std::get<0>(connection), std::get<1>(connection), std::get<2>(connection));
    }
}

void CScenario::updateParameterConnections(const std::string& oldScenarioParameterName, const std::string& newScenarioParameterName) {
    for (auto& connection : m_parameterConnections) {
        if (connection->getScenarioParameterName() == oldScenarioParameterName) {
            connection->setScenarioParameterName(newScenarioParameterName);
        }
    }
}

void CScenario::getParameterConnections(const std::string& parameterName, std::vector<CScenarioParameterConnection*>& connections) const {
    for (auto& connection : m_parameterConnections) {
        if (connection->getScenarioParameterName() == parameterName) {
            connections.push_back(connection);
        }
    }
}

void CScenario::getParameterConnectionCandidates(const std::string& parameterName, std::vector<CScenarioParameterConnection>& connections) const {
    // Get scenario parameter Type
    Parameters::ParameterTraits* param = nullptr;
    this->getSettingValue(parameterName.c_str(), param);

    if(!param) {
        this->getLogManager() << Kernel::LogLevel_Error << "Failed to find parameter " << parameterName << "\n";
        return;
    }
    std::string scenarioParamType = param->type();

    // Loop through boxes to find non connected parameters of matching type
    for (auto& boxInfo : m_boxes) {
        IBox* box = boxInfo.second;
        for (size_t i = 0; i < box->getSettingCount(); i++) {
            std::string boxParameterType;
            box->getSettingValue(i, param);
            if (!param) {
                this->getLogManager() << Kernel::LogLevel_Error << "Failed to retrieve parameter at index " << i << " for box " << box->getName().toASCIIString() <<"\n";
                continue;
            }
            boxParameterType = param->type();
            if (boxParameterType == scenarioParamType) {
                bool alreadyConnected = false;
                for (auto& connection : m_parameterConnections) {
                    if (connection->getScenarioParameterName() == parameterName
                     && connection->getBoxID() == box->getIdentifier() 
                     && connection->getBoxParameterName() == param->name()) {
                        alreadyConnected = true;
                        break;
                    }
                }
                if (!alreadyConnected) {
                    connections.push_back(CScenarioParameterConnection(parameterName, box->getIdentifier(), param->name()));
                }
            }
        }
    }
}

size_t CScenario::getParameterConnectionsCount(const std::string& parameterName) const {
    size_t count = 0;
    for (auto& connection : m_parameterConnections) {
        if (connection->getScenarioParameterName() == parameterName) {
            count++;
        }
    }
    return count;
}

void CScenario::getParameterConnectionDetails(const std::string& parameterName, size_t index, CIdentifier& boxID, std::string& boxParameterName) const {
    size_t i = 0;
    for (auto& connection : m_parameterConnections) {
        if (connection->getScenarioParameterName() == parameterName) {
            if (i == index) {
                boxID = connection->getBoxID();
                boxParameterName = connection->getBoxParameterName();
                return;
            }
            i++;
        }
    }
}

//___________________________________________________________________//

CIdentifier CScenario::getNextMetadataIdentifier(const CIdentifier& previousID) const
{
    if (previousID == CIdentifier::undefined()) { return m_firstMetadataID; }
    if (m_metadatas.count(previousID) == 0) { return CIdentifier::undefined(); }
    return m_nextMetadataID.at(previousID);
}

const IMetadata* CScenario::getMetadataDetails(const CIdentifier& metadataID) const
{
    const auto it = m_metadatas.find(metadataID);
    OV_ERROR_UNLESS_KRN(it != m_metadatas.end(), "Metadata [" << metadataID.str() << "] is not part of the scenario", Kernel::ErrorType::ResourceNotFound);
    return it->second;
}

IMetadata* CScenario::getMetadataDetails(const CIdentifier& metadataID)
{
    const auto it = m_metadatas.find(metadataID);
    OV_ERROR_UNLESS_KRN(it != m_metadatas.end(), "Metadata [" << metadataID.str() << "] is not part of the scenario", Kernel::ErrorType::ResourceNotFound);
    return it->second;
}

bool CScenario::isMetadata(const CIdentifier& metadataID) const { return m_metadatas.count(metadataID) > 0; }

bool CScenario::addMetadata(CIdentifier& metadataID, const CIdentifier& suggestedMetadataID)
{
    metadataID          = getUnusedIdentifier(suggestedMetadataID);
    CMetadata* metadata = new CMetadata(this->getKernelContext(), *this);
    metadata->setIdentifier(metadataID);

    m_nextMetadataID[metadataID] = m_firstMetadataID;
    m_firstMetadataID            = metadataID;
    m_metadatas[metadataID]      = metadata;
    return true;
}

bool CScenario::removeMetadata(const CIdentifier& metadataID)
{
    // Finds the comment according to its identifier
    auto itMetadata = m_metadatas.find(metadataID);

    OV_ERROR_UNLESS_KRF(itMetadata != m_metadatas.end(), "Comment [" << metadataID.str() << "] is not part of the scenario",
                        Kernel::ErrorType::ResourceNotFound);

    // Deletes the metadata and remove it from the cache
    delete itMetadata->second;

    m_metadatas.erase(itMetadata);

    if (metadataID == m_firstMetadataID)
    {
        m_firstMetadataID = m_nextMetadataID[m_firstMetadataID];
        m_nextMetadataID.erase(metadataID);
    }
    else
    {
        const auto previousID = std::find_if(m_nextMetadataID.begin(), m_nextMetadataID.end(), [metadataID](const std::pair<CIdentifier, CIdentifier>& v)
        {
            return v.second == metadataID;
        });

        OV_FATAL_UNLESS_K(previousID != m_nextMetadataID.end(), "Removing metadata [" << metadataID << "] which is not in the cache ",
                          Kernel::ErrorType::Internal);

        m_nextMetadataID[previousID->first] = m_nextMetadataID[metadataID];
        m_nextMetadataID.erase(metadataID);
    }

    return true;
}

// Links

CIdentifier CScenario::getNextLinkIdentifier(const CIdentifier& previousID) const
{
    return getNextID<CLink*, STestTrue<CLink*>>(m_links, previousID, STestTrue<CLink*>());
}

CIdentifier CScenario::getNextLinkIdentifierFromBox(const CIdentifier& previousID, const CIdentifier& boxID) const
{
    return getNextID<CLink*, STestEqSourceBox>(m_links, previousID, STestEqSourceBox(boxID));
}

CIdentifier CScenario::getNextLinkIdentifierFromBoxOutput(const CIdentifier& previousID, const CIdentifier& boxID, const size_t index) const
{
    return getNextID<CLink*, STestEqSourceBoxOutput>(m_links, previousID, STestEqSourceBoxOutput(boxID, index));
}

CIdentifier CScenario::getNextLinkIdentifierToBox(const CIdentifier& previousID, const CIdentifier& boxID) const
{
    return getNextID<CLink*, STestEqTargetBox>(m_links, previousID, STestEqTargetBox(boxID));
}

CIdentifier CScenario::getNextLinkIdentifierToBoxInput(const CIdentifier& previousID, const CIdentifier& boxID, const size_t index) const
{
    return getNextID<CLink*, STestEqTargetBoxInput>(m_links, previousID, STestEqTargetBoxInput(boxID, index));
}

bool CScenario::isLink(const CIdentifier& boxID) const
{
    const auto itLink = m_links.find(boxID);
    return itLink != m_links.end();
}

bool CScenario::setHasIO(const bool hasIO)
{
    m_hasIO = hasIO;
    return true;
}

bool CScenario::hasIO() const { return m_hasIO; }

bool CScenario::setScenarioInputLink(const size_t scenarioInputIdx, const CIdentifier& boxID, const size_t boxInputIdx)
{
    if (boxID != CIdentifier::undefined())
    {
        OV_ERROR_UNLESS_KRF(
            scenarioInputIdx < this->getInputCount(),
            "Scenario Input index = [" << scenarioInputIdx << "] is out of range (max index = [" << (this->getInputCount() - 1) << "])",
            ErrorType::OutOfBound);

        OV_ERROR_UNLESS_KRF(
            this->isBox(boxID),
            "Box [" << boxID.str() << "] is not part of the scenario",
            ErrorType::ResourceNotFound);

        OV_ERROR_UNLESS_KRF(
            boxInputIdx < this->getBoxDetails(boxID)->getInputCount(),
            "Box Input index = [" << boxInputIdx << "] is out of range (max index = [" << (this->getBoxDetails(boxID)->getInputCount() - 1) << "])",
            ErrorType::OutOfBound);
    }

    if (scenarioInputIdx >= m_scenarioInputLinks.size()) { m_scenarioInputLinks.resize(this->getInputCount()); }

    // Remove any existing inputs connected to the target
    for (size_t i = 0; i < m_scenarioInputLinks.size(); ++i)
    {
        CIdentifier id;
        size_t index;
        this->getScenarioInputLink(i, id, index);

        if (id == boxID && index == boxInputIdx) { this->removeScenarioInputLink(i, id, index); }
    }

    // Remove any existing link to this input
    for (auto& kv : m_links)
    {
        CIdentifier linkID = kv.first;
        const CLink* link  = kv.second;
        if (link->getTargetBoxIdentifier() == boxID && link->getTargetBoxInputIndex() == boxInputIdx) { this->disconnect(linkID); }
    }

    m_scenarioInputLinks[scenarioInputIdx] = std::make_pair(boxID, boxInputIdx);
    return true;
}

bool CScenario::setScenarioInputLink(const size_t scenarioInputIdx, const CIdentifier& boxID, const CIdentifier& boxInputID)
{
    size_t boxInputIdx = size_t(-1);

    if (boxID != CIdentifier::undefined())
    {
        OV_ERROR_UNLESS_KRF(this->isBox(boxID), "Box [" << boxID.str() << "] is not part of the scenario", Kernel::ErrorType::ResourceNotFound);
        this->getBoxDetails(boxID)->getInterfacorIndex(Input, boxInputID, boxInputIdx);
    }
    return this->setScenarioInputLink(scenarioInputIdx, boxID, boxInputIdx);
}

bool CScenario::setScenarioOutputLink(const size_t scenarioOutputIdx, const CIdentifier& boxID, const size_t boxOutputIdx)
{
    if (boxID != CIdentifier::undefined())
    {
        OV_ERROR_UNLESS_KRF(scenarioOutputIdx < this->getOutputCount(),
                            "Scenario output index = [" << scenarioOutputIdx << "] is out of range (max index = [" << (this->getOutputCount() - 1) << "])",
                            ErrorType::OutOfBound);

        OV_ERROR_UNLESS_KRF(this->isBox(boxID),
                            "Box [" << boxID.str() << "] is not part of the scenario",
                            ErrorType::ResourceNotFound);

        OV_ERROR_UNLESS_KRF(boxOutputIdx < this->getBoxDetails(boxID)->getOutputCount(),
                            "Box output index = [" << boxOutputIdx << "] is out of range (max index = [" << (this->getBoxDetails(boxID)->getOutputCount() - 1)
                            << "])",
                            ErrorType::OutOfBound);
    }

    if (scenarioOutputIdx >= m_scenarioOutputLinks.size()) { m_scenarioOutputLinks.resize(this->getOutputCount()); }

    // Remove any existing outputs connected to the target
    for (size_t i = 0; i < m_scenarioOutputLinks.size(); ++i)
    {
        CIdentifier id;
        size_t index;
        this->getScenarioOutputLink(i, id, index);

        if (id == boxID && index == boxOutputIdx) { this->removeScenarioOutputLink(i, id, index); }
    }

    m_scenarioOutputLinks[scenarioOutputIdx] = std::make_pair(boxID, boxOutputIdx);

    return true;
}

bool CScenario::setScenarioOutputLink(const size_t scenarioOutputIdx, const CIdentifier& boxID, const CIdentifier& boxOutputID)
{
    size_t boxOutputIdx = size_t(-1);

    if (boxID != CIdentifier::undefined())
    {
        OV_ERROR_UNLESS_KRF(this->isBox(boxID), "Box [" << boxID.str() << "] is not part of the scenario", Kernel::ErrorType::ResourceNotFound);
        this->getBoxDetails(boxID)->getInterfacorIndex(Output, boxOutputID, boxOutputIdx);
    }
    return this->setScenarioOutputLink(scenarioOutputIdx, boxID, boxOutputIdx);
}

bool CScenario::getScenarioInputLink(const size_t scenarioInputIdx, CIdentifier& boxID, size_t& boxInputIdx) const
{
    OV_ERROR_UNLESS_KRF(scenarioInputIdx < this->getInputCount(),
                        "Scenario input index = [" << scenarioInputIdx << "] is out of range (max index = [" << (this->getInputCount() - 1) << "])",
                        ErrorType::OutOfBound);

    if (scenarioInputIdx >= m_scenarioInputLinks.size()) { m_scenarioInputLinks.resize(this->getInputCount()); }

    boxID       = m_scenarioInputLinks[scenarioInputIdx].first;
    boxInputIdx = m_scenarioInputLinks[scenarioInputIdx].second;

    return true;
}

bool CScenario::getScenarioInputLink(const size_t scenarioInputIdx, CIdentifier& boxID, CIdentifier& boxOutputID) const
{
    size_t index;
    boxOutputID = CIdentifier::undefined();

    this->getScenarioInputLink(scenarioInputIdx, boxID, index);

    if (boxID != CIdentifier::undefined())
    {
        if (m_boxes.find(boxID) != m_boxes.end()) { this->getBoxDetails(boxID)->getInterfacorIdentifier(Input, index, boxOutputID); }
    }

    return true;
}

bool CScenario::getScenarioOutputLink(const size_t scenarioOutputIdx, CIdentifier& boxID, size_t& boxOutputIdx) const
{
    OV_ERROR_UNLESS_KRF(scenarioOutputIdx < this->getOutputCount(),
                        "Scenario output index = [" << scenarioOutputIdx << "] is out of range (max index = [" << (this->getOutputCount() - 1) << "])",
                        ErrorType::OutOfBound);

    if (scenarioOutputIdx >= m_scenarioOutputLinks.size()) { m_scenarioOutputLinks.resize(this->getOutputCount()); }

    boxID        = m_scenarioOutputLinks[scenarioOutputIdx].first;
    boxOutputIdx = m_scenarioOutputLinks[scenarioOutputIdx].second;

    return true;
}

bool CScenario::getScenarioOutputLink(const size_t scenarioOutputIdx, CIdentifier& boxID, CIdentifier& boxOutputID) const
{
    size_t index;
    boxOutputID = CIdentifier::undefined();

    this->getScenarioOutputLink(scenarioOutputIdx, boxID, index);

    if (boxID != CIdentifier::undefined())
    {
        if (m_boxes.find(boxID) != m_boxes.end()) { this->getBoxDetails(boxID)->getInterfacorIdentifier(Output, index, boxOutputID); }
    }

    return true;
}

// Note: In current implementation only the scenarioInputIdx is necessary as it can only be connected to one input
// but to keep things simpler we give it all the info
bool CScenario::removeScenarioInputLink(const size_t scenarioInputIdx, const CIdentifier& boxID, const size_t boxInputIdx)
{
    if (boxID != CIdentifier::undefined())
    {
        OV_ERROR_UNLESS_KRF(scenarioInputIdx < this->getInputCount(),
                            "Scenario Input index = [" << scenarioInputIdx << "] is out of range (max index = [" << (this->getInputCount() - 1) << "])",
                            ErrorType::OutOfBound);

        OV_ERROR_UNLESS_KRF(this->isBox(boxID), "Box [" << boxID.str() << "] is not part of the scenario", Kernel::ErrorType::ResourceNotFound);

        OV_ERROR_UNLESS_KRF(boxInputIdx < this->getBoxDetails(boxID)->getInputCount(),
                            "Box Input index = [" << boxInputIdx << "] is out of range (max index = [" << (this->getBoxDetails(boxID)->getInputCount() - 1) <<
                            "])",
                            ErrorType::OutOfBound);
    }

    // This should not happen either
    if (scenarioInputIdx >= m_scenarioInputLinks.size()) { m_scenarioInputLinks.resize(this->getInputCount()); }

    m_scenarioInputLinks[scenarioInputIdx] = std::make_pair(CIdentifier::undefined(), 0);
    return true;
}

// Note: In current implementation only the scenarioOutputIdx is necessary as it can only be connected to one Output
// but to keep things simpler we give it all the info
bool CScenario::removeScenarioOutputLink(const size_t scenarioOutputIdx, const CIdentifier& boxID, const size_t boxOutputIdx)
{
    if (boxID != CIdentifier::undefined())
    {
        OV_ERROR_UNLESS_KRF(scenarioOutputIdx < this->getOutputCount(),
                            "Scenario output index = [" << scenarioOutputIdx << "] is out of range (max index = [" << (this->getOutputCount() - 1) << "])",
                            ErrorType::OutOfBound);

        OV_ERROR_UNLESS_KRF(this->isBox(boxID), "Box [" << boxID.str() << "] is not part of the scenario", Kernel::ErrorType::ResourceNotFound);

        OV_ERROR_UNLESS_KRF(boxOutputIdx < this->getBoxDetails(boxID)->getOutputCount(),
                            "Box output index = [" << boxOutputIdx << "] is out of range (max index = [" << (this->getBoxDetails(boxID)->getOutputCount() - 1)
                            << "])",
                            ErrorType::OutOfBound);
    }

    // This should not happen either
    if (scenarioOutputIdx >= m_scenarioOutputLinks.size()) { m_scenarioOutputLinks.resize(this->getOutputCount()); }

    m_scenarioOutputLinks[scenarioOutputIdx] = std::make_pair(CIdentifier::undefined(), 0);
    return true;
}

const ILink* CScenario::getLinkDetails(const CIdentifier& linkID) const
{
    const auto it = m_links.find(linkID);
    OV_ERROR_UNLESS_KRN(it != m_links.end(), "link [" << linkID.str() << "] is not part of the scenario", Kernel::ErrorType::ResourceNotFound);
    return it->second;
}

ILink* CScenario::getLinkDetails(const CIdentifier& linkID)
{
    const auto it = m_links.find(linkID);
    OV_ERROR_UNLESS_KRN(it != m_links.end(), "Link [" << linkID.str() << "] is not part of the scenario", Kernel::ErrorType::ResourceNotFound);
    return it->second;
}

bool CScenario::connect(CIdentifier& linkID, const CIdentifier& srcBoxID, const size_t srcBoxOutputIdx,
                        const CIdentifier& dstBoxID, const size_t dstBoxInputIdx, const CIdentifier& suggestedLinkID)
{
    const auto itBox1 = m_boxes.find(srcBoxID);
    const auto itBox2 = m_boxes.find(dstBoxID);

    OV_ERROR_UNLESS_KRF(itBox1 != m_boxes.end(), "Source Box [" << srcBoxID.str() << "] is not part of the scenario", Kernel::ErrorType::ResourceNotFound);

    OV_ERROR_UNLESS_KRF(itBox2 != m_boxes.end(), "Target Box [" << dstBoxID.str() << "] is not part of the scenario", Kernel::ErrorType::ResourceNotFound);

    CBox* srcBox = itBox1->second;
    CBox* dstBox = itBox2->second;

    OV_ERROR_UNLESS_KRF(dstBoxInputIdx < dstBox->getInterfacorCountIncludingDeprecated(Input),
                        "Target box input index = [" << dstBoxInputIdx << "] is out of range (max index = [" << (dstBox->getInputCount() - 1) << "])",
                        ErrorType::OutOfBound);

    OV_ERROR_UNLESS_KRF(srcBoxOutputIdx < srcBox->getInterfacorCountIncludingDeprecated(Output),
                        "Source box output index = [" << srcBoxOutputIdx << "] is out of range (max index = [" << (srcBox->getOutputCount() - 1) << "])",
                        ErrorType::OutOfBound);

    // Looks for any connected link to this box input and removes it
    auto itLink = m_links.begin();
    while (itLink != m_links.end())
    {
        const auto itLinkCurrent = itLink;
        ++itLink;

        CLink* link = itLinkCurrent->second;
        if (link)
        {
            if (link->getTargetBoxIdentifier() == dstBoxID && link->getTargetBoxInputIndex() == dstBoxInputIdx)
            {
                delete link;
                m_links.erase(itLinkCurrent);
            }
        }
    }

	// Todo: check for any connected scenario IO to this box input
	// Make scenario IO Connections retrievalbe with a getter (e.g. getScenarioIOConnectionsDetails)
	// This way, UI can update its model accordingly, instead of maintaining a parallel one

	linkID = getUnusedIdentifier(suggestedLinkID);

    CLink* link = new CLink(this->getKernelContext(), *this);
    CIdentifier srcBoxOutputID;
    CIdentifier dstBoxInputID;

    this->getSourceBoxOutputIdentifier(srcBoxID, srcBoxOutputIdx, srcBoxOutputID);
    this->getTargetBoxInputIdentifier(dstBoxID, dstBoxInputIdx, dstBoxInputID);

    link->setIdentifier(linkID);
    link->setSource(srcBoxID, srcBoxOutputIdx, srcBoxOutputID);
    link->setTarget(dstBoxID, dstBoxInputIdx, dstBoxInputID);

    m_links[link->getIdentifier()] = link;

    return true;
}

bool CScenario::connect(CIdentifier& linkID, const CIdentifier& srcBoxID, const CIdentifier& srcBoxOutputID,
                        const CIdentifier& dstBoxID, const CIdentifier& dstBoxInputID, const CIdentifier& suggestedLinkID)
{
    size_t srcBoxOutputIdx;
    size_t dstBoxInputIdx;

    this->getSourceBoxOutputIndex(srcBoxID, srcBoxOutputID, srcBoxOutputIdx);
    this->getTargetBoxInputIndex(dstBoxID, dstBoxInputID, dstBoxInputIdx);

    return this->connect(linkID, srcBoxID, srcBoxOutputIdx, dstBoxID, dstBoxInputIdx, suggestedLinkID);
}


bool CScenario::disconnect(const CIdentifier& srcBoxID, const size_t srcBoxOutputIdx, const CIdentifier& dstBoxID, const size_t dstBoxInputIdx)
{
    // Looks for any link with the same signature
    for (auto itLink = m_links.begin(); itLink != m_links.end(); ++itLink)
    {
        CLink* link = itLink->second;
        if (link)
        {
            if (link->getTargetBoxIdentifier() == dstBoxID && link->getTargetBoxInputIndex() == dstBoxInputIdx)
            {
                if (link->getSourceBoxIdentifier() == srcBoxID && link->getSourceBoxOutputIndex() == srcBoxOutputIdx)
                {
                    // Found a link, so removes it
                    delete link;
                    m_links.erase(itLink);

                    return true;
                }
            }
        }
    }

    OV_ERROR_KRF("Link is not part of the scenario", Kernel::ErrorType::ResourceNotFound);
}

bool CScenario::disconnect(const CIdentifier& srcBoxID, const CIdentifier& srcBoxOutputID, const CIdentifier& dstBoxID, const CIdentifier& dstBoxInputID)
{
    size_t srcBoxOutputIdx;
    size_t dstBoxInputIdx;

    this->getSourceBoxOutputIndex(srcBoxID, srcBoxOutputID, srcBoxOutputIdx);
    this->getTargetBoxInputIndex(dstBoxID, dstBoxInputID, dstBoxInputIdx);

    return this->disconnect(srcBoxID, srcBoxOutputIdx, dstBoxID, dstBoxInputIdx);
}

bool CScenario::disconnect(const CIdentifier& linkID)
{
    // Finds the link according to its identifier
    auto itLink = m_links.find(linkID);

    OV_ERROR_UNLESS_KRF(itLink != m_links.end(), "Link [" << linkID.str() << "] is not part of the scenario", Kernel::ErrorType::ResourceNotFound);

    // Deletes the link itself
    delete itLink->second;

    // Removes link from the link list
    m_links.erase(itLink);

    return true;
}

//___________________________________________________________________//
//                                                                   //

bool CScenario::applyLocalSettings()
{
    for (auto& box : m_boxes)
    {
        // Expand all the variables inside the newly created scenario by replacing only the $var variables
        CScenarioSettingKeywordParserCallback scenarioSettingKeywordParserCallback(*this);
        this->getConfigurationManager().registerKeywordParser("var", scenarioSettingKeywordParserCallback);

        for (size_t settingIndex = 0; settingIndex < box.second->getSettingCount(); ++settingIndex)
        {
            CString settingName  = "";
            CString settingValue = "";

            box.second->getSettingName(settingIndex, settingName);
            box.second->getSettingValue(settingIndex, settingValue);

            box.second->setSettingValue(settingIndex, this->getConfigurationManager().expandOnlyKeyword("var", settingValue, true));
            box.second->getSettingValue(settingIndex, settingValue);

            if (box.second->hasAttribute(OVD_AttributeId_SettingOverrideFilename))
            {
                settingValue = box.second->getAttributeValue(OVD_AttributeId_SettingOverrideFilename);
                box.second->setAttributeValue(
                    OVD_AttributeId_SettingOverrideFilename, this->getConfigurationManager().expandOnlyKeyword("var", settingValue, true));
            }
        }

        this->getConfigurationManager().unregisterKeywordParser("var");
    }
    return true;
}

bool CScenario::isMetabox()
{
    // A scenario with inputs and/or outputs is a metabox
    if (this->getInputCount() + this->getOutputCount() > 0) { return true; }

    // TODO_JL: Find a way to check for other conditions as well

    return false;
}

//___________________________________________________________________//
//                                                                   //

bool CScenario::acceptVisitor(IObjectVisitor& objectVisitor)
{
    CObjectVisitorContext objectVisitorContext(getKernelContext());

    if (!objectVisitor.processBegin(objectVisitorContext, *this)) { return false; }
    for (auto& box : m_boxes) { if (!box.second->acceptVisitor(objectVisitor)) { return false; } }
    for (auto& comment : m_comments) { if (!comment.second->acceptVisitor(objectVisitor)) { return false; } }
    for (auto& link : m_links) { if (!link.second->acceptVisitor(objectVisitor)) { return false; } }
    if (!objectVisitor.processEnd(objectVisitorContext, *this)) { return false; }

    return true;
}

//___________________________________________________________________//
//                                                                   //

CIdentifier CScenario::getUnusedIdentifier(const CIdentifier& suggestedID) const
{
    uint64_t newID = (uint64_t(rand()) << 32) + uint64_t(rand());
    if (suggestedID != CIdentifier::undefined()) { newID = suggestedID.id() - 1; }

    CIdentifier result;
    std::map<CIdentifier, CBox*>::const_iterator itBox;
    std::map<CIdentifier, CComment*>::const_iterator itComment;
    std::map<CIdentifier, CLink*>::const_iterator itLink;
    do
    {
        newID++;
        result    = CIdentifier(newID);
        itBox     = m_boxes.find(result);
        itComment = m_comments.find(result);
        itLink    = m_links.find(result);
    } while (itBox != m_boxes.end() || itComment != m_comments.end() || itLink != m_links.end() || result == CIdentifier::undefined());
    return result;
}

bool CScenario::checkSettings(IConfigurationManager* configurationManager)
{
    for (auto& box : m_boxes)
    {
        if (!box.second->hasAttribute(OV_AttributeId_Box_Disabled))
        {
            this->applyLocalSettings();
            // Expand all the variables inside the newly created scenario by replacing only the $var variables
            CScenarioSettingKeywordParserCallback scenarioSettingKeywordParserCallback(*this);
            this->getConfigurationManager().registerKeywordParser("var", scenarioSettingKeywordParserCallback);

            for (size_t settingIndex = 0; settingIndex < box.second->getSettingCount(); ++settingIndex)
            {
                CString settingName     = "";
                CString rawSettingValue = "";
                CIdentifier typeID;

                if (box.second->hasAttribute(OVD_AttributeId_SettingOverrideFilename)) { return true; }
                box.second->getSettingName(settingIndex, settingName);
                box.second->getSettingValue(settingIndex, rawSettingValue);
                box.second->getSettingType(settingIndex, typeID);

                CString settingValue = rawSettingValue;
                if (configurationManager) { settingValue = configurationManager->expand(settingValue); }
                else { settingValue = this->getConfigurationManager().expandOnlyKeyword("var", settingValue); }

                const auto settingTypeName = this->getTypeManager().getTypeName(typeID);

                OV_ERROR_UNLESS_KRF(
                    ::checkSettingValue(settingValue, typeID, this->getTypeManager()),
                    "<" << box.second->getName() << "> The following value: ["<< rawSettingValue <<"] expanded as ["<< settingValue <<
                    "] given as setting is not a valid [" << settingTypeName << "] value.",
                    ErrorType::BadValue);
            }

            this->getConfigurationManager().unregisterKeywordParser("var");
        }
    }
    return true;
}


//___________________________________________________________________//
//                                                                   //

CIdentifier CScenario::getNextOutdatedBoxIdentifier(const CIdentifier& previousID) const
{
    return getNextID<std::shared_ptr<CBox>, STestTrue<std::shared_ptr<CBox>>>(m_outdatedBoxes, previousID, STestTrue<std::shared_ptr<CBox>>());
}

bool CScenario::hasOutdatedBox()
{
    for (auto& box : m_boxes) { if (box.second->hasAttribute(OV_AttributeId_Box_ToBeUpdated)) { return true; } }
    return false;
}

bool CScenario::isBoxOutdated(const CIdentifier& boxId)
{
    IBox* box = getBoxDetails(boxId);
    if (!box) { return false; }
    CIdentifier boxHashCode1;
    CIdentifier boxHashCode2;
    if (box->getAlgorithmClassIdentifier() == OVP_ClassId_BoxAlgorithm_Metabox)
    {
        CIdentifier metaboxId;
        metaboxId.fromString(box->getAttributeValue(OVP_AttributeId_Metabox_ID));
        boxHashCode1 = getKernelContext().getMetaboxManager().getMetaboxHash(metaboxId);
    }
    else { boxHashCode1 = this->getKernelContext().getPluginManager().getPluginObjectHashValue(box->getAlgorithmClassIdentifier()); }

    boxHashCode2.fromString(box->getAttributeValue(OV_AttributeId_Box_InitialPrototypeHashValue));

    if (!(boxHashCode1 == CIdentifier::undefined() || boxHashCode1 == boxHashCode2)) { return true; }

    return false;
}

bool CScenario::checkOutdatedBoxes()
{
    bool result = false;

    m_outdatedBoxes.clear();
    m_updatedBoxIOCorrespondence[Input]  = std::map<CIdentifier, std::map<size_t, size_t>>();
    m_updatedBoxIOCorrespondence[Output] = std::map<CIdentifier, std::map<size_t, size_t>>();

    for (auto& box : m_boxes)
    {
        // Do not attempt to update boxes which do not have existing box algorithm identifiers
        auto boxAlgorithmClassID = box.second->getAlgorithmClassIdentifier();
        if (boxAlgorithmClassID != OVP_ClassId_BoxAlgorithm_Metabox
            && !dynamic_cast<const Plugins::IBoxAlgorithmDesc*>(this->getKernelContext().getPluginManager().getPluginObjectDescCreating(boxAlgorithmClassID)))
        {
            continue;
        }

        // Do not attempt to update metaboxes which do not have an associated scenario
        if (boxAlgorithmClassID == OVP_ClassId_BoxAlgorithm_Metabox)
        {
            CString metaboxIdentifier = box.second->getAttributeValue(OVP_AttributeId_Metabox_ID);
            if (metaboxIdentifier == CString("")) { continue; }

            CIdentifier metaboxId;
            metaboxId.fromString(metaboxIdentifier);
            CString metaboxScenarioPath(this->getKernelContext().getMetaboxManager().getMetaboxFilePath(metaboxId));

            if (metaboxScenarioPath == CString("")) { continue; }
        }

        // Box Updater instance which is in charge of create updated boxes and links
        CBoxUpdater boxUpdater(*this, box.second);

        if (!boxUpdater.initialize())
        {
            OV_WARNING_K("Could not check for update the box with id " << box.second->getIdentifier());
            continue;
        }

        // exception for boxes that could not be automatically updated
        if (boxUpdater.flaggedForManualUpdate())
        {
            if (this->isBoxOutdated(box.second->getIdentifier()))
            {
                auto toBeUpdatedBox = std::make_shared<CBox>(getKernelContext());
                toBeUpdatedBox->initializeFromAlgorithmClassIdentifierNoInit(boxAlgorithmClassID);
                m_outdatedBoxes[box.second->getIdentifier()] = toBeUpdatedBox;
                m_boxes[box.first]->setAttributeValue(OV_AttributeId_Box_ToBeUpdated, "");
                result = true;
            }
            continue;
        }

        // collect updated boxes
        if (boxUpdater.isUpdateRequired())
        {
            m_updatedBoxIOCorrespondence[Input][box.second->getIdentifier()]  = boxUpdater.getOriginalToUpdatedInterfacorCorrespondence(Input);
            m_updatedBoxIOCorrespondence[Output][box.second->getIdentifier()] = boxUpdater.getOriginalToUpdatedInterfacorCorrespondence(Output);
            // it is important to set box algorithm at
            // last so the box listener is never called
            boxUpdater.getUpdatedBox().setAlgorithmClassIdentifier(boxAlgorithmClassID);
            // copy requested box into a new instance managed in scenario
            auto newBox = std::make_shared<CBox>(this->getKernelContext());
            newBox->initializeFromExistingBox(boxUpdater.getUpdatedBox());
            m_outdatedBoxes[box.second->getIdentifier()] = newBox;
            m_boxes[box.first]->setAttributeValue(OV_AttributeId_Box_ToBeUpdated, "");
            result = true;
        }
    }

    return result;
}


template <class T, class TTest>
void getIdentifierList(const std::map<CIdentifier, T>& elementMap, const TTest& testFunctor, CIdentifier** listID, size_t* size)
{
    *listID = new CIdentifier[elementMap.size()];

    size_t index = 0;
    for (auto it = elementMap.begin(); it != elementMap.end(); ++it) { if (testFunctor(it)) { (*listID)[index++] = it->first; } }
    *size = index;
}

void CScenario::getBoxIdentifierList(CIdentifier** listID, size_t* size) const
{
    getIdentifierList<CBox*, STestTrue<CBox*>>(m_boxes, STestTrue<CBox*>(), listID, size);
}

void CScenario::getCommentIdentifierList(CIdentifier** listID, size_t* size) const
{
    getIdentifierList<CComment*, STestTrue<CComment*>>(m_comments, STestTrue<CComment*>(), listID, size);
}

void CScenario::getMetadataIdentifierList(CIdentifier** listID, size_t* size) const
{
    getIdentifierList<CMetadata*, STestTrue<CMetadata*>>(m_metadatas, STestTrue<CMetadata*>(), listID, size);
}

void CScenario::getLinkIdentifierList(CIdentifier** listID, size_t* size) const
{
    getIdentifierList<CLink*, STestTrue<CLink*>>(m_links, STestTrue<CLink*>(), listID, size);
}

void CScenario::getLinkIdentifierFromBoxList(const CIdentifier& boxID, CIdentifier** listID, size_t* size) const
{
    getIdentifierList<CLink*, STestEqSourceBox>(m_links, STestEqSourceBox(boxID), listID, size);
}

void CScenario::getLinkIdentifierFromBoxOutputList(const CIdentifier& boxID, const size_t index, CIdentifier** listID, size_t* size) const
{
    getIdentifierList<CLink*, STestEqSourceBoxOutput>(m_links, STestEqSourceBoxOutput(boxID, index), listID, size);
}

void CScenario::getLinkIdentifierToBoxList(const CIdentifier& boxID, CIdentifier** listID, size_t* size) const
{
    getIdentifierList<CLink*, STestEqTargetBox>(m_links, STestEqTargetBox(boxID), listID, size);
}

void CScenario::getLinkIdentifierToBoxInputList(const CIdentifier& boxID, const size_t index, CIdentifier** listID, size_t* size) const
{
    getIdentifierList<CLink*, STestEqTargetBoxInput>(m_links, STestEqTargetBoxInput(boxID, index), listID, size);
}

void CScenario::getOutdatedBoxIdentifierList(CIdentifier** listID, size_t* size) const
{
    getIdentifierList<std::shared_ptr<CBox>, STestTrue<std::shared_ptr<CBox>>>(m_outdatedBoxes, STestTrue<std::shared_ptr<CBox>>(), listID, size);
}

void CScenario::releaseIdentifierList(CIdentifier* listID) const { delete[] listID; }

bool CScenario::getSourceBoxOutputIndex(const CIdentifier& srcBoxID, const CIdentifier& srcBoxOutputID, size_t& srcBoxOutputIdx)
{
    const auto itSourceBox = m_boxes.find(srcBoxID);
    OV_ERROR_UNLESS_KRF(itSourceBox != m_boxes.end(), "Source Box [" << srcBoxID.str() << "] is not part of the scenario", Kernel::ErrorType::ResourceNotFound);
    m_boxes[srcBoxID]->getInterfacorIndex(Output, srcBoxOutputID, srcBoxOutputIdx);
    return true;
}

bool CScenario::getTargetBoxInputIndex(const CIdentifier& dstBoxID, const CIdentifier& dstBoxInputID, size_t& dstBoxInputIdx)
{
    const auto itTargetBox = m_boxes.find(dstBoxID);
    OV_ERROR_UNLESS_KRF(itTargetBox != m_boxes.end(), "Target Box [" << dstBoxID.str() << "] is not part of the scenario", Kernel::ErrorType::ResourceNotFound);
    m_boxes[dstBoxID]->getInterfacorIndex(Input, dstBoxInputID, dstBoxInputIdx);
    return true;
}

bool CScenario::getSourceBoxOutputIdentifier(const CIdentifier& srcBoxID, const size_t& srcBoxOutputIdx, CIdentifier& srcBoxOutputID)
{
    const auto itSourceBox = m_boxes.find(srcBoxID);
    OV_ERROR_UNLESS_KRF(itSourceBox != m_boxes.end(), "Source Box [" << srcBoxID.str() << "] is not part of the scenario", Kernel::ErrorType::ResourceNotFound);
    m_boxes[srcBoxID]->getInterfacorIdentifier(Output, srcBoxOutputIdx, srcBoxOutputID);
    return true;
}

bool CScenario::getTargetBoxInputIdentifier(const CIdentifier& dstBoxID, const size_t& dstBoxInputIdx, CIdentifier& dstBoxInputID)
{
    const auto itTargetBox = m_boxes.find(dstBoxID);
    OV_ERROR_UNLESS_KRF(itTargetBox != m_boxes.end(), "Target Box [" << dstBoxID.str() << "] is not part of the scenario", Kernel::ErrorType::ResourceNotFound);
    m_boxes[dstBoxID]->getInterfacorIdentifier(Input, dstBoxInputIdx, dstBoxInputID);
    return true;
}
/**
 * \brief Process to the update of the identified box.
 *      It consists in recreate the prototype of the box according to the updated reference box which is the box
 *      resulting of the add/pull requests to the kernel prototype.
 * \param boxID     the identifier of the box to be updated
 * \return   true when update has been done successfully
 * \return   false in case of failure
 */
bool CScenario::updateBox(const CIdentifier& boxID)
{
    // Check if box must be updated
    const auto itSourceBox = m_boxes.find(boxID);
    OV_ERROR_UNLESS_KRF(itSourceBox != m_boxes.end(), "Box [" << boxID.str() << "] is not part of the scenario", Kernel::ErrorType::ResourceNotFound);

    auto itUpdateBox = m_outdatedBoxes.find(boxID);

    if (itUpdateBox == m_outdatedBoxes.end())
    {
        this->checkOutdatedBoxes();
        itUpdateBox = m_outdatedBoxes.find(boxID);
    }

    OV_ERROR_UNLESS_KRF(itUpdateBox != m_outdatedBoxes.end(), "Box [" << boxID.str() << "] misses an updated version", Kernel::ErrorType::ResourceNotFound);


    if (itUpdateBox->second->hasAttribute(OV_AttributeId_Box_FlagNeedsManualUpdate)
        || itUpdateBox->second->hasAttribute(OV_AttributeId_Box_FlagCanAddInput)
        || itUpdateBox->second->hasAttribute(OV_AttributeId_Box_FlagCanAddOutput)
        || itUpdateBox->second->hasAttribute(OV_AttributeId_Box_FlagCanAddSetting)
        || itUpdateBox->second->hasAttribute(OV_AttributeId_Box_FlagCanModifyInput)
        || itUpdateBox->second->hasAttribute(OV_AttributeId_Box_FlagCanModifyOutput)
        || itUpdateBox->second->hasAttribute(OV_AttributeId_Box_FlagCanModifySetting))
    {
        OV_ERROR_KRF(m_boxes[boxID]->getName() << " must be manually updated. Its prototype is too complex.", Kernel::ErrorType::NotImplemented);
    }
    OV_ERROR_UNLESS_KRF(itUpdateBox != m_outdatedBoxes.end(), "Box [" << boxID.str() << "] does not have to be updated", Kernel::ErrorType::ResourceNotFound);

    // get all non-updatable attributes from the source box
    std::map<CIdentifier, CString> nonUpdatableAttributes;
    {
        CIdentifier attributeId = CIdentifier::undefined();
        while ((attributeId = itSourceBox->second->getNextAttributeIdentifier(attributeId)) != CIdentifier::undefined())
        {
            const auto& updatableAttrs = CBoxUpdater::UPDATABLE_ATTRIBUTES;
            if (std::find(updatableAttrs.cbegin(), updatableAttrs.cend(), attributeId) == updatableAttrs.cend())
            {
                nonUpdatableAttributes[attributeId] = itSourceBox->second->getAttributeValue(attributeId);
            }
        }
    }

    // gather links coming to and from the box
    std::map<EBoxInterfacorType, std::vector<std::shared_ptr<CLink>>> links;
    for (auto interfacorType : { Input, Output })
    {
        links[interfacorType] = std::vector<std::shared_ptr<CLink>>();
        for (size_t index = 0; index < itUpdateBox->second->getInterfacorCountIncludingDeprecated(interfacorType); ++index)
        {
            CIdentifier* linkIdentifierList = nullptr;
            size_t linkCount                = 0;
            if (interfacorType == Input) { this->getLinkIdentifierToBoxInputList(boxID, index, &linkIdentifierList, &linkCount); }
            else if (interfacorType == Output) { this->getLinkIdentifierFromBoxOutputList(boxID, index, &linkIdentifierList, &linkCount); }

            for (size_t i = 0; i < linkCount; ++i)
            {
                auto link = std::make_shared<CLink>(this->getKernelContext(), *this);
                link->initializeFromExistingLink(*this->getLinkDetails(linkIdentifierList[i]));
                if (this->getLinkDetails(linkIdentifierList[i])->hasAttribute(OV_AttributeId_Link_Invalid))
                {
                    link->addAttribute(OV_AttributeId_Link_Invalid, "");
                }
                links[interfacorType].emplace_back(link);
            }

            this->releaseIdentifierList(linkIdentifierList);
        }
    }

    OV_FATAL_UNLESS_K(this->removeBox(boxID), "Failed to remove redundant box", Kernel::ErrorType::Internal);

    CIdentifier updatedBoxIdentifier;
    OV_FATAL_UNLESS_K(this->addBox(updatedBoxIdentifier, *(itUpdateBox->second.get()), boxID), "Failed to add box to the scenario",
                      Kernel::ErrorType::Internal);
    OV_FATAL_UNLESS_K(updatedBoxIdentifier == boxID, "Updated box failed to initialize with same identifier", Kernel::ErrorType::Internal);

    auto updatedBox = this->getBoxDetails(boxID);

    for (const auto& attr : nonUpdatableAttributes)
    {
        if (attr.first == OV_AttributeId_Box_ToBeUpdated) { continue; }
        if (updatedBox->hasAttribute(attr.first)) { updatedBox->setAttributeValue(attr.first, attr.second); }
        else { updatedBox->addAttribute(attr.first, attr.second); }
    }

    // Reconnect links
    std::map<EBoxInterfacorType, std::set<size_t>> isInterfacorConnected;
    isInterfacorConnected[Input]  = std::set<size_t>();
    isInterfacorConnected[Output] = std::set<size_t>();
    for (auto& link : links[Input])
    {
        CIdentifier newLinkIdentifier;
        auto index = m_updatedBoxIOCorrespondence.at(Input).at(boxID).at(link->getTargetBoxInputIndex());
        this->connect(newLinkIdentifier, link->getSourceBoxIdentifier(), link->getSourceBoxOutputIndex(), boxID, index, link->getIdentifier());
        isInterfacorConnected[Input].insert(index);
        if (link->hasAttribute(OV_AttributeId_Link_Invalid)) { this->getLinkDetails(newLinkIdentifier)->setAttributeValue(OV_AttributeId_Link_Invalid, ""); }
    }
    for (const auto& link : links[Output])
    {
        CIdentifier newLinkIdentifier;
        auto index = m_updatedBoxIOCorrespondence.at(Output).at(boxID).at(link->getSourceBoxOutputIndex());
        this->connect(newLinkIdentifier, boxID, index, link->getTargetBoxIdentifier(), link->getTargetBoxInputIndex(), link->getIdentifier());
        isInterfacorConnected[Output].insert(index);
        if (link->hasAttribute(OV_AttributeId_Link_Invalid)) { this->getLinkDetails(newLinkIdentifier)->setAttributeValue(OV_AttributeId_Link_Invalid, ""); }
    }

    // Cleanup the i/o that are redundant and disconnected
    for (auto t : { Input, Output })
    {
        size_t i = updatedBox->getInterfacorCountIncludingDeprecated(t);
        while (i > 0)
        {
            --i;
            bool isDeprecated;
            updatedBox->getInterfacorDeprecatedStatus(t, i, isDeprecated);
            if (isDeprecated && isInterfacorConnected.at(t).find(i) == isInterfacorConnected.at(t).end()) { updatedBox->removeInterfacor(t, i, true); }
        }
    }

    // Cleanup the settings that are redundant and have the same value as default
    size_t settingIndex = updatedBox->getInterfacorCountIncludingDeprecated(Setting);
    while (settingIndex > 0)
    {
        settingIndex--;
        bool isDeprecated;
        CString value;
        CString defaultValue;
        updatedBox->getInterfacorDeprecatedStatus(Setting, settingIndex, isDeprecated);
        updatedBox->getSettingValue(settingIndex, value);
        updatedBox->getSettingDefaultValue(settingIndex, defaultValue);
        if (isDeprecated && value == defaultValue) { updatedBox->removeInterfacor(Setting, settingIndex, true); }
    }


    bool hasDeprecatedInterfacor = false;
    for (auto t : { Setting, Input, Output })
    {
        hasDeprecatedInterfacor |= (updatedBox->getInterfacorCount(t) != updatedBox->getInterfacorCountIncludingDeprecated(t));
    }

    if (hasDeprecatedInterfacor)
    {
        OV_WARNING_K(m_boxes[boxID]->getName()
            << " box has not been fully updated. Deprecated Inputs, Outputs or Settings are pending.\n"
            << " Please remove them before exporting scenario\n");
        this->getBoxDetails(boxID)->setAttributeValue(OV_AttributeId_Box_PendingDeprecatedInterfacors, "");
    }
    else { this->getLogManager() << LogLevel_Info << m_boxes[boxID]->getName() << " box has been updated successfully\n"; }

    return true;
}

bool CScenario::removeDeprecatedInterfacorsFromBox(const CIdentifier& boxID)
{
    // Check if box must be updated
    IBox* box = getBoxDetails(boxID);
    if (!box) { return false; }

    for (auto interfacorType : { Input, Output, Setting })
    {
        const auto nInterfacor = box->getInterfacorCountIncludingDeprecated(interfacorType);
        if (nInterfacor == 0) { continue; }
        auto index = nInterfacor;
        do
        {
            index--;
            bool isDeprecated = false;
            box->getInterfacorDeprecatedStatus(interfacorType, index, isDeprecated);
            if (isDeprecated) { box->removeInterfacor(interfacorType, index); }
        } while (index != 0);
    }

    this->getLogManager() << LogLevel_Info << m_boxes[boxID]->getName()
            << " Deprecated I/O and settings have been removed successfully\n";

    box->removeAttribute(OV_AttributeId_Box_PendingDeprecatedInterfacors);

    return true;
}

bool CScenario::containsBoxWithDeprecatedInterfacors() const
{
    for (const auto& box : m_boxes)
    {
        for (const auto interfacorType : { Input, Output, Setting })
        {
            if (box.second->getInterfacorCountIncludingDeprecated(interfacorType) > box.second->getInterfacorCount(interfacorType)) { return true; }
        }
    }
    return false;
}

}  // namespace Kernel
}  // namespace OpenViBE
