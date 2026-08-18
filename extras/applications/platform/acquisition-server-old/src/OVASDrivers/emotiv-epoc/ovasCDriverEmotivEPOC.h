#pragma once

#if defined TARGET_HAS_ThirdPartyEmotivAPI

#include "ovasIDriver.h"
#include "../ovasCHeader.h"

#include "../ovasCSettingsHelper.h"
#include "../ovasCSettingsHelperOperators.h"

#include <openvibe/ov_all.h>

#if defined(TARGET_HAS_ThirdPartyEmotivResearchAPI3x)
#include "IEmoStateDLL.h"
#include "Iedk.h"
#include "IedkErrorCode.h"
#include "IEegData.h"
#else
// Old API
#include "EmoStateDLL.h"
#include "edk.h"
#include "edkErrorCode.h"

#define IEE_DataAcquisitionEnable                   EE_DataAcquisitionEnable
#define IEE_DataCreate                              EE_DataCreate
#define IEE_DataFree                                EE_DataFree
#define IEE_DataGet                                 EE_DataGet
#define IEE_DataGetNumberOfSample                   EE_DataGetNumberOfSample
#define IEE_DataSetBufferSizeInSec                  EE_DataSetBufferSizeInSec
#define IEE_DataUpdateHandle                        EE_DataUpdateHandle
#define IEE_EmoEngineEventCreate                    EE_EmoEngineEventCreate
#define IEE_EmoEngineEventFree                      EE_EmoEngineEventFree
#define IEE_EmoEngineEventGetType                   EE_EmoEngineEventGetType
#define IEE_EmoEngineEventGetUserId                 EE_EmoEngineEventGetUserId
#define IEE_EngineConnect                           EE_EngineConnect
#define IEE_EngineDisconnect                        EE_EngineDisconnect
#define IEE_EngineGetNextEvent                      EE_EngineGetNextEvent
#define IEE_Event_t                                 EE_Event_t
#define IEE_UserAdded                               EE_UserAdded
#endif

#include <vector>

#if defined TARGET_OS_Windows
#include <windows.h>
#endif

namespace OpenViBE {
namespace AcquisitionServer {
/**
 * \class CDriverEmotivEPOC
 * \author Laurent Bonnet (INRIA)
 * \date 21 july 2010
 * \erief The CDriverEmotivEPOC allows the acquisition server to acquire data from a Emotiv EPOC amplifier, Research Edition or above.
 *
 */
class CDriverEmotivEPOC final : public IDriver
{
public:

    explicit CDriverEmotivEPOC(IDriverContext& ctx);
    ~CDriverEmotivEPOC() override;
    const char* getName() override;

    bool initialize(const uint32_t nSamplePerSentBlock, IDriverCallback& callback) override;
    bool uninitialize() override;

    bool start() override;
    bool stop() override;
    bool loop() override;

    bool isConfigurable() override { return true; }
    bool configure() override;
    const IHeader* getHeader() override { return &m_header; }

protected:

    SettingsHelper m_settings;

    IDriverCallback* m_callback = nullptr;

    CHeader m_header;

    uint32_t m_nSamplePerSentBlock = 0;
    uint32_t m_nTotalSample        = 0;
    float* m_sample                = nullptr;
    //double* m_Buffer;

private:
    bool buildPath();
    bool restoreState();

    uint32_t m_lastErrorCode = 0;

    EmoEngineEventHandle m_eventHandle;
    uint32_t m_userID     = 0;
    bool m_readyToCollect = false;

    DataHandle m_dataHandle;
    bool m_firstStart = false;

    bool m_useGyroscope = false;
    CString m_pathToEmotivSDK;
    CString m_cmdForPathModification;
    CString m_oldPath;
};
}  // namespace AcquisitionServer
}  // namespace OpenViBE

#endif // TARGET_HAS_ThirdPartyEmotivAPI
