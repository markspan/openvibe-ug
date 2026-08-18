#pragma once

#include "../ovkTKernelObject.h"
#include "openvibe/parameters/parameters.h"

#include "ovkTBox.hpp"

#include <vector>
#include <map>
#include <memory>

namespace OpenViBE {
namespace Kernel {
typedef TBox<IBox> CBox;
class CComment;
class CMetadata;
class CLink;

class CScenario final : public TBox<IScenario>
{
public:

    CScenario(const IKernelContext& ctx, const CIdentifier& identifier);
    ~CScenario() override {
        this->clear();
    }

    bool clear() override;
    bool merge(const IScenario& scenario, IScenarioMergeCallback* scenarioMergeCallback, bool mergeSettings, bool preserveIDs) override;

    CIdentifier getNextBoxIdentifier(const CIdentifier& previousID) const override;
    bool isBox(const CIdentifier& boxID) const override;
    const IBox* getBoxDetails(const CIdentifier& boxID) const override;
    IBox* getBoxDetails(const CIdentifier& boxID) override;
    bool addBox(CIdentifier& boxID, const CIdentifier& suggestedBoxID) override;
    bool addBox(CIdentifier& boxID, const IBox& box, const CIdentifier& suggestedBoxID) override;
    bool addBox(CIdentifier& boxID, const CIdentifier& boxAlgorithmID, const CIdentifier& suggestedBoxID) override;
    bool addBox(CIdentifier& boxID, const Plugins::IBoxAlgorithmDesc& boxAlgorithmDesc, const CIdentifier& suggestedBoxID) override;
    bool removeBox(const CIdentifier& boxID) override;

    CIdentifier getNextCommentIdentifier(const CIdentifier& previousID) const override;
    bool isComment(const CIdentifier& commentID) const override;
    const IComment* getCommentDetails(const CIdentifier& commentID) const override;
    IComment* getCommentDetails(const CIdentifier& commentID) override;
    bool addComment(CIdentifier& commentID, const CIdentifier& suggestedCommentID) override;
    bool addComment(CIdentifier& commentID, const IComment& comment, const CIdentifier& suggestedCommentID) override;
    bool removeComment(const CIdentifier& commentID) override;

    void getAvailableParameterTypes(std::vector<std::string>& types) override;
    void addParameter(const std::string& name, const std::string& type, const std::any& value) override;
    void addParameter(const std::string& name, const std::string& type) override;
    void removeParameter(const std::string& parameterName) override;
    void updateParameterName(const std::string& oldName, const std::string& newName) override;

    void addParameterConnection(const std::string& parameterName, const CIdentifier& boxID, const std::string& boxParameterName) override;
    void removeParameterConnection(const std::string& parameterName, const CIdentifier& boxID, const std::string& boxParameterName) override;
    void getParameterConnections(const std::string& parameterName, std::vector<CScenarioParameterConnection*>& connections) const override;
    void getParameterConnectionCandidates(const std::string& parameterName, std::vector<CScenarioParameterConnection>& connections) const override;
    size_t getParameterConnectionsCount(const std::string& parameterName) const override;
    void getParameterConnectionDetails(const std::string& parameterName, size_t index, CIdentifier& boxID, std::string& boxParameterName) const override;
    

    CIdentifier getNextMetadataIdentifier(const CIdentifier& previousID) const override;
    bool isMetadata(const CIdentifier& metadataID) const override;
    const IMetadata* getMetadataDetails(const CIdentifier& metadataID) const override;
    IMetadata* getMetadataDetails(const CIdentifier& metadataID) override;
    bool addMetadata(CIdentifier& metadataID, const CIdentifier& suggestedMetadataID) override;
    bool removeMetadata(const CIdentifier& metadataID) override;

    CIdentifier getNextLinkIdentifier(const CIdentifier& previousID) const override;

    CIdentifier getNextLinkIdentifierFromBox(const CIdentifier& previousID, const CIdentifier& boxID) const override;
    CIdentifier getNextLinkIdentifierFromBoxOutput(const CIdentifier& previousID, const CIdentifier& boxID, size_t index) const override;
    CIdentifier getNextLinkIdentifierToBox(const CIdentifier& previousID, const CIdentifier& boxID) const override;
    CIdentifier getNextLinkIdentifierToBoxInput(const CIdentifier& previousID, const CIdentifier& boxID, size_t index) const override;
    bool isLink(const CIdentifier& boxID) const override;

    bool setHasIO(bool hasIO) override;
    bool hasIO() const override;
    bool setScenarioInputLink(size_t scenarioInputIdx, const CIdentifier& boxID, size_t boxInputIdx) override;
    bool setScenarioInputLink(size_t scenarioInputIdx, const CIdentifier& boxID, const CIdentifier& boxInputID) override;
    bool setScenarioOutputLink(size_t scenarioOutputIdx, const CIdentifier& boxID, size_t boxOutputIdx) override;
    bool setScenarioOutputLink(size_t scenarioOutputIdx, const CIdentifier& boxID, const CIdentifier& boxOutputID) override;
    bool getScenarioInputLink(size_t scenarioInputIdx, CIdentifier& boxID, size_t& boxInputIdx) const override;
    bool getScenarioInputLink(size_t scenarioInputIdx, CIdentifier& boxID, CIdentifier& boxOutputID) const override;
    bool getScenarioOutputLink(size_t scenarioOutputIdx, CIdentifier& boxID, size_t& boxOutputIdx) const override;
    bool getScenarioOutputLink(size_t scenarioOutputIdx, CIdentifier& boxID, CIdentifier& boxOutputID) const override;
    bool removeScenarioInputLink(size_t scenarioInputIdx, const CIdentifier& boxID, size_t boxInputIdx) override;
    bool removeScenarioOutputLink(size_t scenarioOutputIdx, const CIdentifier& boxID, size_t boxOutputIdx) override;

