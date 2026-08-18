#include "ovtkCAlgorithmScenarioImporter.h"

#include <vector>
#include <map>
#include <iostream>
#include <algorithm>

namespace OpenViBE {
namespace Toolkit {

namespace {
typedef struct SScenarioInput
{
    CIdentifier id     = CIdentifier::undefined();
    CIdentifier typeID = CIdentifier::undefined();
    CString name;
    CIdentifier linkedBoxID      = CIdentifier::undefined();
    size_t linkedBoxInputIdx     = size_t(-1);
    CIdentifier linkedBoxInputID = CIdentifier::undefined();
} scenario_input_t;

typedef struct SScenarioOutput
{
    CIdentifier id     = CIdentifier::undefined();
    CIdentifier typeID = CIdentifier::undefined();
    CString name;
    CIdentifier linkedBoxID       = CIdentifier::undefined();
    size_t linkedBoxOutputIdx     = size_t(-1);
    CIdentifier linkedBoxOutputID = CIdentifier::undefined();
} scenario_output_t;

typedef struct SInput
{
    CIdentifier id     = CIdentifier::undefined();
    CIdentifier typeID = CIdentifier::undefined();
    CString name;
} input_t;

typedef struct SOutput
{
    CIdentifier id     = CIdentifier::undefined();
    CIdentifier typeID = CIdentifier::undefined();
    CString name;
} output_t;

typedef struct SSettingConnection
{
    CIdentifier boxID = CIdentifier::undefined();
    CString boxParamName;
} setting_connection_t;

typedef struct SSetting
{
    CIdentifier typeID = CIdentifier::undefined();
    CString name;
    CString defaultValue;
    CString value;
    bool modifiability = false;
    CIdentifier id     = CIdentifier::undefined();
    std::vector<setting_connection_t> connections;
} setting_t;

typedef struct SAttribute
{
    CIdentifier id = CIdentifier::undefined();
    CString value;
} attribute_t;

typedef struct SBox
{
    CIdentifier id               = CIdentifier::undefined();
    CIdentifier algorithmClassID = CIdentifier::undefined();
    CString name;
    std::vector<input_t> inputs;
    std::vector<output_t> outputs;
    std::vector<setting_t> settings;
    std::vector<attribute_t> attributes;
} box_t;

typedef struct SComment
{
    CIdentifier id;
    CString text;
    std::vector<attribute_t> attributes;
} comment_t;

typedef struct SMetadata
{
    CIdentifier identifier;
    CIdentifier type;
    CString data;
} metadata_t;

typedef struct SLinkSrc
{
    CIdentifier boxID;
    size_t boxOutputIdx     = size_t(-1);
    CIdentifier boxOutputID = CIdentifier::undefined();
} link_src_t;

typedef struct SLinkDst
{
    CIdentifier boxID;
    size_t boxInputIdx     = size_t(-1);
    CIdentifier boxInputID = CIdentifier::undefined();
} link_dst_t;

typedef struct SLink
{
    CIdentifier id;
    link_src_t linkSrc;
    link_dst_t linkDst;
    std::vector<attribute_t> attributes;
} link_t;

typedef struct SScenario
{
    std::vector<setting_t> settings;
    std::vector<scenario_input_t> iScenarios;
    std::vector<scenario_output_t> oScenarios;
    std::vector<box_t> boxes;
    std::vector<comment_t> comments;
    std::vector<metadata_t> metadata;
    std::vector<link_t> links;
    std::vector<attribute_t> attributes;
} scenario_t;
}  // namespace

class CAlgorithmScenarioImporterContext final : public Plugins::IAlgorithmScenarioImporterContext
{
public:

    explicit CAlgorithmScenarioImporterContext(Kernel::IAlgorithmContext& algorithmCtx) : m_AlgorithmContext(algorithmCtx) { }

    bool processStart(const CIdentifier& identifier) override;
    bool processIdentifier(const CIdentifier& identifier, const CIdentifier& value) override;
    bool processString(const CIdentifier& identifier, const CString& value) override;
    bool processUInteger(const CIdentifier& identifier, uint64_t value) override;
    bool processStop() override { return true; }

    _IsDerivedFromClass_Final_(IAlgorithmScenarioImporterContext, CIdentifier::undefined())

