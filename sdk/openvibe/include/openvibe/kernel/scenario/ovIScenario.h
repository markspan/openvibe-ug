#pragma once

#include "ovIBox.h"

namespace OpenViBE {
class CMemoryBuffer;

namespace Plugins {
class IBoxAlgorithmDesc;
}  // namespace Plugins

namespace Parameters {
class ParameterTraits;
typedef std::vector<ParameterTraits*> Parameters;
}  // namespace Parameters


namespace Kernel {
class ILink;
class IComment;
class IMetadata;
class IConfigurationManager;
class CScenarioParameterConnection;

/**
 * \class IScenario
 * \author Yann Renard (IRISA/INRIA)
 * \date 2006-08-16
 * \brief A static OpenViBE scenario
 * \ingroup Group_Scenario
 * \ingroup Group_Kernel
 *
 * This class is a static scenario description.
 * It is used to manipulate an OpenViBE
 * box/connection collection...
 *
 * \todo Add meta information for this scenario
 */
class OV_API IScenario : public IBox
{
public:
    class IScenarioMergeCallback
    {
    public:
        virtual void process(CIdentifier& originalID, CIdentifier& newID) = 0;
        virtual ~IScenarioMergeCallback() = default;
    };

    /** \name General purpose functions */
    //@{

    /**
     * \brief Clears the scenario
     * \retval true In case of success.
     * \retval false In case of error.
     */
    virtual bool clear() = 0;
    /**
     * \brief Merges this scenario with an other existing scenario
     * \param scenario A reference to the scenario to merge this scenario with
     * \param scenarioMergeCallback A callback that will be called for each merged element and will be passed the old and new identifiers
     * \param mergeSettings When true, the settings from the source scenario will also be merged
     * \param preserveIDs When true, the element identifiers from the source scenario will be preserved.
     * \retval true In case of success
     * \retval false In case of error
     * \note The \c bPreservedIdentifiers setting is only reliable when the destination scenario is empty. As an identifier can only be
     * preserved when the destination scenario does not contain any elements which use it. In general, this parameter should only be set
     * to true when cloning a scenario.
     */
    virtual bool merge(const IScenario& scenario, IScenarioMergeCallback* scenarioMergeCallback, bool mergeSettings, bool preserveIDs) = 0;

    //@}
    /** \name Box management */
    //@{

    /**
     * \brief Gets next box identifier
     * \param previousID The identifier
     *        for the preceeding box
     * \return The identifier of the next box in case of success.
     * \retval CIdentifier::undefined() on error.
     * \note Giving \c CIdentifier::undefined() as \c previousID
     *       will cause this function to return the first box
     *       identifier.
     */
    virtual CIdentifier getNextBoxIdentifier(const CIdentifier& previousID) const = 0;

    /**
     * \brief Tests whether a given identifier is a box or not
     * \param boxID the identifier to test
     * \retval true if the identified object is a box
     * \retval false if the identified object is not a box
     * \note Requesting a bad identifier returns \e false
     */
    virtual bool isBox(const CIdentifier& boxID) const = 0;

    /**
     * \brief Gets the details for a specific box
     * \param boxID The identifier of the box which details should be sent.
     * \return The box details
     */
    virtual const IBox* getBoxDetails(const CIdentifier& boxID) const = 0;

    /// \copydoc getBoxDetails(const CIdentifier&)const
    virtual IBox* getBoxDetails(const CIdentifier& boxID) = 0;

