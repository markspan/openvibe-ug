#pragma once

#include "../ovp_defines.h"

#include <toolkit/ovtk_all.h>

namespace OpenViBE {
namespace Plugins {
namespace Examples {
class CLog final : public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }
    uint64_t getClockFrequency() override { return 1LL << 32; }
    bool initialize() override;
    bool uninitialize() override;
    bool processClock(Kernel::CMessageClock& msg) override;
    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(IBoxAlgorithm, OVP_ClassId_Log)
};

class CLogListener final : public Toolkit::TBoxListener<IBoxListener>
{
public:
    CLogListener() {}

    bool initialize() override
    {
        this->getLogManager() << m_logLevel << "initialize\n";
        return true;
    }

    bool uninitialize() override
    {
        this->getLogManager() << m_logLevel << "uninitialize\n";
        return true;
    }

    bool onInitialized(Kernel::IBox& /*box*/) override
    {
        this->getLogManager() << m_logLevel << "onInitialized\n";
        return true;
    }

    bool onNameChanged(Kernel::IBox& /*box*/) override
    {
        this->getLogManager() << m_logLevel << "onNameChanged\n";
        return true;
    }

    bool onIdentifierChanged(Kernel::IBox& /*box*/) override
    {
        this->getLogManager() << m_logLevel << "onIdentifierChanged\n";
        return true;
    }

    bool onAlgorithmClassIdentifierChanged(Kernel::IBox& /*box*/) override
    {
        this->getLogManager() << m_logLevel << "onAlgorithmClassIdentifierChanged\n";
        return true;
    }

    bool onProcessingUnitChangedon(Kernel::IBox& /*box*/) const
    {
        this->getLogManager() << m_logLevel << "onProcessingUnitChangedon\n";
        return true;
    }

    bool onInputConnected(Kernel::IBox& /*box*/, const size_t /*index*/) override
    {
        this->getLogManager() << m_logLevel << "onInputConnected\n";
        return true;
    }

    bool onInputDisconnected(Kernel::IBox& /*box*/, const size_t /*index*/) override
    {
        this->getLogManager() << m_logLevel << "onInputDisconnected\n";
        return true;
    }

    bool onInputAdded(Kernel::IBox& /*box*/, const size_t /*index*/) override
    {
        this->getLogManager() << m_logLevel << "onInputAdded\n";
        return true;
    }

    bool onInputRemoved(Kernel::IBox& /*box*/, const size_t /*index*/) override
    {
        this->getLogManager() << m_logLevel << "onInputRemoved\n";
        return true;
    }

    bool onInputTypeChanged(Kernel::IBox& /*box*/, const size_t /*index*/) override
    {
        this->getLogManager() << m_logLevel << "onInputTypeChanged\n";
        return true;
    }

    bool onInputNameChanged(Kernel::IBox& /*box*/, const size_t /*index*/) override
    {
        this->getLogManager() << m_logLevel << "onInputNameChanged\n";
        return true;
    }

    bool onOutputConnected(Kernel::IBox& /*box*/, const size_t /*index*/) override
    {
        this->getLogManager() << m_logLevel << "onOutputConnected\n";
        return true;
    }

    bool onOutputDisconnected(Kernel::IBox& /*box*/, const size_t /*index*/) override
    {
        this->getLogManager() << m_logLevel << "onOutputDisconnected\n";
        return true;
    }

    bool onOutputAdded(Kernel::IBox& /*box*/, const size_t /*index*/) override
    {
        this->getLogManager() << m_logLevel << "onOutputAdded\n";
        return true;
    }

    bool onOutputRemoved(Kernel::IBox& /*box*/, const size_t /*index*/) override
    {
        this->getLogManager() << m_logLevel << "onOutputRemoved\n";
        return true;
    }

    bool onOutputTypeChanged(Kernel::IBox& /*box*/, const size_t /*index*/) override
    {
        this->getLogManager() << m_logLevel << "onOutputTypeChanged\n";
        return true;
    }

    bool onOutputNameChanged(Kernel::IBox& /*box*/, const size_t /*index*/) override
    {
        this->getLogManager() << m_logLevel << "onOutputNameChanged\n";
        return true;
    }

    bool onSettingAdded(Kernel::IBox& /*box*/, const size_t /*index*/) override
    {
        this->getLogManager() << m_logLevel << "onSettingAdded\n";
        return true;
    }

    bool onSettingRemoved(Kernel::IBox& /*box*/, const size_t /*index*/) override
    {
        this->getLogManager() << m_logLevel << "onSettingRemoved\n";
        return true;
    }

    bool onSettingTypeChanged(Kernel::IBox& /*box*/, const size_t /*index*/) override
    {
        this->getLogManager() << m_logLevel << "onSettingTypeChanged\n";
        return true;
    }

    bool onSettingNameChanged(Kernel::IBox& /*box*/, const size_t /*index*/) override
    {
        this->getLogManager() << m_logLevel << "onSettingNameChanged\n";
        return true;
    }

    bool onSettingDefaultValueChanged(Kernel::IBox& /*box*/, const size_t /*index*/) override
    {
        this->getLogManager() << m_logLevel << "onSettingDefaultValueChanged\n";
        return true;
    }

    bool onSettingValueChanged(Kernel::IBox& /*box*/, const size_t /*index*/) override
    {
        this->getLogManager() << m_logLevel << "onSettingValueChanged\n";
        return true;
    }

    _IsDerivedFromClass_Final_(Toolkit::TBoxListener<IBoxListener>, CIdentifier::undefined())

protected:
    Kernel::ELogLevel m_logLevel = Kernel::LogLevel_Info;
};

class CLogDesc final : public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Log"; }
    CString getAuthorName() const override { return "Yann Renard"; }
    CString getAuthorCompanyName() const override { return "INRIA/IRISA"; }
    CString getShortDescription() const override { return "Logs stuffs (init, uninit, input, clock, process)"; }

    CString getDetailedDescription() const override
    {
        return
                "This sample box shows how stuffs could be logged in the log manager. Note that the different inputs, outputs and parameters have no effect, they only exist to test the logging when there are modifications.";
    }

    CString getCategory() const override { return "Examples/Basic"; }
    CString getVersion() const override { return "1.0"; }

    CIdentifier getCreatedClass() const override { return OVP_ClassId_Log; }
    IPluginObject* create() override { return new CLog(); }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        // Adds box inputs
        prototype.addInput("Input stream", OV_TypeId_Signal);

        // Adds box outputs
        prototype.addOutput("Output stream", OV_TypeId_Signal);

        // Adds box settings
        prototype.addSetting("Integer setting", OV_TypeId_Integer, "0");
        prototype.addSetting("String setting", OV_TypeId_String, "");

        prototype.addFlag(Kernel::BoxFlag_CanAddInput);
        prototype.addFlag(Kernel::BoxFlag_CanModifyInput);
        prototype.addFlag(Kernel::BoxFlag_CanAddOutput);
        prototype.addFlag(Kernel::BoxFlag_CanModifyOutput);
        prototype.addFlag(Kernel::BoxFlag_CanAddSetting);
        prototype.addFlag(Kernel::BoxFlag_CanModifySetting);

        return true;
    }

    CString getStockItemName() const override { return "gtk-edit"; }

    IBoxListener* createBoxListener() const override { return new CLogListener; }
    void releaseBoxListener(IBoxListener* listener) const override { delete listener; }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, OVP_ClassId_LogDesc)
};
}  // namespace Examples
}  // namespace Plugins
}  // namespace OpenViBE
