/**
 * The gMobilab driver was contributed
 * by Lucie Daubigney from Supelec Metz
 */

#pragma once

#if defined TARGET_HAS_ThirdPartyGMobiLabPlusAPI

#include "ovasIDriver.h"
#include "../ovasCHeader.h"

#include "../ovasCSettingsHelper.h"
#include "../ovasCSettingsHelperOperators.h"

#include <string>
#ifdef TARGET_OS_Windows
#include <Windows.h>
#endif

namespace OpenViBE {
namespace AcquisitionServer {
class CDriverGTecGMobiLabPlusPrivate; // fwd declare

/**
 * \class CDriverGTecGMobiLabPlus
 * \author Lucie Daubigney (Supelec Metz)
 */
class CDriverGTecGMobiLabPlus final : public IDriver
{
public:

    explicit CDriverGTecGMobiLabPlus(IDriverContext& ctx);
    ~CDriverGTecGMobiLabPlus() override;
    void release();
    const char* getName() override;

    bool isFlagSet(const EDriverFlag flag) const override { return flag == EDriverFlag::IsUnstable; }

    //configuration
    bool isConfigurable() override;
    bool configure() override;
    //initialisation
    bool initialize(const uint32_t sampleCountPerChannel, IDriverCallback& callback) override;
    bool uninitialize() override;
    const IHeader* getHeader() override;

    //acquisition
    bool start() override;
    bool stop() override;
    bool loop() override;

protected:

    SettingsHelper m_settings;

    //usefull data to communicate with OpenViBE
    IHeader* m_header              = nullptr;
    IDriverCallback* m_callback    = nullptr;
    uint32_t m_nSamplePerSentBlock = 0;//number of sample you want to send in a row
    float* m_sample                = nullptr;//array containing the data to sent to OpenViBE once they had been recovered from the gTec module

    //params
    std::string m_portName;
    bool m_testMode = false;

    // Pointers do gtec-specific data and function pointers
    CDriverGTecGMobiLabPlusPrivate* m_gTec = nullptr;

    // Register the function pointers from the dll. (The dll approach
    // is used with gMobilab to avoid conflicts with the gUSBAmp lib)
    bool registerLibraryFunctions();

#if defined(TARGET_OS_Windows)
    HINSTANCE m_library;
#elif defined(TARGET_OS_Linux)
        void* m_library = nullptr;
#endif
private:

    void allowAnalogInputs(uint32_t index);
};
}  // namespace AcquisitionServer
}  // namespace OpenViBE

#endif // TARGET_HAS_ThirdPartyGMobiLabPlusAPI