    /**
     * \brief Adds a new box in the scenario
     * \param[out] boxID The identifier of the created box
     * \param suggestedBoxID A suggestion for the new box identifier. If this specific identifier is not
     *        yet used, this scenario might use it. If the identifier is already used or \c CIdentifier::undefined() is passed,
     *        then a random unused identifier will be used.
     * \retval true In case of success.
     * \retval false In case of error. In such case, \c boxID remains unchanged.
     * \note This produces an empty and unconfigured box!
     */
    virtual bool addBox(CIdentifier& boxID, const CIdentifier& suggestedBoxID) = 0;
    /**
     * \brief Adds a new box in the scenario based on an existing box
     * \param[out] boxID The identifier of the created box
     * \param box The box to copy in this scenario
     * \param suggestedBoxID a suggestion for the new box identifier. If this specific identifier is not
     *        yet used, this scenario might use it. If the identifier is already used or \c CIdentifier::undefined() is passed,
     *        then a random unused identifier will be used.
     * \retval true In case of success.
     * \retval false In case of error. In such case, \c boxID remains unchanged.
     */
    virtual bool addBox(CIdentifier& boxID, const IBox& box, const CIdentifier& suggestedBoxID) = 0;
    /**
     * \brief Adds a new box in the scenario
     * \param[out] boxID The identifier of the created box
     * \param boxAlgorithmClassID The class identifier of the algorithm for this box
     * \param suggestedBoxID a suggestion for the new box identifier. If this specific identifier is not
     *        yet used, this scenario might use it. If the identifier is already used or \c CIdentifier::undefined() is passed,
     *        then a random unused identifier will be used.
     * \retval true In case of success.
     * \retval false In case of error. In such case, \c boxID remains unchanged.
     * \note This function prepares the box according to its algorithm class identifier !
     */
    virtual bool addBox(CIdentifier& boxID, const CIdentifier& boxAlgorithmClassID, const CIdentifier& suggestedBoxID) = 0;

    // TODO_JL: Doc
    virtual bool addBox(CIdentifier& boxID, const Plugins::IBoxAlgorithmDesc& boxAlgorithmDesc, const CIdentifier& suggestedBoxID) = 0;

    /**
     * \brief Removes a box of the scenario
     * \param boxID The box identifier
     * \retval true In case of success.
     * \retval false In case of error.
     * \note Each link related to this box is also removed
     */
    virtual bool removeBox(const CIdentifier& boxID) = 0;

    //@}
    /** \name Connection management */
    //@{

    /**
     * \brief Gets next link identifier
     * \param previousID The identifier for the preceeding link
     * \return The identifier of the next link in case of success.
     * \retval CIdentifier::undefined() on error.
     * \note Giving \c CIdentifier::undefined() as \c previousID will cause this function to return the first link identifier.
     */
    virtual CIdentifier getNextLinkIdentifier(const CIdentifier& previousID) const = 0;

    /**
     * \brief Gets next link identifier from fixed box
     * \param previousID The identifier for the preceeding link
     * \param boxID The box identifier which the link should end to
     * \return The identifier of the next link in case of success.
     * \retval CIdentifier::undefined() on error.
     * \note Giving \c CIdentifier::undefined() as \c previousID will cause this function to return the first link identifier.
     */
    virtual CIdentifier getNextLinkIdentifierFromBox(const CIdentifier& previousID, const CIdentifier& boxID) const = 0;

    /**
     * \brief Gets next link identifier from fixed box output
     * \param previousID The identifier for the preceeding link
     * \param boxID The box identifier which the link should end to
     * \param outputIdx The input index which the link should end to
     * \return The identifier of the next link in case of success.
     * \retval CIdentifier::undefined() on error.
     * \note Giving \c CIdentifier::undefined() as \c previousID will cause this function to return the first link identifier.
     */
    virtual CIdentifier getNextLinkIdentifierFromBoxOutput(const CIdentifier& previousID, const CIdentifier& boxID, size_t outputIdx) const = 0;

    /**
     * \brief Gets next link identifier from fixed box
     * \param previousID The identifier for the preceeding link
     * \param boxID The box identifier which the link should start from
     * \return The identifier of the next link in case of success.
     * \retval CIdentifier::undefined() on error.
     * \note Giving \c CIdentifier::undefined() as \c previousID will cause this function to return the first link identifier.
     */
    virtual CIdentifier getNextLinkIdentifierToBox(const CIdentifier& previousID, const CIdentifier& boxID) const = 0;