    Kernel::IAlgorithmContext& m_AlgorithmContext;
    scenario_t m_SymbolicScenario;
};


bool CAlgorithmScenarioImporter::process()
{
    Kernel::TParameterHandler<Kernel::IScenario*> op_scenario(this->getOutputParameter(OV_Algorithm_ScenarioImporter_OutputParameterId_Scenario));
    Kernel::IScenario* scenario = op_scenario;

    OV_ERROR_UNLESS_KRF(scenario, "Output scenario is NULL", Kernel::ErrorType::BadOutput);

    Kernel::TParameterHandler<CMemoryBuffer*> ip_buffer(this->getInputParameter(OV_Algorithm_ScenarioImporter_InputParameterId_MemoryBuffer));
    CMemoryBuffer* memoryBuffer = ip_buffer;

    OV_ERROR_UNLESS_KRF(memoryBuffer, "Input memory buffer is NULL", Kernel::ErrorType::BadInput);

    std::map<CIdentifier, CIdentifier> boxIdMapping;

    CAlgorithmScenarioImporterContext context(this->getAlgorithmContext());

    OV_ERROR_UNLESS_KRF(this->import(context, *memoryBuffer), "Import failed", Kernel::ErrorType::Internal);

    scenario_t& symbolicScenario = context.m_SymbolicScenario;

    // Now build the scenario according to what has been loaded

    for (auto b = symbolicScenario.boxes.begin(); b != symbolicScenario.boxes.end(); ++b)
    {
        Kernel::IBox* box = nullptr;
        CIdentifier newBoxID;

        scenario->addBox(newBoxID, b->id);
        box = scenario->getBoxDetails(newBoxID);
        if (box)
        {
            box->setName(b->name);

            for (auto i = b->inputs.begin(); i != b->inputs.end(); ++i) { box->addInput(i->name, i->typeID, i->id); }

            for (auto o = b->outputs.begin(); o != b->outputs.end(); ++o) { box->addOutput(o->name, o->typeID, o->id); }
            for (auto s = b->settings.begin(); s != b->settings.end(); ++s)
            {
                // Todo: check settings here for $var{Setting Name} and link them to the scenario settings
                const CIdentifier& type = s->typeID;
                if (!this->getTypeManager().isRegistered(type) && !(this->getTypeManager().isEnumeration(type)) && (!this->getTypeManager().isBitMask(type)))
                {
                    const std::string msg = std::string("The type of the setting ") + s->name.toASCIIString() + " (" + type.str() + ") from box "
                                            + b->name.toASCIIString() + " cannot be recognized.";

                    if (this->getConfigurationManager().expandAsBoolean("${Kernel_AbortScenarioImportOnUnknownSetting}", true))
                    {
                        OV_ERROR_KRF(msg, Kernel::ErrorType::BadSetting);
                    }
                    OV_WARNING_K(msg);
                }
                box->addSetting(s->name, s->typeID, s->defaultValue, size_t(-1), s->modifiability, s->id);
                box->setSettingValue(box->getSettingCount() - 1, s->value);
            }
            for (auto a = b->attributes.begin(); a != b->attributes.end(); ++a) { box->addAttribute(a->id, a->value); }

            // it is important to set box algorithm at
            // last so the box listener is never called
            box->setAlgorithmClassIdentifier(b->algorithmClassID);
        }
        boxIdMapping[b->id] = newBoxID;
    }

    // Settings need processing after boxes for parameter connections to be applied
    for (auto s = symbolicScenario.settings.begin(); s != symbolicScenario.settings.end(); ++s)
    {
        CIdentifier settingID = s->id;
        // compute identifier only if it does not exists
        if (settingID == CIdentifier::undefined()) { settingID = scenario->getUnusedSettingIdentifier(); }
        scenario->addSetting(s->name, s->typeID, s->defaultValue, size_t(-1), false, settingID);
        scenario->setSettingValue(scenario->getSettingCount() - 1, s->value);
        for (auto connection = s->connections.begin(); connection != s->connections.end(); ++connection)
        {
            CIdentifier boxID = connection->boxID;
            std::string boxParamName = connection->boxParamName.toASCIIString();
            scenario->addParameterConnection(s->name.toASCIIString(), boxID, boxParamName);
        }
    }

    for (auto c = symbolicScenario.comments.begin(); c != symbolicScenario.comments.end(); ++c)
    {
        Kernel::IComment* comment = nullptr;
        CIdentifier newCommentID;

        scenario->addComment(newCommentID, c->id);
        comment = scenario->getCommentDetails(newCommentID);
        if (comment)
        {
            comment->setText(c->text);

            for (auto a = c->attributes.begin(); a != c->attributes.end(); ++a) { comment->addAttribute(a->id, a->value); }
        }
    }

    for (auto& symbolicMetadata : symbolicScenario.metadata)
    {
        CIdentifier newMetadataIdentifier;
        scenario->addMetadata(newMetadataIdentifier, symbolicMetadata.identifier);
        Kernel::IMetadata* metadata = scenario->getMetadataDetails(newMetadataIdentifier);
        if (metadata)
        {
            metadata->setType(symbolicMetadata.type);
            metadata->setData(symbolicMetadata.data);
        }
    }

    for (auto l = symbolicScenario.links.begin(); l != symbolicScenario.links.end(); ++l)
    {
        Kernel::ILink* link = nullptr;
        CIdentifier newLinkID;

        size_t srcBoxOutputIdx = l->linkSrc.boxOutputIdx;
        size_t dstBoxInputIdx  = l->linkDst.boxInputIdx;

        CIdentifier srcBoxOutputID = l->linkSrc.boxOutputID;
        CIdentifier dstBoxInputID  = l->linkDst.boxInputID;

        if (srcBoxOutputID != CIdentifier::undefined()) { scenario->getSourceBoxOutputIndex(boxIdMapping[l->linkSrc.boxID], srcBoxOutputID, srcBoxOutputIdx); }

        OV_ERROR_UNLESS_KRF(srcBoxOutputIdx != size_t(-1), "Output index of the source box could not be found", Kernel::ErrorType::BadOutput);

        if (dstBoxInputID != CIdentifier::undefined()) { scenario->getTargetBoxInputIndex(boxIdMapping[l->linkDst.boxID], dstBoxInputID, dstBoxInputIdx); }

        OV_ERROR_UNLESS_KRF(dstBoxInputIdx != size_t(-1), "Input index of the target box could not be found", Kernel::ErrorType::BadOutput);

        scenario->connect(newLinkID, boxIdMapping[l->linkSrc.boxID], srcBoxOutputIdx, boxIdMapping[l->linkDst.boxID], dstBoxInputIdx, l->id);

        link = scenario->getLinkDetails(newLinkID);
        if (link) { for (auto a = l->attributes.begin(); a != l->attributes.end(); ++a) { link->addAttribute(a->id, a->value); } }
    }

    size_t scenarioInputIdx = 0;
    for (auto symbolicScenarioInput : symbolicScenario.iScenarios)
    {
        CIdentifier scenarioInputID = symbolicScenarioInput.id;
        // compute identifier only if it does not exists
        if (scenarioInputID == CIdentifier::undefined()) { scenarioInputID = scenario->getUnusedInputIdentifier(); }
        scenario->addInput(symbolicScenarioInput.name, symbolicScenarioInput.typeID, scenarioInputID);
        if (symbolicScenarioInput.linkedBoxID != CIdentifier::undefined())
        {
            // Only try to set scenario output links from boxes that actually exist
            // This enables the usage of header-only importers
            if (symbolicScenario.boxes.end() != std::find_if(symbolicScenario.boxes.begin(), symbolicScenario.boxes.end(),
                                                             [&symbolicScenarioInput](const box_t& box) { return box.id == symbolicScenarioInput.linkedBoxID; })
            )
            {
                CIdentifier linkedBoxInputIdentifier = symbolicScenarioInput.linkedBoxInputID;
                size_t linkedBoxInputIndex           = symbolicScenarioInput.linkedBoxInputIdx;

                if (linkedBoxInputIdentifier != CIdentifier::undefined())
                {
                    scenario->getTargetBoxInputIndex(symbolicScenarioInput.linkedBoxID, linkedBoxInputIdentifier, linkedBoxInputIndex);
                }

                OV_ERROR_UNLESS_KRF(linkedBoxInputIndex != size_t(-1), "Input index of the target box could not be found",
                                    Kernel::ErrorType::BadOutput);

                scenario->setScenarioInputLink(scenarioInputIdx, symbolicScenarioInput.linkedBoxID, linkedBoxInputIndex);
            }
        }
        scenarioInputIdx++;
    }

    size_t scenarioOutputIdx = 0;
    for (auto symbolicScenarioOutput : symbolicScenario.oScenarios)
    {
        CIdentifier scenarioOutputID = symbolicScenarioOutput.id;
        // compute identifier only if it does not exists
        if (scenarioOutputID == CIdentifier::undefined()) { scenarioOutputID = scenario->getUnusedOutputIdentifier(); }
        scenario->addOutput(symbolicScenarioOutput.name, symbolicScenarioOutput.typeID, scenarioOutputID);
        if (symbolicScenarioOutput.linkedBoxID != CIdentifier::undefined())
        {
            // Only try to set scenario output links from boxes that actually exist
            // This enables the usage of header-only importers
            if (std::any_of(symbolicScenario.boxes.begin(), symbolicScenario.boxes.end(), [&symbolicScenarioOutput](const box_t& box)
            {
                return box.id == symbolicScenarioOutput.linkedBoxID;
            }))
            {
                CIdentifier linkedBoxOutputIdentifier = symbolicScenarioOutput.linkedBoxOutputID;
                size_t linkedBoxOutputIndex           = symbolicScenarioOutput.linkedBoxOutputIdx;

                if (linkedBoxOutputIdentifier != CIdentifier::undefined())
                {
                    scenario->getSourceBoxOutputIndex(symbolicScenarioOutput.linkedBoxID, linkedBoxOutputIdentifier, linkedBoxOutputIndex);
                }

                OV_ERROR_UNLESS_KRF(linkedBoxOutputIndex != size_t(-1), "Output index of the target box could not be found",
                                    Kernel::ErrorType::BadOutput);
                scenario->setScenarioOutputLink(scenarioOutputIdx, symbolicScenarioOutput.linkedBoxID, linkedBoxOutputIndex);
            }
        }
        scenarioOutputIdx++;
    }

    for (auto a = symbolicScenario.attributes.begin(); a != symbolicScenario.attributes.end(); ++a) { scenario->addAttribute(a->id, a->value); }

    if (scenario->checkOutdatedBoxes())
    {
        CIdentifier* listID = nullptr;
        size_t nbElems      = 0;
        scenario->getOutdatedBoxIdentifierList(&listID, &nbElems);
        for (size_t i = 0; i < nbElems; ++i)
        {
            const Kernel::IBox* box = scenario->getBoxDetails(listID[i]);
            OV_WARNING_K(std::string("Box ") + box->getName().toASCIIString() + " [" + box->getAlgorithmClassIdentifier().str() + "] should be updated. (from CAlgorithmScenarioImporter)");
        }
        scenario->releaseIdentifierList(listID);
    }

    return true;
}

bool CAlgorithmScenarioImporterContext::processStart(const CIdentifier& identifier)
{
    if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_OpenViBEScenario) { }
    else if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_Scenario_Settings) { }
    else if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_Scenario_Setting) { m_SymbolicScenario.settings.push_back(setting_t()); }
    else if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_Scenario_Setting_Connections) { }
    else if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_Scenario_Setting_Connection) { m_SymbolicScenario.settings.back().connections.push_back(setting_connection_t()); }
    else if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_Scenario_Inputs) { }
    else if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_Scenario_Input) { m_SymbolicScenario.iScenarios.push_back(scenario_input_t()); }
    else if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_Scenario_Outputs) { }
    else if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_Scenario_Output) { m_SymbolicScenario.oScenarios.push_back(scenario_output_t()); }
    else if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_Creator) { }
    else if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_CreatorVersion) { }
    else if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_Scenario_Attributes) { }
    else if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_Scenario_Attribute) { m_SymbolicScenario.attributes.push_back(attribute_t()); }
    else if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_Box_Attributes) { }
    else if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_Box_Attribute) { m_SymbolicScenario.boxes.back().attributes.push_back(attribute_t()); }
    else if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_Link_Attributes) { }
    else if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_Link_Attribute) { m_SymbolicScenario.links.back().attributes.push_back(attribute_t()); }
    else if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_Boxes) { }
    else if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_Box) { m_SymbolicScenario.boxes.push_back(box_t()); }
    else if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_Comments) { }
    else if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_Comment) { m_SymbolicScenario.comments.push_back(comment_t()); }
    else if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_Comment_Attributes) { }
    else if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_Comment_Attribute) { m_SymbolicScenario.comments.back().attributes.push_back(attribute_t()); }
    else if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_Metadata) { }
    else if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_MetadataEntry) { m_SymbolicScenario.metadata.push_back(metadata_t()); }
    else if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_Links) { }
    else if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_Link) { m_SymbolicScenario.links.push_back(link_t()); }
    else if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_Link_Source) { }
    else if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_Link_Target) { }
    else if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_Box_Inputs) { }
    else if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_Box_Input) { m_SymbolicScenario.boxes.back().inputs.push_back(input_t()); }
    else if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_Box_Outputs) { }
    else if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_Box_Output) { m_SymbolicScenario.boxes.back().outputs.push_back(output_t()); }
    else if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_Box_Settings) { }
    else if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_Box_Setting) { m_SymbolicScenario.boxes.back().settings.push_back(setting_t()); }
        //
    else
    {
        OV_ERROR("(start) Unexpected node identifier " << identifier.str(), Kernel::ErrorType::BadArgument, false,
                 m_AlgorithmContext.getErrorManager(), m_AlgorithmContext.getLogManager());
    }
    return true;
}

