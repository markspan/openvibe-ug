#include <string>
#include <vector>
#include <ovp_global_defines.h>
#include "ovtAssert.h"
#include "ovtTestFixtureCommon.h"
#include "urSimpleTestScenarioDefinition.h"

#define to_cppstring(str) std::string(str.toASCIIString())

int urImportScenarioFromFileTest(int /*argc*/, char* argv[])
{
    Test::ScopedTest<Test::SKernelFixture> fixture(argv[1]);
    const auto& context = fixture->context;

    context->getPluginManager().addPluginsFromFiles(Directories::getLib("plugins-sdk-file-io*"));
    context->getPluginManager().addPluginsFromFiles(Directories::getLib("plugins-sdk-stimulation*"));
    context->getPluginManager().addPluginsFromFiles(Directories::getLib("plugins-sdk-tools*"));

    const std::string scenarioFilePath = std::string(argv[2]) + "/" + s_SimpleScenarioFileName;

    CIdentifier scenarioID;
    OVT_ASSERT(context->getScenarioManager().importScenarioFromFile(scenarioID, scenarioFilePath.c_str(), OVP_GD_ClassId_Algorithm_XMLScenarioImporter),
               "Failed to import the scenario file");
    OVT_ASSERT(scenarioID != CIdentifier::undefined(), "Scenario importer failed to import the scenario but failed to report an error");

    auto& scenario = context->getScenarioManager().getScenario(scenarioID);

    // Test scenario attributes
    for (auto& attribute : simpleScenarioAttributes) {
        OVT_ASSERT_STREQ(to_cppstring(scenario.getAttributeValue(std::get<0>(attribute))), std::get<1>(attribute), "Badly imported scenario attribute");
    }

    // Test settings
    OVT_ASSERT(scenario.getSettingCount() == simpleScenarioSettings.size(), "Imported scenario does not have the correct number of settings");

    for (size_t settingIndex = 0; settingIndex < simpleScenarioSettings.size(); settingIndex += 1) {
        CIdentifier settingTypeId;
        OVT_ASSERT(scenario.getSettingType(settingIndex, settingTypeId), "Cannot get setting type");
        OVT_ASSERT(settingTypeId == std::get<0>(simpleScenarioSettings[settingIndex]), "Setting has wrong type");

        CString settingName;
        OVT_ASSERT(scenario.getSettingName(settingIndex, settingName), "Cannot get setting name");
        OVT_ASSERT_STREQ(to_cppstring(settingName), std::get<1>(simpleScenarioSettings[settingIndex]), "Setting has wrong name");

        CString settingDefaultValue;
        OVT_ASSERT(scenario.getSettingDefaultValue(settingIndex, settingDefaultValue), "Cannot get setting default value");
        OVT_ASSERT_STREQ(to_cppstring(settingDefaultValue), std::get<2>(simpleScenarioSettings[settingIndex]), "Setting has wrong default value");

        CString settingValue;
        OVT_ASSERT(scenario.getSettingValue(settingIndex, settingValue), "Cannot get setting value");
        OVT_ASSERT_STREQ(to_cppstring(settingValue), std::get<3>(simpleScenarioSettings[settingIndex]), "Setting has wrong value");
    }

    // Test boxes
    OVT_ASSERT(scenario.isBox(s_ClockStimulatorBoxId), "Imported scenario does not contain the Clock Stimulator Box");
    OVT_ASSERT(scenario.isBox(s_StimulationListenerBoxId), "Imported scenario does not contain the Stimulation Listener Box");

    const Kernel::IBox* clockStimulatorBox = scenario.getBoxDetails(s_ClockStimulatorBoxId);
    OVT_ASSERT_STREQ(to_cppstring(clockStimulatorBox->getName()), std::string("Clock stimulator"), "Badly imported Clock Stimulator name");

    const Kernel::IBox* stimulationListenerBox = scenario.getBoxDetails(s_StimulationListenerBoxId);
    OVT_ASSERT_STREQ(to_cppstring(stimulationListenerBox->getName()), std::string("Stimulation listener"), "Badly imported Stimulation Listener name");

    // Test inputs
    OVT_ASSERT(scenario.getInputCount() == simpleScenarioInputs.size(), "Imported scenario has wrong number of inputs");

    for (size_t index = 0; index < simpleScenarioInputs.size(); index += 1) {
        CIdentifier inputTypeId;
        OVT_ASSERT(scenario.getInputType(index, inputTypeId), "Cannot get input type");
        OVT_ASSERT(inputTypeId == std::get<0>(simpleScenarioInputs[index]), "Input has wrong type");

        CString inputName;
        OVT_ASSERT(scenario.getInputName(index, inputName), "Cannot get input name");
        OVT_ASSERT_STREQ(to_cppstring(inputName), std::get<1>(simpleScenarioInputs[index]), "Input has wrong name");

        CIdentifier dstBoxID;
        size_t dstBoxInputIdx;
        CIdentifier dstBoxInputID = CIdentifier::undefined();
        OVT_ASSERT(scenario.getScenarioInputLink(index, dstBoxID, dstBoxInputIdx), "Cannot get scenario input details by index");
        OVT_ASSERT(scenario.getScenarioInputLink(index, dstBoxID, dstBoxInputID), "Cannot get scenario input details by identifier");
        OVT_ASSERT(dstBoxID == std::get<2>(simpleScenarioInputs[index]), "Scenario input is not connected to the correct box");
        OVT_ASSERT(dstBoxInputIdx == std::get<3>(simpleScenarioInputs[index]), "Scenario input is not connected to the correct box input");
    }

    // Test outputs
    OVT_ASSERT(scenario.getOutputCount() == simpleScenarioOutputs.size(), "Imported scenario has wrong number of outputs");

    for (size_t idx = 0; idx < simpleScenarioOutputs.size(); idx += 1) {
        CIdentifier outputTypeId;
        OVT_ASSERT(scenario.getOutputType(idx, outputTypeId), "Cannot get output type");
        OVT_ASSERT(outputTypeId == std::get<0>(simpleScenarioOutputs[idx]), "Output has wrong type");

        CString outputName;
        OVT_ASSERT(scenario.getOutputName(idx, outputName), "Cannot get output name");
        OVT_ASSERT_STREQ(to_cppstring(outputName), std::get<1>(simpleScenarioOutputs[idx]), "Output has wrong name");

        CIdentifier dstBoxID;
        size_t dstBoxOutputIndex;
        CIdentifier dstBoxOutputIdentifier = CIdentifier::undefined();
        OVT_ASSERT(scenario.getScenarioOutputLink(idx, dstBoxID, dstBoxOutputIndex), "Cannot get scenario output details by index");
        OVT_ASSERT(scenario.getScenarioOutputLink(idx, dstBoxID, dstBoxOutputIdentifier), "Cannot get scenario output details by  identifier");
        OVT_ASSERT(dstBoxID == std::get<2>(simpleScenarioOutputs[idx]), "Scenario output is not connected to the correct box");
        OVT_ASSERT(dstBoxOutputIndex == std::get<3>(simpleScenarioOutputs[idx]), "Scenario output is not connected to the correct box output");
    }

    // Test links
    OVT_ASSERT(scenario.isLink(s_ClockStimulatorToStimulationListenerLinkId),
               "Imported scenario does not contain a link between the Clock Stimulator and Stimulation Listener boxes");

    const Kernel::ILink* clockStimulatorToStimulationListenerLink = scenario.getLinkDetails(s_ClockStimulatorToStimulationListenerLinkId);

    CIdentifier linkSourceBoxId;
    size_t linkSourceOutputIndex;
    CIdentifier linkSourceOutputIdentifier;
    OVT_ASSERT(clockStimulatorToStimulationListenerLink->getSource(linkSourceBoxId, linkSourceOutputIndex, linkSourceOutputIdentifier),
               "Could not get link details");
    OVT_ASSERT(linkSourceBoxId == s_ClockStimulatorBoxId, "The Clock Stimulator to Stimulation Listener link does not have the Clock Stimulator as the source");
    OVT_ASSERT(linkSourceOutputIndex == 0,
               "The Clock Stimulator to Stimulation Listener link does not have the first output of Clock Stimulator as the output");

    CIdentifier linkTargetBoxId;
    size_t linkTargetInputIndex;
    CIdentifier linkTargetInputIdentifier;
    OVT_ASSERT(clockStimulatorToStimulationListenerLink->getTarget(linkTargetBoxId, linkTargetInputIndex, linkTargetInputIdentifier),
               "Could not get link details");
    OVT_ASSERT(linkTargetBoxId == s_StimulationListenerBoxId,
               "The Clock Stimulator to Stimulation Listener link does not have the Stimulation Listener as the target");
    OVT_ASSERT(linkTargetInputIndex == 1,
               "The Clock Stimulator to Stimulation Listener link does not have the second input of Stimulation Listener as the input");

    // Test comments

    OVT_ASSERT(scenario.isComment(s_SimpleCommentId), "Imported scenario does not contain the simple comment");
    OVT_ASSERT(scenario.isComment(s_UnicodeCommentId), "Imported scenario does not contain the comment containing unicode");

    const Kernel::IComment* simpleComment = scenario.getCommentDetails(s_SimpleCommentId);

    const CString simpleCommentText = simpleComment->getText();
    OVT_ASSERT_STREQ(std::string("Content of a comment"), to_cppstring(simpleCommentText), "The imported scenario comment contains a wrong text");

    const Kernel::IComment* unicodeComment = scenario.getCommentDetails(s_UnicodeCommentId);

    const CString unicodeCommentText = unicodeComment->getText();
    OVT_ASSERT_STREQ(std::string("This comment contains a newline\nand unicode characters 日本語"), to_cppstring(unicodeCommentText),
                     "The imported scenario comment contains a wrong text");

    return EXIT_SUCCESS;
}