    bool removeScenarioInput(size_t index) override;
    bool removeScenarioOutput(size_t index) override;

    const ILink* getLinkDetails(const CIdentifier& linkID) const override;
    ILink* getLinkDetails(const CIdentifier& linkID) override;

    bool connect(CIdentifier& linkID, const CIdentifier& srcBoxID, const size_t srcBoxOutputIdx, const CIdentifier& dstBoxID,
                 const size_t dstBoxInputIdx, const CIdentifier& suggestedLinkID) override;
    bool connect(CIdentifier& linkID, const CIdentifier& srcBoxID, const CIdentifier& srcBoxOutputID, const CIdentifier& dstBoxID,
                 const CIdentifier& dstBoxInputID, const CIdentifier& suggestedLinkID) override;
    bool disconnect(const CIdentifier& srcBoxID, size_t srcBoxOutputIdx, const CIdentifier& dstBoxID, size_t dstBoxInputIdx) override;
    bool disconnect(const CIdentifier& srcBoxID, const CIdentifier& srcBoxOutputID, const CIdentifier& dstBoxID, const CIdentifier& dstBoxInputID) override;
    bool disconnect(const CIdentifier& linkID) override;

    bool getSourceBoxOutputIndex(const CIdentifier& srcBoxID, const CIdentifier& srcBoxOutputID, size_t& srcBoxOutputIdx) override;
    bool getTargetBoxInputIndex(const CIdentifier& dstBoxID, const CIdentifier& dstBoxInputID, size_t& dstBoxInputIdx) override;
    bool getSourceBoxOutputIdentifier(const CIdentifier& srcBoxID, const size_t& srcBoxOutputIdx, CIdentifier& srcBoxOutputID) override;
    bool getTargetBoxInputIdentifier(const CIdentifier& dstBoxID, const size_t& dstBoxInputIdx, CIdentifier& dstBoxInputID) override;

    bool applyLocalSettings() override;
    bool checkSettings(IConfigurationManager* configurationManager) override;

    bool isBoxOutdated(const CIdentifier& boxId);
    bool checkOutdatedBoxes() override;
    bool hasOutdatedBox() override;

    CIdentifier getNextOutdatedBoxIdentifier(const CIdentifier& previousID) const override;

    bool isMetabox() override;

    void getBoxIdentifierList(CIdentifier** listID, size_t* size) const override;
    void getCommentIdentifierList(CIdentifier** listID, size_t* size) const override;
    void getMetadataIdentifierList(CIdentifier** listID, size_t* size) const override;
    void getLinkIdentifierList(CIdentifier** listID, size_t* size) const override;
    void getLinkIdentifierFromBoxList(const CIdentifier& boxID, CIdentifier** listID, size_t* size) const override;
    void getLinkIdentifierFromBoxOutputList(const CIdentifier& boxID, size_t index, CIdentifier** listID, size_t* size) const override;
    void getLinkIdentifierToBoxList(const CIdentifier& boxID, CIdentifier** listID, size_t* size) const override;
    void getLinkIdentifierToBoxInputList(const CIdentifier& boxID, size_t index, CIdentifier** listID, size_t* size) const override;
    void getOutdatedBoxIdentifierList(CIdentifier** listID, size_t* size) const override;
    void releaseIdentifierList(CIdentifier* listID) const override;


    bool acceptVisitor(IObjectVisitor& objectVisitor) override;

    bool updateBox(const CIdentifier& boxID) override;

    bool containsBoxWithDeprecatedInterfacors() const override;

    bool removeDeprecatedInterfacorsFromBox(const CIdentifier& boxID) override;

    _IsDerivedFromClass_Final_(TBox<IScenario>, OVK_ClassId_Kernel_Scenario_Scenario)

private:
    CIdentifier getUnusedIdentifier(const CIdentifier& suggestedID) const;
    void clearParameterConnection(const std::string& parameterName, const CIdentifier& boxID, const std::string& boxParameterName);
    void clearParameterConnections(const std::string& parameterName);
    void clearBoxParameterConnections(const CIdentifier& boxId);
    void updateParameterConnections(const std::string& oldScenarioParameterName, const std::string& newScenarioParameterName);

    template<typename T>
    T handleParameterType(const std::any& value) const {
        T result;
        try {
            result = std::any_cast<T>(value);
        } catch (const std::bad_any_cast& e) {
            OV_ERROR_KRF("Scenario parameter type mismatch expected " << typeid(T).name() << " got " << value.type().name(), Kernel::ErrorType::BadCast);
        }
        return result;
    };

    std::map<CIdentifier, CBox*> m_boxes;
    std::map<CIdentifier, CComment*> m_comments;
    std::map<CIdentifier, CMetadata*> m_metadatas;
    std::map<CIdentifier, CLink*> m_links;
    std::map<CIdentifier, std::shared_ptr<CBox>> m_outdatedBoxes;
    std::map<EBoxInterfacorType, std::map<CIdentifier, std::map<size_t, size_t>>> m_updatedBoxIOCorrespondence;
    std::vector<CScenarioParameterConnection*> m_parameterConnections;

    bool m_hasIO = false;

    mutable std::vector<std::pair<CIdentifier, size_t>> m_scenarioInputLinks;
    mutable std::vector<std::pair<CIdentifier, size_t>> m_scenarioOutputLinks;

    // Helper members. These are used for quick lookup of next identifiers for the purpose
    // of the getNextMetadataIdentifier function.
    std::map<CIdentifier, CIdentifier> m_nextMetadataID;
    CIdentifier m_firstMetadataID = CIdentifier::undefined();
};
}  // namespace Kernel
}  // namespace OpenViBE
