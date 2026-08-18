#pragma once

#include "../defines.hpp"

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>
#include <visualization-toolkit/ovviz_all.h>

#include <vector>

namespace OpenViBE {
namespace Plugins {
namespace Examples {
/**
 * \class CBoxAlgorithmModifiableSettings
 * \author lmahe (Inria)
 * \date Mon Oct 14 16:35:48 2013
 * \brief The class CBoxAlgorithmModifiableSettings describes the box ModifiableSettings.
 *
 */
class CBoxAlgorithmModifiableSettings final : virtual public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }
    bool initialize() override { return true; }
    bool uninitialize() override { return true; }
    bool processClock(Kernel::CMessageClock& msg) override;
    uint64_t getClockFrequency() override { return 0x1ULL << 30; }  // 4Hz

    bool process() override { return true; }
    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_ModifiableSettings)

protected:
    bool updateSettings();

    std::vector<CString> m_settingsValue;
};

/**
 * \class CBoxAlgorithmModifiableSettingsDesc
 * \author lmahe (Inria)
 * \date Mon Oct 14 16:35:48 2013
 * \brief Descriptor of the box ModifiableSettings.
 *
 */
class CBoxAlgorithmModifiableSettingsDesc final : virtual public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Modifiable Settings example"; }
    CString getAuthorName() const override { return "lmahe"; }
    CString getAuthorCompanyName() const override { return "Inria"; }
    CString getShortDescription() const override { return "Settings of this box are modifiable during playback. Values are displayed in log every 5 seconds"; }

    CString getDetailedDescription() const override
    {
        return
                "This box purpose is to test and demonstrate the modifiable settings feature.\n It has a setting of each type and all are modifiable during scenario playback.\n";
    }

    CString getCategory() const override { return "Examples/Basic"; }
    CString getVersion() const override { return "1.0"; }
    CString getStockItemName() const override { return ""; }

    CIdentifier getCreatedClass() const override { return Box_ModifiableSettings; }
    IPluginObject* create() override { return new CBoxAlgorithmModifiableSettings; }

    bool hasFunctionality(const EPluginFunctionality functionality) const override { return functionality == EPluginFunctionality::Visualization; }


    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addSetting("Int", OV_TypeId_Integer, "1", true);
        prototype.addSetting("Float", OV_TypeId_Float, "1.3", true);
        prototype.addSetting("Bool", OV_TypeId_Boolean, "false", true);
        prototype.addSetting("String", OV_TypeId_String, "string", true);
        prototype.addSetting("filename", OV_TypeId_Filename, "somefile.txt", true);
        prototype.addSetting("script", OV_TypeId_Script, "somescript.lua", true);
        prototype.addSetting("color", OV_TypeId_Color, "20,65,90", true);
        prototype.addSetting("colorgradient", OV_TypeId_ColorGradient, "0:0,0,0; 100:60,40,40", true);
        prototype.addSetting("unit", OV_TypeId_MeasurementUnit, "V", true);
        prototype.addSetting("factor", OV_TypeId_Factor, "1e-01", true);

        prototype.addFlag(Box_FlagIsUnstable);

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_ModifiableSettingsDesc)
};
}  // namespace Examples
}  // namespace Plugins
}  // namespace OpenViBE