    /**
     * \brief Gets next link identifier from fixed box input
     * \param previousID The identifier for the preceeding link
     * \param boxID The box identifier which the link should start from
     * \param index The input index which the link should start from
     * \return The identifier of the next link in case of success.
     * \retval CIdentifier::undefined() on error.
     * \note Giving \c CIdentifier::undefined() as \c previousID will cause this function to return the first link identifier.
     */
    virtual CIdentifier getNextLinkIdentifierToBoxInput(const CIdentifier& previousID, const CIdentifier& boxID, size_t index) const = 0;

    /**
     * \brief Tests whether a given id is a link or not
     * \param id the id to test
     * \retval true if the identified object is a link
     * \retval false if the identified object is not a link
     * \note Requesting a bad id returns \e false
     */
    virtual bool isLink(const CIdentifier& id) const = 0;

    /**
     * \brief Gets the details for a specific link
     * \param id The identifier of the link which details should be sent.
     * \return The link details
     */
    virtual const ILink* getLinkDetails(const CIdentifier& id) const = 0;

    /// \copydoc getLinkDetails(const CIdentifier&)const
    virtual ILink* getLinkDetails(const CIdentifier& id) = 0;

    /**
     * \brief Creates a connection between two boxes
     * \param[out] linkID The created link identifier.
     * \param srcBoxID The source box identifier
     * \param srcBoxOutputIdx The output index for the given source box
     * \param dstBoxID The target box identifier
     * \param dstBoxInputIndex The input index for the given target box
     * \param suggestedLinkID a suggestion for the new link identifier. If this specific identifier is not
     *        yet used, this scenario might use it. If the identifier is already used or \c CIdentifier::undefined() is passed,
     *        then a random unused identifier will be used.
     * \retval true In case of success.
     * \retval false In case of error. In such case, \c linkID remains unchanged.
     */
    virtual bool connect(CIdentifier& linkID, const CIdentifier& srcBoxID, const size_t srcBoxOutputIdx,
                         const CIdentifier& dstBoxID, const size_t dstBoxInputIndex, const CIdentifier& suggestedLinkID) = 0;

    /**
     * \brief Creates a connection between two boxes
     * \param[out] linkID The created link identifier.
     * \param srcBoxID The source box identifier
     * \param srcBoxOutputID The output identifier for the given source box
     * \param dstBoxID The target box identifier
     * \param dstBoxInputID The input identifier for the given target box
     * \param suggestedLinkID a suggestion for the new link identifier. If this specific identifier is not
     *        yet used, this scenario might use it. If the identifier is already used or \c CIdentifier::undefined() is passed,
     *        then a random unused identifier will be used.
     * \retval true In case of success.
     * \retval false In case of error. In such case, \c linkID remains unchanged.
     */
    virtual bool connect(CIdentifier& linkID, const CIdentifier& srcBoxID, const CIdentifier& srcBoxOutputID,
                         const CIdentifier& dstBoxID, const CIdentifier& dstBoxInputID, const CIdentifier& suggestedLinkID) = 0;

    /**
     * \brief Deletes a connection between two boxes
     * \param srcBoxID The source box identifier
     * \param srcBoxOutputIdx The output index for the given source box
     * \param dstBoxID The target box identifier
     * \param dstBoxInputIdx The input index for the given target box
     * \retval true In case of success.
     * \retval false In case of error.
     */
    virtual bool disconnect(const CIdentifier& srcBoxID, size_t srcBoxOutputIdx, const CIdentifier& dstBoxID, size_t dstBoxInputIdx) = 0;

    /**
     * \brief Deletes a connection between two boxes
     * \param srcBoxID The source box identifier
     * \param srcBoxOutputID The output identifier for the given source box
     * \param dstBoxID The target box identifier
     * \param dstBoxInputID The input identifier for the given target box
     * \retval true In case of success.
     * \retval false In case of error.
     */
    virtual bool disconnect(const CIdentifier& srcBoxID, const CIdentifier& srcBoxOutputID, const CIdentifier& dstBoxID,
                            const CIdentifier& dstBoxInputID) = 0;

