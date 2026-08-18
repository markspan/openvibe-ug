#pragma once

#include "CSkeletonGenerator.hpp"
#include <map>
#include <vector>

namespace OpenViBE {
namespace SkeletonGenerator {
class CBoxAlgorithmSkeletonGenerator final : public CSkeletonGenerator
{
public:
    CBoxAlgorithmSkeletonGenerator(Kernel::IKernelContext& ctx, GtkBuilder* builder) : CSkeletonGenerator(ctx, builder) { }
    ~CBoxAlgorithmSkeletonGenerator() override { }

    bool initialize() override;
    bool save(const std::string& filename) override;
    bool load(const std::string& filename) override;
    void getCurrentParameters() override;

    // Box Description
    std::string m_Name, m_Version, m_ClassName, m_Category, m_ShortDesc, m_DetailedDesc;
    std::string m_GtkStockItemName;
    int m_GtkStockItemIdx = 0;

    struct SIOS
    {
        std::string name, type, typeID, defaultValue;
    };

    // Inputs
    bool m_CanModifyInputs = false, m_CanAddInputs = false;
    std::vector<SIOS> m_Inputs;
    // Outputs
    bool m_CanModifyOutputs = false, m_CanAddOutputs = false;
    std::vector<SIOS> m_Outputs;
    // Settings
    bool m_CanModifySettings = false, m_CanAddSettings = false;
    std::vector<SIOS> m_Settings;

    //Algorithms
    std::vector<std::string> m_Algorithms;  // the algorithm selected by user
    // Can be made non-const after '= false' produces working code
    static const bool USE_CODEC_TOOLKIT = true;     // use or not the codec toolkit for encoder and decoder algorithms
    //the map between algorithm and corresponding header declaration (all variables algo/input/output).
    std::map<std::string, std::string> m_AlgoDeclaration;
    //the map between algorithm and corresponding initialisation
    std::map<std::string, std::string> m_AlgoInitialisations;
    //the map between algorithm and corresponding initialisation of ref targets
    std::map<std::string, std::string> m_AlgoInitialisationReferenceTargets;
    //the map between algorithm and corresponding uninitialisation
    std::map<std::string, std::string> m_AlgoUninitialisations;

    // Box Listener
    bool m_UseBoxListener = false;
    // input
    bool m_HasOnInputAdded        = false;
    bool m_HasOnInputRemoved      = false;
    bool m_HasOnInputTypeChanged  = false;
    bool m_HasOnInputNameChanged  = false;
    bool m_HasOnInputConnected    = false;
    bool m_HasOnInputDisconnected = false;
    // output
    bool m_HasOnOutputAdded        = false;
    bool m_HasOnOutputRemoved      = false;
    bool m_HasOnOutputTypeChanged  = false;
    bool m_HasOnOutputNameChanged  = false;
    bool m_HasOnOutputConnected    = false;
    bool m_HasOnOutputDisconnected = false;
    // setting
    bool m_HasOnSettingAdded               = false;
    bool m_HasOnSettingRemoved             = false;
    bool m_HasOnSettingTypeChanged         = false;
    bool m_HasOnSettingNameChanged         = false;
    bool m_HasOnSettingDefaultValueChanged = false;
    bool m_HasOnSettingValueChanged        = false;

    bool m_HasProcessInput    = false;
    bool m_HasProcessClock    = false;
    bool m_HasProcessMessage  = false;
    uint32_t m_ClockFrequency = 0;

    void ButtonCheckCB();
    void ButtonOkCB();
    void ToggleListenerCheckbuttonsStateCB(bool state) const;
    void ButtonTooltipCB(GtkButton* button);
    void ButtonExitCB();

    void ButtonAddInputCB() const;
    void ButtonAddOutputCB() const;
    void ButtonAddSettingCB() const;
    void ButtonAddAlgorithmCB() const;
    void ButtonRemoveGeneric(const std::string& buttonName) const;

    void ButtonAlgorithmSelectedCB(int index) const;
    void SetSensitivity(const std::string& widgetName, bool isActive) const;

private:
    Kernel::ILogManager& getLogManager() const override { return m_kernelCtx.getLogManager(); }
    Kernel::CErrorManager& getErrorManager() const override { return m_kernelCtx.getErrorManager(); }

    static std::string getRandomIdentifierString() { return CIdentifier::random().str(); }

    // Sanity checks that a string is not empty or consist of spaces
    static bool isStringValid(const std::string& str) { return !str.empty() && str.find_first_not_of(' ') != std::string::npos; }

    void clearStore(const std::string& name) const;
    void addDialogButttons(const std::string& name) const;
    void setEntryToConfigValue(const std::string& name, const std::string& token) const;
    void setActiveFromConf(const std::string& name, const std::string& token) const;
    void setActiveFromConfTog(const std::string& name, const std::string& token) const;
    const gchar* getText(const std::string& name) const;
    void addCollectionToTree(const std::string& treeName, const std::vector<std::string>& collection) const;
    void fillCollection(const std::string& treeName, std::vector<SIOS>& collection) const;
    bool isActive(const std::string& name) const;

    std::map<GtkButton*, std::string> m_tooltips;
    std::vector<std::string> m_typeCorrespondances;
};

class CDummyAlgoProto final : public Kernel::IAlgorithmProto
{
public:
    std::map<std::string, Kernel::EParameterType> m_Inputs, m_Outputs;
    std::vector<std::string> m_InputTriggers, m_OutputTriggers;

    bool addInputParameter(const CIdentifier& id, const CString& name, Kernel::EParameterType typeID, const CIdentifier& subTypeID = CIdentifier::undefined()) override;
    bool addOutputParameter(const CIdentifier& id, const CString& name, Kernel::EParameterType typeID, const CIdentifier& subTypeID = CIdentifier::undefined()) override;
    bool addInputTrigger(const CIdentifier& id, const CString& name) override;
    bool addOutputTrigger(const CIdentifier& id, const CString& name) override;

    CIdentifier getClassIdentifier() const override { return CIdentifier::undefined(); }
};
}  // namespace SkeletonGenerator
}  // namespace OpenViBE
