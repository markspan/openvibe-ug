#include <openvibe/ov_all.h>
#include <vector>
#include <tuple>

using namespace OpenViBE;

static const auto s_SimpleScenarioFileName = std::string("test-scenario-simple.mxs");
static const auto s_ClockStimulatorBoxId = CIdentifier(0x66b53b19, 0x043b1afe);
static const auto s_StimulationListenerBoxId = CIdentifier(0x4468da36, 0x3fce3251);
static const auto s_ClockStimulatorToStimulationListenerLinkId = CIdentifier(0x1dd7b2d2, 0x3c1d084d);
static const auto s_SimpleCommentId = CIdentifier(0x24da4aa1, 0x7759a2ee);
static const auto s_UnicodeCommentId = CIdentifier(0x160bd469, 0x62a644ce);

static std::vector<std::tuple<CIdentifier, std::string>> simpleScenarioAttributes = {
    std::make_tuple(OV_AttributeId_Scenario_Name, std::string("SCENARIO NAME")),
    std::make_tuple(OV_AttributeId_Scenario_Author, std::string("AUTHOR NAME")),
    std::make_tuple(OV_AttributeId_Scenario_Company, std::string("COMPANY NAME")),
    std::make_tuple(OV_AttributeId_Scenario_ShortDescription, std::string("SHORT DESCRIPTION")),
    std::make_tuple(OV_AttributeId_Scenario_DetailedDescription, std::string("DETAILED DESCRIPTION")),
    std::make_tuple(OV_AttributeId_Scenario_Category, std::string("SCENARIO CATEGORY")),
    std::make_tuple(OV_AttributeId_Scenario_Version, std::string("SCENARIO VERSION")),
    std::make_tuple(OV_AttributeId_Scenario_DocumentationPage, std::string("DOCUMENTATION PAGE")),
};

static std::vector<std::tuple<CIdentifier, std::string, std::string, std::string>> simpleScenarioSettings = {
    std::make_tuple(OV_TypeId_Integer, "Integer Setting", "10", "100"),
    std::make_tuple(OV_TypeId_Float, "Float Setting", "3.14", "2.42"),
    std::make_tuple(OV_TypeId_String, "String Setting", "Default string value", "Modified string value")
};

static std::vector<std::tuple<CIdentifier, std::string, CIdentifier, size_t>> simpleScenarioInputs = {
    std::make_tuple(OV_TypeId_Stimulations, "Stimulation Input", s_StimulationListenerBoxId, 0),
    std::make_tuple(OV_TypeId_StreamedMatrix, "Disconnected Matrix Input", CIdentifier::undefined(), 0),
};

static std::vector<std::tuple<CIdentifier, std::string, CIdentifier, size_t>> simpleScenarioOutputs = {
    std::make_tuple(OV_TypeId_Stimulations, "Stimulation Output", s_ClockStimulatorBoxId, 0),
    std::make_tuple(OV_TypeId_StreamedMatrix, "Disconnected Matrix Output", CIdentifier::undefined(), 0),
};