    /**
     * \brief Deletes a connection between two boxes
     * \param[out] linkID The identifier for the link to be deleted
     * \retval true In case of success.
     * \retval false In case of error.
     */
    virtual bool disconnect(const CIdentifier& linkID) = 0;

    /**
     * \brief Get the output index of a source, for a specific box
     * \param[in] srcBoxID The source box identifier
     * \param[in] srcBoxOutputID The output identifier for the given source box
     * \param[out] srcBoxOutputIdx The output index for the given source box
     * \retval true in case of success.
     */
    virtual bool getSourceBoxOutputIndex(const CIdentifier& srcBoxID, const CIdentifier& srcBoxOutputID, size_t& srcBoxOutputIdx) = 0;

    /**
     * \brief Get the input index of a target, for a specific box
     * \param[in] dstBoxID The target box identifier
     * \param[in] dstBoxInputID The input identifier for the given target box
     * \param[out] dstBoxInputIdx The input index for the given target box
     * \retval true in case of success.
     */
    virtual bool getTargetBoxInputIndex(const CIdentifier& dstBoxID, const CIdentifier& dstBoxInputID, size_t& dstBoxInputIdx) = 0;

    /**
     * \brief  Get the output identifier of a source, for a specific box
     * \param srcBoxID The source box identifier
     * \param srcBoxOutputIdx The output index for the given source box
     * \param srcBoxOutputID The output identifier for the given source box
     * \retval true in case of success.
     */
    virtual bool getSourceBoxOutputIdentifier(const CIdentifier& srcBoxID, const size_t& srcBoxOutputIdx, CIdentifier& srcBoxOutputID) = 0;

    /**
     * \brief  Get the input identifier of a target, for a specific box
     * \param dstBoxID The target box identifier
     * \param dstBoxInputIdx The input index for the given target box
     * \param dstBoxInputID The input identifier for the given target box
     * \retval true in case of success.
     */
    virtual bool getTargetBoxInputIdentifier(const CIdentifier& dstBoxID, const size_t& dstBoxInputIdx, CIdentifier& dstBoxInputID) = 0;

    //@}
    /** \name Scenario Input/Output and MetaBox management */
    //@{

    virtual bool setHasIO(bool hasIO) = 0;
    virtual bool hasIO() const = 0;

    virtual bool setScenarioInputLink(size_t scenarioInputIdx, const CIdentifier& boxID, size_t boxInputIdx) = 0;
    virtual bool setScenarioInputLink(size_t scenarioInputIdx, const CIdentifier& boxID, const CIdentifier& boxInputID) = 0;
    virtual bool setScenarioOutputLink(size_t scenarioOutputIdx, const CIdentifier& boxID, size_t boxOutputIdx) = 0;
    virtual bool setScenarioOutputLink(size_t scenarioOutputIdx, const CIdentifier& boxID, const CIdentifier& boxOutputID) = 0;

    virtual bool getScenarioInputLink(size_t scenarioInputIdx, CIdentifier& boxID, size_t& boxInputIdx) const = 0;
    virtual bool getScenarioInputLink(size_t scenarioInputIdx, CIdentifier& boxID, CIdentifier& boxInputID) const = 0;
    virtual bool getScenarioOutputLink(size_t scenarioOutputIdx, CIdentifier& boxID, size_t& boxOutputIdx) const = 0;
    virtual bool getScenarioOutputLink(size_t scenarioOutputIdx, CIdentifier& boxID, CIdentifier& boxOutputID) const = 0;

    virtual bool removeScenarioInputLink(size_t scenarioInputIdx, const CIdentifier& boxID, size_t boxInputIdx) = 0;
    virtual bool removeScenarioOutputLink(size_t scenarioOutputIdx, const CIdentifier& boxID, size_t boxOutputIdx) = 0;

