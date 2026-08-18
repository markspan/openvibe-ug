#if defined(TARGET_HAS_ThirdPartyNeXus)

#include "ovasCDriverMindMediaNeXus32B.h"
#include "../ovasCConfigurationBuilder.h"

#include <toolkit/ovtk_all.h>

#include <system/ovCTime.h>
#include <system/ovCMemory.h>

#if defined TARGET_OS_Windows

#include <cmath>
#include <windows.h>

namespace OpenViBE {
namespace AcquisitionServer {

static const uint32_t INTERNALE_BUFFER_COUNT = 32;

//___________________________________________________________________//
//                                                                   //

CDriverMindMediaNeXus32B::CDriverMindMediaNeXus32B(IDriverContext& ctx)
    : IDriver(ctx), m_settings("AcquisitionServer_Driver_MindMediaNexus32B", m_driverCtx.getConfigurationManager())
{
    m_header.setSamplingFrequency(512);
    m_header.setChannelCount(4);

    m_settings.add("Header", &m_header);
    m_settings.load();
}

void CDriverMindMediaNeXus32B::release() { delete this; }

const char* CDriverMindMediaNeXus32B::getName() { return "MindMedia NeXus32B"; }

//___________________________________________________________________//
//                                                                   //

#define _MindMedia_NeXus32B_DLLFileName_ "NeXusDLL.dll"

typedef void (*NeXusDLL_ProcessData)(int sampleCount, int channel, float* sample);
typedef DWORD (*NeXusDLL_Init)(NeXusDLL_ProcessData fpProcessData);
typedef DWORD (*NeXusDLL_Start)(DWORD* sampling);
typedef DWORD (*NeXusDLL_Stop)();

static HANDLE mutex                   = nullptr;
static HINSTANCE neXusDLLInstance     = nullptr;
static NeXusDLL_Init fpNeXusDLLInit   = nullptr;
static NeXusDLL_Start fpNeXusDLLStart = nullptr;
static NeXusDLL_Stop fpNeXusDLLStop   = nullptr;

static CDriverMindMediaNeXus32B* driver = nullptr;

//___________________________________________________________________//
//                                                                   //

static void processData(const int sampleCount, const int channel, float* sample)
{
    if (driver) { driver->processData(uint32_t(sampleCount), uint32_t(channel), sample); }
}

//___________________________________________________________________//
//                                                                   //

bool CDriverMindMediaNeXus32B::initialize(const uint32_t nSamplePerSentBlock, IDriverCallback& callback)
{
    if (m_driverCtx.isConnected()) { return false; }

    const CString binPath = m_driverCtx.getConfigurationManager().expand("${Path_Bin}");
    neXusDLLInstance      = ::LoadLibrary((binPath + "/" + _MindMedia_NeXus32B_DLLFileName_).toASCIIString());
    if (!neXusDLLInstance) { return false; }

    fpNeXusDLLInit  = NeXusDLL_Init(GetProcAddress(neXusDLLInstance, "InitNeXusDevice"));
    fpNeXusDLLStart = NeXusDLL_Start(GetProcAddress(neXusDLLInstance, "StartNeXusDevice"));
    fpNeXusDLLStop  = NeXusDLL_Stop(GetProcAddress(neXusDLLInstance, "StopNeXusDevice"));
    m_sample        = new float[m_header.getChannelCount() * nSamplePerSentBlock * INTERNALE_BUFFER_COUNT];

    if (!fpNeXusDLLInit || !fpNeXusDLLStart || !fpNeXusDLLStop || !m_sample)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Error finding NeXus API functions / allocating sample buffer\n";

        FreeLibrary(neXusDLLInstance);
        delete [] m_sample;
        neXusDLLInstance = nullptr;
        fpNeXusDLLInit   = nullptr;
        fpNeXusDLLStart  = nullptr;
        fpNeXusDLLStop   = nullptr;
        m_sample         = nullptr;
        mutex            = nullptr;
        return false;
    }

    mutex = CreateMutex(nullptr, FALSE, nullptr); // default security attributes, not initially owned, no name

    if (!mutex)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Could not create synchronisation mutex\n";
        FreeLibrary(neXusDLLInstance);
        delete [] m_sample;
        neXusDLLInstance = nullptr;
        fpNeXusDLLInit   = nullptr;
        fpNeXusDLLStart  = nullptr;
        fpNeXusDLLStop   = nullptr;
        m_sample         = nullptr;
        mutex            = nullptr;
        return false;
    }