bool CAlgorithmScenarioImporterContext::processIdentifier(const CIdentifier& identifier, const CIdentifier& value)
{
    if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_Scenario_Setting_ID) { m_SymbolicScenario.settings.back().id = value; }
    else if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_Scenario_Setting_TypeID) { m_SymbolicScenario.settings.back().typeID = value; }
    else if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_Scenario_Setting_Connection_BoxID) { m_SymbolicScenario.settings.back().connections.back().boxID = value; }

    else if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_Scenario_Input_ID) { m_SymbolicScenario.iScenarios.back().id = value; }
    else if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_Scenario_Input_TypeID) { m_SymbolicScenario.iScenarios.back().typeID = value; }
    else if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_Scenario_Input_LinkedBoxID) { m_SymbolicScenario.iScenarios.back().linkedBoxID = value; }
    else if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_Scenario_Input_LinkedBoxInputID)
    {
        m_SymbolicScenario.iScenarios.back().linkedBoxInputID = value;
    }
    else if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_Scenario_Output_ID) { m_SymbolicScenario.oScenarios.back().id = value; }
    else if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_Scenario_Output_TypeID) { m_SymbolicScenario.oScenarios.back().typeID = value; }
    else if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_Scenario_Output_LinkedBoxID) { m_SymbolicScenario.oScenarios.back().linkedBoxID = value; }
    else if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_Scenario_Output_LinkedBoxOutputID)
    {
        m_SymbolicScenario.oScenarios.back().linkedBoxOutputID = value;
    }

    else if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_Box_Attribute_ID) { m_SymbolicScenario.boxes.back().attributes.back().id = value; }
    else if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_Box_ID) { m_SymbolicScenario.boxes.back().id = value; }
    else if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_Box_AlgorithmClassIdD) { m_SymbolicScenario.boxes.back().algorithmClassID = value; }
    else if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_Box_Input_ID) { m_SymbolicScenario.boxes.back().inputs.back().id = value; }
    else if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_Box_Input_TypeID) { m_SymbolicScenario.boxes.back().inputs.back().typeID = value; }
    else if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_Box_Output_ID) { m_SymbolicScenario.boxes.back().outputs.back().id = value; }
    else if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_Box_Output_TypeID) { m_SymbolicScenario.boxes.back().outputs.back().typeID = value; }
    else if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_Box_Setting_ID) { m_SymbolicScenario.boxes.back().settings.back().id = value; }
    else if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_Box_Setting_TypeID) { m_SymbolicScenario.boxes.back().settings.back().typeID = value; }
    else if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_Comment_ID) { m_SymbolicScenario.comments.back().id = value; }
    else if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_Comment_Attribute_ID) { m_SymbolicScenario.comments.back().attributes.back().id = value; }
    else if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_MetadataEntry_ID) { m_SymbolicScenario.metadata.back().identifier = value; }
    else if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_MetadataEntry_Type) { m_SymbolicScenario.metadata.back().type = value; }
    else if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_Link_Attribute_ID) { m_SymbolicScenario.links.back().attributes.back().id = value; }
    else if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_Link_ID) { m_SymbolicScenario.links.back().id = value; }
    else if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_Link_Source_BoxID) { m_SymbolicScenario.links.back().linkSrc.boxID = value; }
    else if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_Link_Source_BoxOutputID) { m_SymbolicScenario.links.back().linkSrc.boxOutputID = value; }
    else if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_Link_Target_BoxID) { m_SymbolicScenario.links.back().linkDst.boxID = value; }
    else if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_Link_Target_BoxInputID) { m_SymbolicScenario.links.back().linkDst.boxInputID = value; }


    else if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_Scenario_Attribute_ID) { m_SymbolicScenario.attributes.back().id = value; }
    else
    {
        OV_ERROR("(id) Unexpected node identifier " << identifier.str(),
                 Kernel::ErrorType::BadArgument, false, m_AlgorithmContext.getErrorManager(), m_AlgorithmContext.getLogManager());
    }
    return true;
}

