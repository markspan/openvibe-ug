#include <openvibe/ovExceptionHandler.h>

#include "ovkCScheduler.h"
#include "ovkCSimulatedBox.h"
#include "ovkCPlayer.h"
#include "../scenario/ovkCScenarioSettingKeywordParserCallback.h"
#include "ovkCBoxSettingModifierVisitor.h"

#include <fs/Files.h>

#include <string>

#include <cstdlib>
#include <set>

#if defined TARGET_OS_Windows
#define stricmp _stricmp
#elif defined TARGET_OS_Linux || defined TARGET_OS_MacOS
 #include <strings.h>
 #define stricmp strcasecmp
#else
// TODO
#endif

#include <ovp_global_defines.h>

namespace OpenViBE {
namespace Kernel {

#define OVTK_Algorithm_ScenarioImporter_OutputParameterId_Scenario      CIdentifier(0x29574C87, 0x7BA77780)
#define OVTK_Algorithm_ScenarioImporter_InputParameterId_MemoryBuffer   CIdentifier(0x600463A3, 0x474B7F66)

#define OV_AttributeId_Box_Disabled                                     CIdentifier(0x341D3912, 0x1478DE86)

CScheduler::CScheduler(const IKernelContext& ctx, CPlayer& player)
    : TKernelObject<IKernelObject>(ctx), m_rPlayer(player), m_scenarioID(CIdentifier::undefined()) {}

CScheduler::~CScheduler() { this->uninitialize(); }

//___________________________________________________________________//
//                                                                   //

bool CScheduler::setScenario(const CIdentifier& scenarioID)
{
    this->getLogManager() << LogLevel_Trace << "Scheduler setScenario\n";

    OV_ERROR_UNLESS_KRF(!this->isHoldingResources(), "Trying to configure a scheduler with non-empty resources", Kernel::ErrorType::BadCall);

    m_scenarioID = scenarioID;

    // We need to flatten the scenario here as the application using the scheduler needs time
    // between the moment the visualisation tree is complete and the moment when boxes are initialized.
    // The application needs to initialize necessary windows for the boxes to draw into.
    m_scenario = &m_rPlayer.getRuntimeScenarioManager().getScenario(m_scenarioID);

    if (!this->flattenScenario())
    {
        // error handling is performed within flattenScenario
        return false;
    }

    m_scenario = nullptr;

    return true;
}

bool CScheduler::setFrequency(const uint64_t frequency)
{
    this->getLogManager() << LogLevel_Trace << "Scheduler setFrequency\n";

    OV_ERROR_UNLESS_KRF(!this->isHoldingResources(), "Trying to configure a scheduler with non-empty resources", Kernel::ErrorType::BadCall);

    m_frequency    = frequency;
    m_stepDuration = (1LL << 32) / frequency;
    return true;
}

//___________________________________________________________________//
//                                                                   //

bool CScheduler::isHoldingResources() const { return !m_simulatedBoxes.empty(); }

//___________________________________________________________________//
//                                                                   //

bool CScheduler::flattenScenario()
{
    OV_ERROR_UNLESS_KRF(m_scenario->applyLocalSettings(), "Failed to flatten scenario: applying local settings failed", Kernel::ErrorType::Internal);

    // We are going to find all metaboxes in the scenario and then push their contents to this one
    // As the scenario itself can contain more metaboxes, we are going to repeat this process
    // util there are no unhandled metaboxes left
    bool hasFinishedHandlingMetaboxes = false;

    while (!hasFinishedHandlingMetaboxes)
    {
        // First find all the metaboxes in the scenario
        std::vector<CIdentifier> scenarioMetaboxes;

        {
            CIdentifier* listID = nullptr;
            size_t nbElems      = 0;
            m_scenario->getBoxIdentifierList(&listID, &nbElems);
            for (size_t i = 0; i < nbElems; ++i)
            {
                const CIdentifier boxID = listID[i];
                const IBox* box         = m_scenario->getBoxDetails(boxID);

                if (box->getAlgorithmClassIdentifier() == OVP_ClassId_BoxAlgorithm_Metabox)
                {
                    if (box->hasAttribute(OV_AttributeId_Box_Disabled)) // We only process this box if it is not disabled
                    {
                        m_scenario->removeBox(boxID);
                    }
                    else if (box->hasAttribute(OVP_AttributeId_Metabox_ID)) // We verify that the box actually has a backend scenario
                    {
                        CIdentifier metaboxId;
                        metaboxId.fromString(box->getAttributeValue(OVP_AttributeId_Metabox_ID));
                        CString metaboxScenarioPath(this->getKernelContext().getMetaboxManager().getMetaboxFilePath(metaboxId));

                        if (FS::Files::fileExists(metaboxScenarioPath.toASCIIString()))
                        {
                            // If the scenario exists we will handle this metabox
                            scenarioMetaboxes.push_back(boxID);
                        }
                        else
                        {
                            // Non-utilisable metaboxes can be easily removed
                            OV_WARNING_K("The scenario for metabox [" << metaboxId.str() << "] is missing.");
                            m_scenario->removeBox(boxID);
                        }
                    }
                    else
                    {
                        OV_WARNING_K("The metabox [" << boxID << "] is missing its identifier field.");
                        m_scenario->removeBox(boxID);
                    }
                }
            }
            m_scenario->releaseIdentifierList(listID);
        }

        if (scenarioMetaboxes.empty()) { hasFinishedHandlingMetaboxes = true; }

        // Now load each of the found metaboxes, load the scenario that represents it
        // Assign the settings from the box to the scenario
        // Calculate new settings for each box inside this scenario (by expanding $var settings)
        // Merge the scenario into this one
        // Re-connect all links for the scenario

        for (const auto& boxID : scenarioMetaboxes)
        {
            IBox* box = m_scenario->getBoxDetails(boxID);

            // The box has an attribute with the metabox ID and config manager has a path to each metabox scenario
            CString str = box->getAttributeValue(OVP_AttributeId_Metabox_ID);
            CIdentifier metaboxId;
            metaboxId.fromString(str);
            CString metaboxScenarioPath(this->getKernelContext().getMetaboxManager().getMetaboxFilePath(metaboxId));

            OV_ERROR_UNLESS_KRF(str != CString(""), "Failed to find metabox with id " << str, Kernel::ErrorType::ResourceNotFound);

            // We are going to copy the template scenario, flatten it and then copy all
            // Note that copy constructor for IScenario does not exist
            CIdentifier metaboxScenarioTemplateID;

            OV_ERROR_UNLESS_KRF(
                m_rPlayer.getRuntimeScenarioManager().importScenarioFromFile(metaboxScenarioTemplateID,
                    OV_ScenarioImportContext_SchedulerMetaboxImport, metaboxScenarioPath),
                "Failed to import the scenario file", Kernel::ErrorType::Internal);

            IScenario& metaboxScenarioInstance = m_rPlayer.getRuntimeScenarioManager().getScenario(metaboxScenarioTemplateID);

            OV_WARNING_UNLESS_K(metaboxScenarioInstance.hasAttribute(OV_AttributeId_Scenario_MetaboxHash),
                                "Box " << box->getName() << " [" << metaboxScenarioPath << "] has no computed hash");

            OV_WARNING_UNLESS_K(
                box->getAttributeValue(OV_AttributeId_Box_InitialPrototypeHashValue) == metaboxScenarioInstance.getAttributeValue(
                    OV_AttributeId_Scenario_MetaboxHash),
                "Box " << box->getName() << " [" << str << "] should be updated");

            metaboxScenarioInstance.addAttribute(OV_AttributeId_ScenarioFilename, metaboxScenarioPath);

            // Push down the settings from the box to the scenario
            for (size_t idx = 0; idx < box->getSettingCount(); ++idx)
            {
                CString value;
                CIdentifier id;

                box->getSettingValue(idx, value);
                box->getInterfacorIdentifier(Setting, idx, id);

                if (id != CIdentifier::undefined()) { metaboxScenarioInstance.setSettingValue(id, value); }
                else { metaboxScenarioInstance.setSettingValue(idx, value); }
            }

            // Create settings with the path to the Metabox,
            // these settings will be accessible from within the Metabox on runtime
            std::string metaboxFilename      = metaboxScenarioPath.toASCIIString();
            std::string metaboxDirectoryPath = ".";
            metaboxScenarioInstance.addSetting("Player_MetaboxScenarioFilename", OV_TypeId_Filename, metaboxScenarioPath);

            const size_t lastSlashPos = metaboxFilename.rfind('/');
            if (lastSlashPos != std::string::npos) { metaboxDirectoryPath = metaboxFilename.substr(0, lastSlashPos); }

            metaboxScenarioInstance.addSetting("Player_MetaboxScenarioDirectory", OV_TypeId_Foldername, metaboxDirectoryPath.c_str());

            // apply the settings within the loaded scenario
            metaboxScenarioInstance.applyLocalSettings();

            std::map<CIdentifier, CIdentifier> correspondenceID;
            class CScenarioMergeCallback : public IScenario::IScenarioMergeCallback
            {
            public:
                explicit
                CScenarioMergeCallback(std::map<CIdentifier, CIdentifier>& rIdentifierCorrespondence) : m_correspondenceID(rIdentifierCorrespondence) { }

                void process(CIdentifier& originalID, CIdentifier& newId) override { m_correspondenceID[originalID] = newId; }

            private:
                std::map<CIdentifier, CIdentifier>& m_correspondenceID;
            };

            CScenarioMergeCallback scenarioMergeCB(correspondenceID);

            // Copy the boxes and the links from the template metabox scenario to this one
            m_scenario->merge(metaboxScenarioInstance, &scenarioMergeCB, false, false);

            // Now reconnect all the pipes

            // Connect metabox inputs
            {
                CIdentifier* listID = nullptr;
                size_t nbElems      = 0;
                m_scenario->getLinkIdentifierToBoxList(box->getIdentifier(), &listID, &nbElems);
                for (size_t i = 0; i < nbElems; ++i)
                {
                    ILink* link = m_scenario->getLinkDetails(listID[i]);
                    // Find out the target inside the metabox scenario
                    CIdentifier dstBoxID;
                    size_t dstBoxInputIdx      = 0;
                    CIdentifier metaBoxInputID = link->getTargetBoxInputIdentifier();
                    size_t metaBoxInputIdx     = link->getTargetBoxInputIndex();

                    if (metaBoxInputID != CIdentifier::undefined()) { metaboxScenarioInstance.getInterfacorIndex(Input, metaBoxInputID, metaBoxInputIdx); }
                    OV_ERROR_UNLESS_KRF(metaBoxInputIdx != size_t(-1), "Failed to find metabox input with identifier " << metaBoxInputID.str(),
                                        ErrorType::ResourceNotFound);
                    metaboxScenarioInstance.getScenarioInputLink(metaBoxInputIdx, dstBoxID, dstBoxInputIdx);

                    // Now redirect the link to the newly created copy of the box in the scenario
                    CIdentifier dstBoxInputID = CIdentifier::undefined();
                    if (dstBoxID != CIdentifier::undefined())
                    {
                        m_scenario->getBoxDetails(correspondenceID[dstBoxID])->getInterfacorIdentifier(Input, dstBoxInputIdx, dstBoxInputID);
                        link->setTarget(correspondenceID[dstBoxID], dstBoxInputIdx, dstBoxInputID);
                    }
                }
                m_scenario->releaseIdentifierList(listID);
            }

            // Connect metabox outputs
            {
                CIdentifier* listID = nullptr;
                size_t nbElems      = 0;
                m_scenario->getLinkIdentifierFromBoxList(box->getIdentifier(), &listID, &nbElems);
                for (size_t i = 0; i < nbElems; ++i)
                {
                    ILink* link = m_scenario->getLinkDetails(listID[i]);
                    // Find out the Source inside the metabox scenario
                    CIdentifier srcBoxID;
                    size_t srcBoxOutputIdx      = 0;
                    CIdentifier metaBoxOutputID = link->getSourceBoxOutputIdentifier();
                    size_t metaBoxOutputIdx     = link->getSourceBoxOutputIndex();

                    if (metaBoxOutputID != CIdentifier::undefined()) { metaboxScenarioInstance.getInterfacorIndex(Output, metaBoxOutputID, metaBoxOutputIdx); }
                    OV_ERROR_UNLESS_KRF(metaBoxOutputIdx != size_t(-1),
                                        "Failed to find metabox input with identifier " << metaBoxOutputID.str(), Kernel::ErrorType::ResourceNotFound);
                    metaboxScenarioInstance.getScenarioOutputLink(metaBoxOutputIdx, srcBoxID, srcBoxOutputIdx);

                    // Now redirect the link to the newly created copy of the box in the scenario
                    CIdentifier srcBoxOutputID = CIdentifier::undefined();
                    if (srcBoxID != CIdentifier::undefined())
                    {
                        m_scenario->getBoxDetails(correspondenceID[srcBoxID])->getInterfacorIdentifier(Output, srcBoxOutputIdx, srcBoxOutputID);

                        link->setSource(correspondenceID[srcBoxID], srcBoxOutputIdx, srcBoxOutputID);
                    }
                }
                m_scenario->releaseIdentifierList(listID);
            }
        }

        // Remove processed metaboxes from the scenario
        for (const CIdentifier& metaboxID : scenarioMetaboxes) { m_scenario->removeBox(metaboxID); }
    }

    return true;
}

ESchedulerInitialization CScheduler::initialize()
{
    this->getLogManager() << LogLevel_Trace << "Scheduler initialize\n";

    OV_ERROR_UNLESS_K(!this->isHoldingResources(), "Trying to configure a scheduler with non-empty resources", Kernel::ErrorType::BadCall,
                      ESchedulerInitialization::Failed);

    m_scenario = &m_rPlayer.getRuntimeScenarioManager().getScenario(m_scenarioID);

    OV_ERROR_UNLESS_K(m_scenario, "Failed to find scenario with id " << m_scenarioID.str(), Kernel::ErrorType::ResourceNotFound,
                      ESchedulerInitialization::Failed);

    OV_ERROR_UNLESS_K(m_scenario->getNextBoxIdentifier(CIdentifier::undefined()) != CIdentifier::undefined(),
                      "Cannot initialize scheduler with an empty scenario", Kernel::ErrorType::BadCall, ESchedulerInitialization::Failed);

    CBoxSettingModifierVisitor boxSettingModifierVisitor(&this->getKernelContext().getConfigurationManager());

    OV_ERROR_UNLESS_K(m_scenario->acceptVisitor(boxSettingModifierVisitor), "Failed to set box settings visitor for scenario with id "
                      << m_scenarioID.str(), Kernel::ErrorType::Internal, ESchedulerInitialization::Failed);


    {
        CIdentifier* listID = nullptr;
        size_t nbElems      = 0;
        m_scenario->getBoxIdentifierList(&listID, &nbElems);

        // Create sets of x and y positions, note that C++ sets are always ordered
        // We work with indexes rather than just positions because the positions are not bounded and can overflow
        std::set<int> xpositions;
        std::set<int> ypositions;
        for (size_t i = 0; i < nbElems; ++i)
        {
            const IBox* box = m_scenario->getBoxDetails(listID[i]);

            if (box->hasAttribute(OV_AttributeId_Box_YCenterPosition))
            {
                try
                {
                    int y = std::stoi(box->getAttributeValue(OV_AttributeId_Box_YCenterPosition).toASCIIString());
                    ypositions.insert(y);
                }
                catch (const std::exception&)
                {
                    OV_WARNING_K(
                        "The Y position (" << box->getAttributeValue(OV_AttributeId_Box_YCenterPosition) << ") " << " in the Box " << listID[i] <<
                        " is corrupted");
                }
            }
            if (box->hasAttribute(OV_AttributeId_Box_XCenterPosition))
            {
                try
                {
                    int x = std::stoi(box->getAttributeValue(OV_AttributeId_Box_XCenterPosition).toASCIIString());
                    xpositions.insert(x);
                }
                catch (const std::exception&)
                {
                    OV_WARNING_K(
                        "The X position (" << box->getAttributeValue(OV_AttributeId_Box_XCenterPosition) << ") " << " in the Box " << listID[i] <<
                        " is corrupted");
                }
            }
        }

        for (size_t i = 0; i < nbElems; ++i)
        {
            const CIdentifier boxID = listID[i];
            const IBox* box         = m_scenario->getBoxDetails(boxID);
            OV_ERROR_UNLESS_K(
                !m_scenario->hasOutdatedBox() || !this->getConfigurationManager().expandAsBoolean("${Kernel_AbortPlayerWhenBoxIsOutdated}", false),
                "Box [" << box->getName() << "] with class identifier [" << boxID.str() << "] should be updated",
                ErrorType::Internal, ESchedulerInitialization::Failed);

            OV_ERROR_UNLESS_K(box->getAlgorithmClassIdentifier() != OVP_ClassId_BoxAlgorithm_Metabox,
                              "Not expanded metabox with id [" << box->getAttributeValue(OVP_AttributeId_Metabox_ID) << "] detected in the scenario",
                              ErrorType::Internal, ESchedulerInitialization::Failed);

            const Plugins::IPluginObjectDesc* boxDesc = this->getPluginManager().getPluginObjectDescCreating(box->getAlgorithmClassIdentifier());

            this->getLogManager() << LogLevel_Info << "elem " << i << " box name = " << box->getName() 
                                  << " algoClassIdentifier = " << box->getAlgorithmClassIdentifier()
                                  << "  boxDesc = " << boxDesc << "\n";

            OV_ERROR_UNLESS_K(
                !(box->hasAttribute(OV_AttributeId_Box_Disabled) && this->getConfigurationManager().expandAsBoolean("${Kernel_AbortPlayerWhenBoxIsDisabled}",
                    false)),
                "Disabled box [" << box->getName() << "] with class identifier [" << boxID.str() << "] detected in the scenario",
                ErrorType::Internal, ESchedulerInitialization::Failed);

            if (!box->hasAttribute(OV_AttributeId_Box_Disabled))
            {
                OV_ERROR_UNLESS_K(boxDesc != nullptr,
                                  "Failed to create runtime box [" << box->getName() << "] with class identifier [" << boxID.str() << "]",
                                  ErrorType::BadResourceCreation, ESchedulerInitialization::Failed);

                CSimulatedBox* simulatedBox = new CSimulatedBox(this->getKernelContext(), *this);
                simulatedBox->setScenarioIdentifier(m_scenarioID);
                simulatedBox->setBoxIdentifier(boxID);


                // Set priority so boxes execute in this order
                // - boxes with positive priority attribute set in descending order
                // - boxes in scenario from left to right and from top to bottom
                // - boxes with negative priority set in descending order
                int priority = 0;
                try
                {
                    if (box->hasAttribute(OV_AttributeId_Box_Priority))
                    {
                        const int p = std::stoi(box->getAttributeValue(OV_AttributeId_Box_Priority).toASCIIString());

                        if (p < 0) { priority = -int((ypositions.size() << 15) + xpositions.size()) + p; }
                        else { priority = p; }
                    }
                    else if (box->hasAttribute(OV_AttributeId_Box_YCenterPosition) && box->hasAttribute(OV_AttributeId_Box_XCenterPosition))
                    {
                        int x = std::stoi(box->getAttributeValue(OV_AttributeId_Box_XCenterPosition).toASCIIString());
                        int y = std::stoi(box->getAttributeValue(OV_AttributeId_Box_YCenterPosition).toASCIIString());

                        const int xindex = int(std::distance(xpositions.begin(), xpositions.find(x)));
                        const int yindex = int(std::distance(ypositions.begin(), ypositions.find(y)));
                        priority         = - ((yindex << 15) + xindex);
                    }
                }
                catch (const std::exception&) { priority = 0; }

                m_simulatedBoxes[std::make_pair(-priority, boxID)] = simulatedBox;
                m_simulatedBoxChronos[boxID].reset(m_frequency);
            }
        }
        m_scenario->releaseIdentifierList(listID);
    }

    OV_ERROR_UNLESS_K(!m_simulatedBoxes.empty(), "Cannot initialize scheduler with an empty scenario", Kernel::ErrorType::BadCall,
                      ESchedulerInitialization::Failed);


    bool boxInitialization = true;
    for (auto it = m_simulatedBoxes.begin(); it != m_simulatedBoxes.end(); ++
         it)
    {
        if (auto simulatedBox = it->second)
        {
            this->getLogManager() << LogLevel_Trace << "Scheduled box : id = " << it->first.second << " priority = " << -it->first.first
                    << " name = " << simulatedBox->getName() << "\n";
            if (!translateException([&]() { return simulatedBox->initialize(); },
                                    std::bind(&CScheduler::handleException, this, simulatedBox, "Box initialization", std::placeholders::_1)))
            {
                boxInitialization = false;

                // return as soon as possible
                // no need to keep on initializing boxes if a box failed to initialize
                break;
            }
        }
    }

    m_steps       = 0;
    m_currentTime = 0;

    m_oBenchmarkChrono.reset(m_frequency);

    return (boxInitialization ? ESchedulerInitialization::Success : ESchedulerInitialization::Failed);
}

bool CScheduler::uninitialize()
{
    this->getLogManager() << LogLevel_Trace << "Scheduler uninitialize\n";

    bool boxUninitialization = true;
    for (auto it = m_simulatedBoxes.begin(); it != m_simulatedBoxes.end(); ++
         it)
    {
        if (auto simulatedBox = it->second)
        {
            if (!translateException([&]() { return simulatedBox->uninitialize(); },
                                    std::bind(&CScheduler::handleException, this, simulatedBox, "Box uninitialization", std::placeholders::_1)))
            {
                // do not break here because we want to try to
                // at least uninitialize other resources properly
                boxUninitialization = false;
            }
        }
    }

    for (auto it = m_simulatedBoxes.begin(); it != m_simulatedBoxes.end(); ++it) { delete it->second; }
    m_simulatedBoxes.clear();

    m_scenario = nullptr;

    return boxUninitialization;
}

//___________________________________________________________________//
//                                                                   //

bool CScheduler::loop()
{
    OV_ERROR_UNLESS_KRF(this->isHoldingResources(), "Trying to use an uninitialized scheduler", Kernel::ErrorType::BadCall);

    bool boxProcessing = true;
    m_oBenchmarkChrono.stepIn();
    for (auto it = m_simulatedBoxes.begin(); it != m_simulatedBoxes.end(); ++
         it)
    {
        CSimulatedBox* simulatedBox = it->second;

        System::CChrono& simulatedBoxChrono = m_simulatedBoxChronos[it->first.second];

        IBox* box = m_scenario->getBoxDetails(it->first.second);

        OV_ERROR_UNLESS_KRF(box, "Unable to get box details for box with id " << it->first.second.str(), Kernel::ErrorType::ResourceNotFound);

        simulatedBoxChrono.stepIn();

        if (!translateException([&]() { return this->processBox(simulatedBox, it->first.second); },
                                std::bind(&CScheduler::handleException, this, simulatedBox, "Box processing", std::placeholders::_1)))
        {
            boxProcessing = false;

            // break here because we do not want to keep on processing if one
            // box fails
            break;
        }

        simulatedBoxChrono.stepOut();

        if (simulatedBoxChrono.hasNewEstimation())
        {
            //IBox* box=m_scenario->getBoxDetails(it->first.second);
            box->addAttribute(OV_AttributeId_Box_ComputationTimeLastSecond, "");
            box->setAttributeValue(OV_AttributeId_Box_ComputationTimeLastSecond, CIdentifier(simulatedBoxChrono.getTotalStepInDuration()).toString());
        }
    }
    m_oBenchmarkChrono.stepOut();

    if ((m_steps % m_frequency) == 0)
    {
        this->getLogManager() << LogLevel_Debug
                << "<" << LogColor_PushStateBit << LogColor_ForegroundBlue << "Scheduler" << LogColor_PopStateBit
                << "::" << LogColor_PushStateBit << LogColor_ForegroundBlue << "elapsed time" << LogColor_PopStateBit << "> "
                << m_steps / m_frequency << "s\n";
    }

    if (m_oBenchmarkChrono.hasNewEstimation())
    {
        this->getLogManager() << LogLevel_Benchmark
                << "<" << LogColor_PushStateBit << LogColor_ForegroundBlue << "Scheduler" << LogColor_PopStateBit
                << "::" << LogColor_PushStateBit << LogColor_ForegroundBlue << "processor use" << LogColor_PopStateBit << "> "
                << m_oBenchmarkChrono.getStepInPercentage() << "%\n";
    }

    m_steps++;

    m_currentTime = m_steps * CTime(m_frequency, 1LL).time();

    return boxProcessing;
}

bool CScheduler::processBox(CSimulatedBox* simulatedBox, const CIdentifier& boxID)
{
    if (simulatedBox)
    {
        OV_ERROR_UNLESS_KRF(simulatedBox->processClock(), "Process clock failed for box with id " << boxID.str(), Kernel::ErrorType::Internal);
        if (simulatedBox->isReadyToProcess())
        {
            OV_ERROR_UNLESS_KRF(simulatedBox->process(), "Process failed for box with id " << boxID.str(), Kernel::ErrorType::Internal);
        }

        //if the box is muted we still have to erase chunks that arrives at the input
        std::map<size_t, std::list<CChunk>>& simulatedBoxInput = m_simulatedBoxInputs[boxID];
        for (auto it1 = simulatedBoxInput.begin(); it1 != simulatedBoxInput.end(); ++it1)
        {
            std::list<CChunk>& simulatedBoxInputChunkList = it1->second;
            for (auto it2 = simulatedBoxInputChunkList.begin(); it2 != simulatedBoxInputChunkList.end(); ++it2)
            {
                OV_ERROR_UNLESS_KRF(simulatedBox->processInput(it1->first, *it2),
                                    "Process failed for box with id " << boxID.str() << " on input " << it1->first,
                                    ErrorType::Internal);

                if (simulatedBox->isReadyToProcess())
                {
                    OV_ERROR_UNLESS_KRF(simulatedBox->process(), "Process failed for box with id " << boxID.str(), Kernel::ErrorType::Internal);
                }
            }
            simulatedBoxInputChunkList.clear();
        }
    }

    return true;
}

//___________________________________________________________________//
//                                                                   //

bool CScheduler::sendInput(const CChunk& chunk, const CIdentifier& boxId, const size_t index)
{
    IBox* box = m_scenario->getBoxDetails(boxId);
    if (box->hasAttribute(OV_AttributeId_Box_Disabled)) { return true; }
    OV_ERROR_UNLESS_KRF(box, "Tried to send data chunk with invalid box identifier " << boxId.str(), Kernel::ErrorType::ResourceNotFound);

    OV_ERROR_UNLESS_KRF(index < box->getInputCount(),
                        "Tried to send data chunk with invalid input index " << index << " for box identifier" << boxId.str(),
                        ErrorType::OutOfBound);

    auto it = m_simulatedBoxes.begin();
    while (it != m_simulatedBoxes.end() && it->first.second != boxId) { ++it; }

    OV_ERROR_UNLESS_KRF(it != m_simulatedBoxes.end(),
                        "Tried to send data chunk with invalid simulated box identifier " << boxId.str(),
                        ErrorType::ResourceNotFound);
    CSimulatedBox* simulatedBox = it->second;

    // use a fatal here because failing to meet this invariant
    // means there is a bug in the scheduler implementation
    OV_FATAL_UNLESS_K(simulatedBox, "Null box found for id " << boxId.str(), Kernel::ErrorType::BadValue);

    // TODO: check if index does not overflow

    m_simulatedBoxInputs[boxId][index].push_back(chunk);

    return true;
}

uint64_t CScheduler::getCurrentLateness() const { return m_rPlayer.getCurrentSimulatedLateness(); }
double CScheduler::getFastForwardMaximumFactor() const { return m_rPlayer.getFastForwardMaximumFactor(); }

void CScheduler::handleException(const CSimulatedBox* box, const char* errorHint, const std::exception& exception)
{
    CIdentifier dstBoxID = CIdentifier::undefined();
    box->getBoxIdentifier(dstBoxID);

    box->getLogManager() << LogLevel_Error << "Exception caught in box\n";
    box->getLogManager() << LogLevel_Error << "  [name:" << box->getName() << "]\n";
    box->getLogManager() << LogLevel_Error << "  [class identifier:" << dstBoxID << "]\n";
    box->getLogManager() << LogLevel_Error << "  [hint: " << (errorHint ? errorHint : "no hint") << "]\n";
    box->getLogManager() << LogLevel_Error << "  [cause:" << exception.what() << "]\n";

    OV_ERROR_KRV("Caught exception: " << exception.what(), Kernel::ErrorType::ExceptionCaught);
}

}  // namespace Kernel
}  // namespace OpenViBE