    virtual bool removeScenarioInput(size_t index) = 0;
    virtual bool removeScenarioOutput(size_t outputIdx) = 0;

    //@}
    /** \name Comment management */
    //@{

    /**
     * \brief Gets next comment identifier
     * \param previousID The identifier for the preceeding comment
     * \return The identifier of the next comment in case of success.
     * \retval CIdentifier::undefined() on error.
     * \note Giving \c CIdentifier::undefined() as \c previousID will cause this function to return the first comment identifier.
     */
    virtual CIdentifier getNextCommentIdentifier(const CIdentifier& previousID) const = 0;

    /**
     * \brief Tests whether a given identifier is a comment or not
     * \param commentID the identifier to test
     * \retval true if the identified object is a comment
     * \retval false if the identified object is not a comment
     * \note Requesting a bad identifier returns \e false
     */
    virtual bool isComment(const CIdentifier& commentID) const = 0;

    /**
     * \brief Gets the details for a specific comment
     * \param commentID The identifier of the comment which details should be sent.
     * \return The comment details
     */
    virtual const IComment* getCommentDetails(const CIdentifier& commentID) const = 0;

    /// \copydoc getCommentDetails(const CIdentifier&)const
    virtual IComment* getCommentDetails(const CIdentifier& commentID) = 0;

    /**
     * \brief Adds a new comment in the scenario
     * \param[out] commentID The identifier of the created comment
     * \param suggestedCommentID a suggestion for the new comment identifier. If this specific identifier is not
     *        yet used, this scenario might use it. If the identifier is already used or \c CIdentifier::undefined() is passed,
     *        then a random unused identifier will be used.
     * \retval true In case of success.
     * \retval false In case of error. In such case, \c commentID remains unchanged.
     * \note This produces an empty and unconfigured comment!
     */
    virtual bool addComment(CIdentifier& commentID, const CIdentifier& suggestedCommentID) = 0;

    /**
     * \brief Adds a new comment in the scenario based on an existing comment
     * \param[out] commentID The identifier of the created comment
     * \param comment the comment to copy in this scenario
     * \param suggestedCommentID a suggestion for the new comment identifier. If this specific identifier is not
     *        yet used, this scenario might use it. If the identifier is already used or \c CIdentifier::undefined() is passed,
     *        then a random unused identifier will be used.
     * \retval true In case of success.
     * \retval false In case of error. In such case, \c commentID remains unchanged.
     */
    virtual bool addComment(CIdentifier& commentID, const IComment& comment, const CIdentifier& suggestedCommentID) = 0;
    /**
     * \brief Removes a comment of the scenario
     * \param commentID The comment identifier
     * \retval true In case of success.
     * \retval false In case of error.
     */
    virtual bool removeComment(const CIdentifier& commentID) = 0;

    /**
     * \brief Get list of supported types for scenario parameters
     * 
     * \param types The vector in which supported types are stored
     */
    virtual void getAvailableParameterTypes(std::vector<std::string>& types) = 0;

    /**
     * \brief Add a parameter to the scenario with a specific value
     * \param name The name of the parameter to add
     * \param type The type of the parameter to add
     * \param value The value of the parameter to add
     */
    virtual void addParameter(const std::string& name, const std::string& type, const std::any& value) = 0;

    /**
     * \brief Add a parameter to the scenario with a default value
     * \param name The name of the parameter to add
     * \param type The type of the parameter to add
     */
    virtual void addParameter(const std::string& name, const std::string& type) = 0;

    /**
     * \brief Remove a parameter from the scenario
     * \param parameterName The name of the parameter to remove
     */
    virtual void removeParameter(const std::string& parameterName) = 0;

    /**
     *  \brief Update the name of a parameter
     *  \param oldName The old name of the parameter
     *  \param newName The new name of the parameter
     */
    virtual void updateParameterName(const std::string& oldName, const std::string& newName) = 0;