bool CAlgorithmScenarioImporterContext::processString(const CIdentifier& identifier, const CString& value)
{
    if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_Box_Attribute_Value) { m_SymbolicScenario.boxes.back().attributes.back().value = value; }
    else if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_Box_Name) { m_SymbolicScenario.boxes.back().name = value; }
    else if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_Box_Input_Name) { m_SymbolicScenario.boxes.back().inputs.back().name = value; }
    else if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_Box_Output_Name) { m_SymbolicScenario.boxes.back().outputs.back().name = value; }
    else if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_Box_Setting_Name) { m_SymbolicScenario.boxes.back().settings.back().name = value; }
    else if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_Box_Setting_DefaultValue)
    {
        m_SymbolicScenario.boxes.back().settings.back().defaultValue = value;
    }
    else if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_Box_Setting_Value) { m_SymbolicScenario.boxes.back().settings.back().value = value; }
    else if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_Box_Setting_Modifiability)
    {
        m_SymbolicScenario.boxes.back().settings.back().modifiability = (value == CString("true")) ? true : false;
    }
    else if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_Comment_Text) { m_SymbolicScenario.comments.back().text = value; }
    else if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_Comment_Attribute_Value)
    {
        m_SymbolicScenario.comments.back().attributes.back().value = value;
    }
    else if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_MetadataEntry_Data) { m_SymbolicScenario.metadata.back().data = value; }
    else if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_Link_Attribute_Value) { m_SymbolicScenario.links.back().attributes.back().value = value; }

    else if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_Scenario_Attribute_Value) { m_SymbolicScenario.attributes.back().value = value; }

    else if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_Scenario_Setting_Name) { m_SymbolicScenario.settings.back().name = value; }
    else if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_Scenario_Setting_DefaultValue) { m_SymbolicScenario.settings.back().defaultValue = value; }
    else if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_Scenario_Setting_Value) { m_SymbolicScenario.settings.back().value = value; }
    else if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_Scenario_Setting_Connection_ParamName) { m_SymbolicScenario.settings.back().connections.back().boxParamName = value; }

    else if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_Scenario_Input_Name) { m_SymbolicScenario.iScenarios.back().name = value; }
    else if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_Scenario_Output_Name) { m_SymbolicScenario.oScenarios.back().name = value; }

    else
    {
        OV_ERROR("(string) Unexpected node identifier " << identifier.str(), Kernel::ErrorType::BadArgument,
                 false, m_AlgorithmContext.getErrorManager(), m_AlgorithmContext.getLogManager());
    }
    return true;
}

bool CAlgorithmScenarioImporterContext::processUInteger(const CIdentifier& identifier, const uint64_t value)
{
    if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_Link_Source_BoxOutputIdx) { m_SymbolicScenario.links.back().linkSrc.boxOutputIdx = size_t(value); }
    else if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_Link_Target_BoxInputIdx)
    {
        m_SymbolicScenario.links.back().linkDst.boxInputIdx = size_t(value);
    }

    else if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_Scenario_Input_LinkedBoxInputIdx)
    {
        m_SymbolicScenario.iScenarios.back().linkedBoxInputIdx = size_t(value);
    }
    else if (identifier == OVTK_Algorithm_ScenarioExporter_NodeId_Scenario_Output_LinkedBoxOutputIdx)
    {
        m_SymbolicScenario.oScenarios.back().linkedBoxOutputIdx = size_t(value);
    }

    else
    {
        OV_ERROR("(uint) Unexpected node identifier " << identifier.str(), Kernel::ErrorType::BadArgument, false,
                 m_AlgorithmContext.getErrorManager(), m_AlgorithmContext.getLogManager());
    }
    return true;
}

}  // namespace Toolkit
}  // namespace OpenViBE