    DWORD error = fpNeXusDLLInit(OpenViBE::AcquisitionServer::processData);
    if (error)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Could not initialize device with NeXus API\n";
        FreeLibrary(neXusDLLInstance);
        delete [] m_sample;
        CloseHandle(mutex);
        neXusDLLInstance = nullptr;
        fpNeXusDLLInit   = nullptr;
        fpNeXusDLLStart  = nullptr;
        fpNeXusDLLStop   = nullptr;
        m_sample         = nullptr;
        mutex            = nullptr;
        return false;
    }

    m_callback            = &callback;
    m_nSamplePerSentBlock = nSamplePerSentBlock;
    m_sampleIdx           = 0;
    driver                = this;

    return true;
}

bool CDriverMindMediaNeXus32B::start()
{
    if (!m_driverCtx.isConnected() || m_driverCtx.isStarted()) { return false; }

    DWORD sampling    = DWORD(m_header.getSamplingFrequency());
    const DWORD error = fpNeXusDLLStart(&sampling);
    if (error)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Could not start acquisition with NeXus API\n";
        return false;
    }

    return true;
}

bool CDriverMindMediaNeXus32B::loop()
{
    if (!m_driverCtx.isConnected() || !m_driverCtx.isStarted()) { return true; }

    WaitForSingleObject(mutex, INFINITE);
    if (m_sampleIdx < m_nSamplePerSentBlock)
    {
        ReleaseMutex(mutex);
        return true;
    }

    m_callback->setSamples(m_sample);
    m_driverCtx.correctDriftSampleCount(m_driverCtx.getSuggestedDriftCorrectionSampleCount());
    memcpy(m_sample, m_sample + m_header.getChannelCount() * m_nSamplePerSentBlock, m_header.getChannelCount() * m_nSamplePerSentBlock * sizeof(float));
    m_sampleIdx -= m_nSamplePerSentBlock;
    ReleaseMutex(mutex);

    return true;
}

bool CDriverMindMediaNeXus32B::stop()
{
    if (!m_driverCtx.isConnected() || !m_driverCtx.isStarted()) { return false; }

    const DWORD error = fpNeXusDLLStop();
    if (error)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Could not stop acquisition with NeXus API\n";
        return false;
    }
    return true;
}

bool CDriverMindMediaNeXus32B::uninitialize()
{
    if (!m_driverCtx.isConnected() || m_driverCtx.isStarted()) { return false; }

    FreeLibrary(neXusDLLInstance);
    delete [] m_sample;
    CloseHandle(mutex);
    m_sample         = nullptr;
    m_callback       = nullptr;
    neXusDLLInstance = nullptr;
    fpNeXusDLLInit   = nullptr;
    fpNeXusDLLStart  = nullptr;
    fpNeXusDLLStop   = nullptr;
    driver           = nullptr;
    mutex            = nullptr;

    return true;
}

//___________________________________________________________________//
//                                                                   //

bool CDriverMindMediaNeXus32B::configure()
{
    CConfigurationBuilder config(Directories::getDataDir() + "/applications/acquisition-server/interface-MindMedia-NeXus32B.ui");
    if (!config.configure(m_header)) { return false; }
    m_settings.save();
    return true;
}

void CDriverMindMediaNeXus32B::processData(uint32_t sampleCount, uint32_t channel, float* sample)
{
    WaitForSingleObject(mutex, INFINITE);

    if (m_sampleIdx < m_nSamplePerSentBlock * INTERNALE_BUFFER_COUNT)
    {
        const uint32_t bufferIdx = m_sampleIdx / m_nSamplePerSentBlock;
        const uint32_t sampleIdx = m_sampleIdx % m_nSamplePerSentBlock;

        for (size_t i = 0; i < m_header.getChannelCount(); ++i)
        {
            m_sample[bufferIdx * m_nSamplePerSentBlock * m_header.getChannelCount() + i * m_nSamplePerSentBlock + sampleIdx] = sample[i];
        }

        m_sampleIdx++; // Please don't overflow :o)
    }

    ReleaseMutex(mutex);
}

}  // namespace AcquisitionServer
}  // namespace OpenViBE
#endif // defined TARGET_OS_Windows
#endif