    /**
     * \brief Add a parameter connection between scenario parameter and a box parameter
     * \param parameterName The name of the scenario parameter
     * \param boxID The identifier of the box
     * \param boxParameterName The name of the box parameter
     */
    virtual void addParameterConnection(const std::string& parameterName, const CIdentifier& boxID, const std::string& boxParameterName) = 0;

    /**
     * \brief Remove a parameter connection between scenario parameter and a box parameter
     * \param parameterName The name of the scenario parameter
     * \param boxID The identifier of the box
     * \param boxParameterName The name of the box parameter
     */
    virtual void removeParameterConnection(const std::string& parameterName, const CIdentifier& boxID, const std::string& boxParameterNameD) = 0;

    /**
     * \brief Get the parameter connections for a given parameter
     * \param parameterName The name of the scenario parameter
     * \param connections [out] The active connections
     */
    virtual void getParameterConnections(const std::string& parameterName, std::vector<CScenarioParameterConnection*>& connections) const = 0;

    /**
     * \brief Get the parameter connection candidates for a given parameter.
     * These are the connections that are not actually effective
     * \param parameterName The name of the scenario parameter
     * \param connections [out] the candidate connections
     */
    virtual void getParameterConnectionCandidates(const std::string& parameterName, std::vector<CScenarioParameterConnection>& connections) const = 0;
    
    /**
     * \brief Get the connections count for a giver parameter
     * \param parameterName The name of the scenario parameter
     * 
     * \return the number of connections
     */
    virtual size_t getParameterConnectionsCount(const std::string& parameterName) const = 0;

    /**
     * \brief Get the details of a parameter connection at a specific index.
     * \param parameterName The name of the scenario parameter.
     * \param index The index of the connection to retrieve.
     * \param boxID [out] The identifier of the box connected to the parameter.
     * \param boxParameterName [out] The name of the box parameter connected to the scenario parameter.
     */
    virtual void getParameterConnectionDetails(const std::string& parameterName, size_t index, CIdentifier& boxID, std::string& boxParameterName) const = 0;

    //@}
    /** \name Metadata management */
    //@{

    /**
     * \brief Get next metadata identifier in regards to another
     * \param previousID The identifier of the metadata
     * \retval CIdentifier::undefined() In case when metadata with the \c previousID is not present
     * \retval CIdentifier::undefined() In case when metadata with the \c previousID is last in the scenario
     * \return The identifier of the next metadata
     * \note Giving \c CIdentifier::undefined() as \c previousID will cause this function to return the first metadata identifier.
     */
    virtual CIdentifier getNextMetadataIdentifier(const CIdentifier& previousID) const = 0;

    /**
     * \param metadataID The identifier to test
     * \retval true If the identified object is metadata
     * \retval false If the identified object is not metadata or when the identifier is not present in the scenario
     */
    virtual bool isMetadata(const CIdentifier& metadataID) const = 0;


    /**
     * \param metadataID The identifier of a metadata
     * \return Pointer to object containing metadata details
     */
    virtual const IMetadata* getMetadataDetails(const CIdentifier& metadataID) const = 0;

    /// \copydoc getMetadataDetails(const CIdentifier&)const
    virtual IMetadata* getMetadataDetails(const CIdentifier& metadataID) = 0;

    /**
     * \brief Add new metadata in the scenario
     * \param[out] metadataID The identifier of the newly created metadata
     * \param suggestedMetadataID A suggestion for the new identifier. If the identifier is already used or \c CIdentifier::undefined() is passed,
     *        then a random unused identifier will be used.
     * \retval true In case of success.
     * \retval false In case of error. In this case, \c metadataID remains unchanged.
     * \note This method creates an empty metadata.
     */
    virtual bool addMetadata(CIdentifier& metadataID, const CIdentifier& suggestedMetadataID) = 0;

    /**
     * \brief Remove metadata from the scenario
     * \param metadataID The metadata identifier
     * \retval true In case of success.
     * \retval false In case of error.
     */
    virtual bool removeMetadata(const CIdentifier& metadataID) = 0;
    //@}

