#include <vector>
#include <tuple>
#include <ovp_global_defines.h>

#include "ovtAssert.h"
#include "ovtTestFixtureCommon.h"

int urImportScenarioFromFileTest(int argc, char* argv[]);

#define OVP_ClassId_BoxAlgorithm_ClockStimulator        CIdentifier(0x4F756D3F, 0x29FF0B96)
#define OVP_ClassId_BoxAlgorithm_StimulationListener    CIdentifier(0x65731E1D, 0x47DE5276)

#include "urSimpleTestScenarioDefinition.h"

int urExportScenarioToFileTest(const int argc, char* argv[])
{
    Test::ScopedTest<Test::SKernelFixture> fixture(argv[1]);

    const auto& context                  = fixture->context;
    const std::string temporaryDirectory = argv[2];

    context->getPluginManager().addPluginsFromFiles(Directories::getLib("plugins-sdk-file-io*"));
    context->getPluginManager().addPluginsFromFiles(Directories::getLib("plugins-sdk-stimulation*"));
    context->getPluginManager().addPluginsFromFiles(Directories::getLib("plugins-sdk-tools*"));

    CIdentifier emptyScenarioIdentifier;
    context->getScenarioManager().createScenario(emptyScenarioIdentifier);

    CMemoryBuffer memoryBuffer;
    OVT_ASSERT(context->getScenarioManager().exportScenario(memoryBuffer, emptyScenarioIdentifier, OVP_GD_ClassId_Algorithm_XMLScenarioExporter),
               "Failed to export an empty scenario");
    OVT_ASSERT(
        context->getScenarioManager().exportScenarioToFile((temporaryDirectory + "/test-scenario-empty.mxs").c_str(), emptyScenarioIdentifier,
            OVP_GD_ClassId_Algorithm_XMLScenarioExporter), "Failed to export an empty scenario to a file");

    CIdentifier importedEmptyScenarioIdentifier;
    OVT_ASSERT(context->getScenarioManager().importScenario(importedEmptyScenarioIdentifier, memoryBuffer, OVP_GD_ClassId_Algorithm_XMLScenarioImporter),
               "Failed to import an empty exported buffer");

    // Create a scenario to be imported by the importer test
    CIdentifier simpleScenarioIdentifier;
    context->getScenarioManager().createScenario(simpleScenarioIdentifier);

    Kernel::IScenario& scenario = context->getScenarioManager().getScenario(simpleScenarioIdentifier);

    // Test scenario attributes
    for (auto& attribute : simpleScenarioAttributes) { scenario.addAttribute(std::get<0>(attribute), std::get<1>(attribute).c_str()); }

    size_t settingIndex = 0;
    for (auto& setting : simpleScenarioSettings) {
        scenario.addSetting(std::get<1>(setting).c_str(), std::get<0>(setting), std::get<2>(setting).c_str());
        scenario.setSettingValue(settingIndex, std::get<3>(setting).c_str());
        settingIndex += 1;
    }

    CIdentifier actualClockStimulatorBoxId;
    scenario.addBox(actualClockStimulatorBoxId, OVP_ClassId_BoxAlgorithm_ClockStimulator, s_ClockStimulatorBoxId);

    CIdentifier actualStimulationListenerBoxId;
    scenario.addBox(actualStimulationListenerBoxId, OVP_ClassId_BoxAlgorithm_StimulationListener, s_StimulationListenerBoxId);

    Kernel::IBox* stimulatorListenerBox = scenario.getBoxDetails(s_StimulationListenerBoxId);
    stimulatorListenerBox->addInput("Stimulation stream 2", OV_TypeId_Stimulations);

    int scenarioInputIdx = 0;
    for (auto& scenarioInput : simpleScenarioInputs) {
        scenario.addInput(std::get<1>(scenarioInput).c_str(), std::get<0>(scenarioInput));
        if (std::get<2>(scenarioInput) != CIdentifier::undefined()) {
            scenario.setScenarioInputLink(scenarioInputIdx, std::get<2>(scenarioInput), std::get<3>(scenarioInput));
        }
        scenarioInputIdx += 1;
    }

    int scenarioOutputIdx = 0;
    for (auto& scenarioOutput : simpleScenarioOutputs) {
        scenario.addOutput(std::get<1>(scenarioOutput).c_str(), std::get<0>(scenarioOutput));
        if (std::get<2>(scenarioOutput) != CIdentifier::undefined()) {
            scenario.setScenarioOutputLink(scenarioOutputIdx, std::get<2>(scenarioOutput), std::get<3>(scenarioOutput));
        }
        scenarioOutputIdx += 1;
    }

    CIdentifier actualClockStimulatorToStimulationListenerLinkId;
    scenario.connect(actualClockStimulatorToStimulationListenerLinkId, s_ClockStimulatorBoxId, 0, s_StimulationListenerBoxId, 1,
                     s_ClockStimulatorToStimulationListenerLinkId);

    CIdentifier actualSimpleCommentIdentifier;
    scenario.addComment(actualSimpleCommentIdentifier, s_SimpleCommentId);

    Kernel::IComment* simpleComment = scenario.getCommentDetails(s_SimpleCommentId);
    simpleComment->setText("Content of a comment");

    CIdentifier actualUnicodeCommentIdentifier;
    scenario.addComment(actualUnicodeCommentIdentifier, s_UnicodeCommentId);

    Kernel::IComment* unicodeComment = scenario.getCommentDetails(s_UnicodeCommentId);
    unicodeComment->setText("This comment contains a newline\nand unicode characters 日本語");


    OVT_ASSERT(
        context->getScenarioManager().exportScenarioToFile((temporaryDirectory + "/" + s_SimpleScenarioFileName).c_str(),
            simpleScenarioIdentifier, OVP_GD_ClassId_Algorithm_XMLScenarioExporter), "Failed to export a simple scenario");

    return urImportScenarioFromFileTest(argc, argv);
}
