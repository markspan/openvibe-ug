#pragma once

#if defined TARGET_HAS_ThirdPartyMatlab

#include "../ovp_defines.h"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>
#include "../ovpCMatlabHelper.h"
#include <map>

namespace OpenViBE {
namespace Plugins {
namespace Matlab {
class CBoxAlgorithmMatlabScripting final : virtual public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }
    uint64_t getClockFrequency() override { return m_clockFrequency << 32; }
    bool initialize() override;
    bool uninitialize() override;
    bool processClock(Kernel::CMessageClock& msg) override;
    bool process() override;
    bool processInput(const size_t index) override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, OVP_ClassId_BoxAlgorithm_MatlabScripting)

private:
    bool openMatlabEngineSafely();
    bool closeMatlabEngineSafely();

    uint64_t m_clockFrequency = 0;

    std::map<size_t, Toolkit::TDecoder<CBoxAlgorithmMatlabScripting>*> m_decoders;
    size_t m_nInputHeaderSent = 0;

    std::map<size_t, Toolkit::TEncoder<CBoxAlgorithmMatlabScripting>*> m_encoders;
    std::map<size_t, bool> m_oHeaderStates;

    void* m_engineHandle = nullptr;
    CString m_boxInstanceVariableName; //must be unique


    CMatlabHelper* m_helper = nullptr;

    CString m_matlabPath;           // On Linux, path of the executable. On Windows, the executable directory.
    CString m_processFunc;
    CString m_initializeFunc;
    CString m_uninitializeFunc;

    char* m_matlabBuffer = nullptr;
    bool m_errorDetected = false;
    bool printOutputBufferWithFormat();
    bool checkFailureRoutine(bool result, const CString& msg);
    static void sanitizePath(CString& pathToModify);

    //void sendOutputHeader(CIdentifier idOutputType);
    //void sendOutputBuffer(CIdentifier idOutputType);
};

class CBoxAlgorithmMatlabScriptingListener final : public Toolkit::TBoxListener<IBoxListener>
{
public:
    bool onSettingTypeChanged(Kernel::IBox& box, const size_t index) override
    {
        //we must have the first STATIC_SETTINGS_COUNT settings
        if (index < 6) {
            this->getLogManager() << Kernel::LogLevel_Warning << "One of the predefined setting has changed type, falling back to default.\n";
            box.setSettingType(index, OV_TypeId_String); // they are all strings
        }

        return true;
    }

    bool onSettingValueChanged(Kernel::IBox& box, const size_t index) override
    {
        //we must have the first STATIC_SETTINGS_COUNT settings

        CString str;
        box.getSettingValue(0, str);
        const uint64_t value = atoi(str.toASCIIString());
        if (index == 0 && (value < 1 || value > 128)) {
            OV_WARNING_K("Clock Frequency must be an integer between 1 and 128 Hz. Falling back to default.");
            box.setSettingValue(index, "64");
        }

        return true;
    }

    _IsDerivedFromClass_Final_(Toolkit::TBoxListener<IBoxListener>, CIdentifier::undefined())
};

class CBoxAlgorithmMatlabScriptingDesc final : virtual public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Matlab Scripting"; }
    CString getAuthorName() const override { return "L. Bonnet"; }
    CString getAuthorCompanyName() const override { return "INRIA"; }
    CString getShortDescription() const override { return "Executes matlab scripts. To be used correctly, you must have Matlab installed."; }

    CString getDetailedDescription() const override
    {
        return
                "User must implement the matlab functions:\n[box_out]=bci_Initialize(box_in)\n[box_out]=bci_Process(box_in)\n[box_out]=bci_Uninitialize(box_in)\nPlease refer to the dedicated documentation <openvibe.inria.fr/tutorial-using-matlab-with-openvibe> for more information.";
    }

    CString getCategory() const override { return "Scripting"; }
    CString getVersion() const override { return "1.0"; }
    CString getStockItemName() const override { return "gtk-execute"; }

    CIdentifier getCreatedClass() const override { return OVP_ClassId_BoxAlgorithm_MatlabScripting; }
    IPluginObject* create() override { return new CBoxAlgorithmMatlabScripting; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addSetting("Box clock frequency in Hz", OV_TypeId_Integer, "64");

#if defined TARGET_OS_Linux
                prototype.addSetting("Matlab executable (path)",      OV_TypeId_Filename, "[ssh user@host] /path/to/matlab");
#elif defined TARGET_OS_Windows
        // On Windows, the executable is not actually used, but its path is parsed from it and added to PATH. 
        // Background: Matlab's engOpen() takes a different argument on Windows and on Linux. On Windows, we need to have Matlab
        // on PATH, in Linux we need to provide a full path of the executable. However, we'd like our example scenarios to give
        // correct instructions to the user on both platforms, hence we use a setting that contains the information for both use-cases.
        prototype.addSetting("Matlab executable (path)", OV_TypeId_Filename, "C:/Program Files (x86)/MATLAB/R2013b/bin/win32/matlab.exe");
#else
#endif
        prototype.addSetting("Matlab working directory", OV_TypeId_String, "${Player_ScenarioDirectory}");
        prototype.addSetting("Initialize function", OV_TypeId_String, "matlab_Initialize");
        prototype.addSetting("Process function", OV_TypeId_String, "matlab_Process");
        prototype.addSetting("Uninitialize function", OV_TypeId_String, "matlab_Uninitialize");

        prototype.addFlag(Kernel::BoxFlag_CanAddInput);
        prototype.addFlag(Kernel::BoxFlag_CanModifyInput);
        prototype.addFlag(Kernel::BoxFlag_CanAddOutput);
        prototype.addFlag(Kernel::BoxFlag_CanModifyOutput);
        prototype.addFlag(Kernel::BoxFlag_CanAddSetting);
        prototype.addFlag(Kernel::BoxFlag_CanModifySetting);

        //prototype.addFlag(OV_AttributeId_Box_FlagIsUnstable);

        return true;
    }

    IBoxListener* createBoxListener() const override { return new CBoxAlgorithmMatlabScriptingListener; }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, OVP_ClassId_BoxAlgorithm_MatlabScriptingDesc)
};
}  // namespace Matlab
}  // namespace Plugins
}  // namespace OpenViBE

#endif // TARGET_HAS_ThirdPartyMatlab