    /**
     * \brief replaces settings of each box by its locally expanded version only expands the $var{} tokens, it leaves others as is
     */
    virtual bool applyLocalSettings() = 0;

    /**
     * \brief Check settings before playing scenario, if the settings are not suitable, stop scenario
     * and launch a console warning. Only check numeric values in the beginning
     * \param configManager local configuration manager that can contain the definition of local scenario settings
     */
    virtual bool checkSettings(IConfigurationManager* configManager) = 0;

    /**
     * \brief Check if boxes in scenario need to be updated. Feed an map of updates boxes instances with the identifiers
     * of outdated boxes
     * \return true if at least one box needs to updated
     */
    virtual bool checkOutdatedBoxes() = 0;

    /**
    * \brief Gets identifier of next outdated box
     * \param previousID The identifier for the preceeding outdated box
     * \return The identifier of the next box that needs updates in case of success.
     * \retval CIdentifier::undefined() on error.
     * \note Giving \c CIdentifier::undefined() as \c previousID will cause this function to return the first processing unit identifier.
     * \note Warning: You need to call at least once the function "checkOutdatedBoxes", before calling this function
     */
    virtual CIdentifier getNextOutdatedBoxIdentifier(const CIdentifier& previousID) const = 0;

    /**
     * \brief Indicates if at least one box in scenario need to be updated.
     * \return true if at least one box needs to updated
     * \note Warning: You need to call at least once the function "checkOutdatedBoxes", before calling this function
     */
    virtual bool hasOutdatedBox() = 0;

    /**
     * \brief Update the prototypes of the box identified by the given identifier.
     * \param boxID Identifier of box.
     * \retval true in case of success
     * \retval false in case of error
     */
    virtual bool updateBox(const CIdentifier& boxID) = 0;

    /**
     * \brief Remove deprecated interfacors from the box identified by the given identifier.
     * \param boxID Id of the box to clean up
     * \retval true in case of success
     * \retval false if the box does not exist
     */
    virtual bool removeDeprecatedInterfacorsFromBox(const CIdentifier& boxID) = 0;

    /**
     * \brief Check if scenario contains a box with a deprecated interfacor due to an incomplete update
     * \retval true if a box in the scenario has a deprecated interfacor
     * \retval false if no box in the scenario contains a deprecated interfacor
     */
    virtual bool containsBoxWithDeprecatedInterfacors() const = 0;

    /**
     * \return true if the scenario is actually a metabox
     */
    virtual bool isMetabox() = 0;

    virtual void getBoxIdentifierList(CIdentifier** listID, size_t* size) const = 0;
    virtual void getCommentIdentifierList(CIdentifier** listID, size_t* size) const = 0;
    virtual void getMetadataIdentifierList(CIdentifier** listID, size_t* size) const = 0;
    virtual void getLinkIdentifierList(CIdentifier** listID, size_t* size) const = 0;
    virtual void getLinkIdentifierFromBoxList(const CIdentifier& boxID, CIdentifier** listID, size_t* size) const = 0;
    virtual void getLinkIdentifierFromBoxOutputList(const CIdentifier& boxID, size_t outputIdx, CIdentifier** listID, size_t* size) const = 0;
    virtual void getLinkIdentifierToBoxList(const CIdentifier& boxID, CIdentifier** listID, size_t* size) const = 0;
    virtual void getLinkIdentifierToBoxInputList(const CIdentifier& boxID, size_t inputIdx, CIdentifier** listID, size_t* size) const = 0;
    virtual void getOutdatedBoxIdentifierList(CIdentifier** listID, size_t* size) const = 0;
    virtual void releaseIdentifierList(CIdentifier* listID) const = 0;

    _IsDerivedFromClass_(IBox, OV_ClassId_Kernel_Scenario_Scenario)
};
}  // namespace Kernel
}  // namespace OpenViBE
