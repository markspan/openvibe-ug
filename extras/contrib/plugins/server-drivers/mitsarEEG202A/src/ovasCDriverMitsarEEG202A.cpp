#if defined(TARGET_HAS_ThirdPartyMitsar)
#if defined TARGET_OS_Windows

#include "ovasCDriverMitsarEEG202A.h"

#include "ovasCConfigurationMitsarEEG202A.h"

#include <toolkit/ovtk_all.h>

#include <system/ovCTime.h>

#include <iostream>
#include <fstream>
#include <iomanip>

#include <math.h>

#include <windows.h>

#define msleep(ms) Sleep(ms) // Sleep windows

namespace OpenViBE {
namespace AcquisitionServer {

//___________________________________________________________________//
//                                                                   //

CDriverMitsarEEG202A::CDriverMitsarEEG202A(IDriverContext& ctx)
    : IDriver(ctx), m_settings("AcquisitionServer_Driver_MitsarEEG202A", m_driverCtx.getConfigurationManager())
{
    m_header.setChannelCount(CHANNEL_NB);
    m_header.setSamplingFrequency(SAMPLING_RATE);

    m_settings.add("Header", &m_header);
    m_settings.add("RefIndex", &m_refIdx);
    m_settings.add("EventAndBioChannelsState", &m_eventAndBioChannelsState);
    m_settings.load();
}

void CDriverMitsarEEG202A::release()
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "CDriverMitsarEEG202A::release\n";
    delete this;
}

const char* CDriverMitsarEEG202A::getName()
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "CDriverMitsarEEG202A::getName\n";
    return "Mitsar EEG 202 - A";
}

//___________________________________________________________________//
//                                                                   //


#define _Mitsar_EEG202A_DLLFileName_ "MitsarDll.dll"

typedef int ( __stdcall *MitsarDLL_initialize)();
typedef int ( __stdcall *MitsarDLL_start)(int refType);
typedef int ( __stdcall *MitsarDLL_stop)();
typedef int ( __stdcall *MitsarDLL_uninitialize)();
typedef int ( __stdcall *MitsarDLL_loop)(float* sample);

static HINSTANCE mitsarDLLInstance                  = nullptr;
static MitsarDLL_initialize mitsarDLLInitialize     = nullptr;
static MitsarDLL_start mitsarDLLStart               = nullptr;
static MitsarDLL_stop mitsarDLLStop                 = nullptr;
static MitsarDLL_uninitialize mitsarDLLUninitialize = nullptr;
static MitsarDLL_loop mitsarDLLLoop                 = nullptr;

//___________________________________________________________________//
//                                                                   //

bool CDriverMitsarEEG202A::initialize(const uint32_t nSamplePerSentBlock, IDriverCallback& callback)
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "CDriverMitsarEEG202A::initialize\n";
    if (m_driverCtx.isConnected()) { return false; }

    mitsarDLLInstance = ::LoadLibrary(_Mitsar_EEG202A_DLLFileName_);

    if (!mitsarDLLInstance)
    {
        std::cout << "INIT ERROR : Load Library" << std::endl;
        return false;
    }

    mitsarDLLInitialize   = (MitsarDLL_initialize)GetProcAddress(mitsarDLLInstance, "MITSAR_EEG202_initialize");
    mitsarDLLStart        = (MitsarDLL_start)GetProcAddress(mitsarDLLInstance, "MITSAR_EEG202_start");
    mitsarDLLStop         = (MitsarDLL_stop)GetProcAddress(mitsarDLLInstance, "MITSAR_EEG202_stop");
    mitsarDLLUninitialize = (MitsarDLL_uninitialize)GetProcAddress(mitsarDLLInstance, "MITSAR_EEG202_uninitialize");
    mitsarDLLLoop         = (MitsarDLL_loop)GetProcAddress(mitsarDLLInstance, "MITSAR_EEG202_loop");

    if (!mitsarDLLInitialize || !mitsarDLLStart || !mitsarDLLStop || !mitsarDLLUninitialize || !mitsarDLLLoop)
    {
        std::cout << "INIT ERROR : DLL functions list" << std::endl;
        std::cout << "mitsarDLLInitialize : " << mitsarDLLInitialize << std::endl;
        std::cout << "mitsarDLLStart : " << mitsarDLLStart << std::endl;
        std::cout << "mitsarDLLStop : " << mitsarDLLStart << std::endl;
        std::cout << "mitsarDLLUninitialize : " << mitsarDLLUninitialize << std::endl;
        std::cout << "mitsarDLLLoop : " << mitsarDLLLoop << std::endl;
        FreeLibrary(mitsarDLLInstance);
        mitsarDLLInstance     = nullptr;
        mitsarDLLInitialize   = nullptr;
        mitsarDLLStart        = nullptr;
        mitsarDLLStop         = nullptr;
        mitsarDLLUninitialize = nullptr;
        mitsarDLLLoop         = nullptr;
        return false;
    }

    const int error = mitsarDLLInitialize();
    if (error)
    {
        FreeLibrary(mitsarDLLInstance);
        mitsarDLLInstance     = nullptr;
        mitsarDLLInitialize   = nullptr;
        mitsarDLLStart        = nullptr;
        mitsarDLLStop         = nullptr;
        mitsarDLLUninitialize = nullptr;
        mitsarDLLLoop         = nullptr;
        std::cout << "INIT ERROR : Init DLL function" << std::endl;
        return false;
    }

    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "CDriverMitsarEEG202A::CDriverMitsarEEG202A\n";

    if (this->m_eventAndBioChannelsState) m_header.setChannelCount(CHANNEL_NB);
    else m_header.setChannelCount(CHANNEL_NB - 2);

    m_header.setSamplingFrequency(SAMPLING_RATE);

    //uint32_t idx = 0;
    m_header.setChannelName(0, "FP1");
    m_header.setChannelName(1, "FPz");
    m_header.setChannelName(2, "FP2");
    m_header.setChannelName(3, "F7");
    m_header.setChannelName(4, "F3");
    m_header.setChannelName(5, "Fz");
    m_header.setChannelName(6, "F4");
    m_header.setChannelName(7, "F8");
    m_header.setChannelName(8, "FT7");
    m_header.setChannelName(9, "FC3");
    m_header.setChannelName(10, "FCz");
    m_header.setChannelName(11, "FC4");
    m_header.setChannelName(12, "FT8");
    m_header.setChannelName(13, "T3");
    m_header.setChannelName(14, "C3");
    m_header.setChannelName(15, "Cz");
    m_header.setChannelName(16, "C4");
    m_header.setChannelName(17, "T4");
    m_header.setChannelName(18, "TP7");
    m_header.setChannelName(19, "CP3");
    m_header.setChannelName(20, "CPz");
    m_header.setChannelName(21, "CP4");
    m_header.setChannelName(22, "TP8");
    m_header.setChannelName(23, "T5");
    m_header.setChannelName(24, "P3");
    m_header.setChannelName(25, "Pz");
    m_header.setChannelName(26, "P4");
    m_header.setChannelName(27, "T6");
    m_header.setChannelName(28, "O1");
    m_header.setChannelName(29, "Oz");
    m_header.setChannelName(30, "O2");

    if (this->m_eventAndBioChannelsState)
    {
        m_header.setChannelName(31, "CH_Event");    // Event signals (0/+3v)
        m_header.setChannelName(32, "Bio1");        // Biological signals (ECG, EMG, EOG...)
    }

    const uint32_t stimulationChannel = 31;

    m_iSamples.resize(SAMPLES_NB * CHANNEL_NB);
    m_oSamples.resize(SAMPLES_NB * CHANNEL_NB);

    m_lastStimulation = STIMULATION_0;
    // memorize the stimulation channel position
    m_stimulationChannel = &m_oSamples[SAMPLES_NB * stimulationChannel];

    m_stimulationIDs.resize(SAMPLES_NB);
    m_stimulationDates.resize(SAMPLES_NB);

    m_callback = &callback;

    return true;
}

bool CDriverMitsarEEG202A::start()
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "CDriverMitsarEEG202A::start\n";


    if (!m_driverCtx.isConnected())
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "CDriverMitsarEEG202A::start - not connected.\n";
        return false;
    }
    if (m_driverCtx.isStarted())
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "CDriverMitsarEEG202A::start - already started.\n";
        return false;
    }

    //) Check reference type : 1->Ref=A1-Left A2-Righ else Ref=Common(A1&A2) 
    int Ref_type = 1;
    if (m_refIdx == 1)
    {
        std::cout << "Ref= A1-Left A2-Right\n";
        Ref_type = 1;
    }
    else
    {
        std::cout << "Ref=Common(A1&A2) \n";
        Ref_type = 0;
    }

    const int error = mitsarDLLStart(Ref_type);
    printf("Dll start %s\n", error ? "WRONG" : "OK");

    return (error ? false : true);
}

bool CDriverMitsarEEG202A::loop()
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Debug << "CDriverMitsarEEG202A::loop\n";

    if (!m_driverCtx.isConnected()) { return false; }
    if (!m_driverCtx.isStarted()) { return true; }

    if (mitsarDLLLoop(&m_iSamples[0])) { return false; }

    // INPUT  s1 c1 c2 c3 ....cN
    //        s2 c1 c2 c3 ....cN
    //        s3 c1 c2 c3 ....cN
    //        sM c1 c2 c3 ....cN
    // OUTPUT c1 s1 s2 s3 ....sM
    //        c2 s1 s2 s3 ....sM
    //        c3 s1 s2 s3 ....sM
    //        cN s1 s2 s3 ....sM

    // transpose data from sample lines to channel lines
    for (uint32_t j = 0; j < CHANNEL_NB; ++j) // channel
    {
        for (uint32_t i = 0; i < SAMPLES_NB; ++i) // sample
        {
            m_oSamples[j * SAMPLES_NB + i] = m_iSamples[CHANNEL_NB * i + j];//CHANNEL_NB
        }
    }

    if (m_eventAndBioChannelsState)
    {
        // look for stimulations
        uint32_t nStimulations = 0;
        float* stimChannel     = m_stimulationChannel;

        for (uint32_t iSample = 0; iSample < SAMPLES_NB; iSample++, stimChannel++)
        {
            uint32_t stim = STIMULATION_0;

            //std::cout << *stimChannel << " " << stim << std::endl;

            // Stim192 < 0.03 <= Stim128 < 0.1 <= Stim64 < 0.16 <= Stim0
            float stimulationF = *stimChannel;

            if (stimulationF < 0.03)
            {
                stim         = STIMULATION_192;
                *stimChannel = 3.0f;
            }
            else if (stimulationF < 0.1)
            {
                stim         = STIMULATION_128;
                *stimChannel = 2.0f;
            }
            else if (stimulationF < 0.16)
            {
                stim         = STIMULATION_64;
                *stimChannel = 1.0f;
            }
            else { *stimChannel = 0.0f; }


            if ((stim != STIMULATION_0) && (stim != m_lastStimulation))
            {
                m_stimulationIDs[nStimulations]   = stim;
                m_stimulationDates[nStimulations] = iSample;
                nStimulations++;
            }

            m_lastStimulation = stim;

            //std::cout << *stimChannel << " " << stim << std::endl;
        }

        // prepare stimulations
        CStimulationSet stimSet;
        stimSet.resize(nStimulations);


        for (uint32_t iStimulation = 0; iStimulation < nStimulations; ++iStimulation)
        {
            uint64_t identifier;
            switch (m_stimulationIDs[iStimulation])
            {
                default: 
                case STIMULATION_64: identifier = OVTK_StimulationId_Label_01;  break;
                case STIMULATION_128: identifier = OVTK_StimulationId_Label_02; break;
                case STIMULATION_192: identifier = OVTK_StimulationId_Label_03; break;
            }

            const uint64_t date = CTime(m_header.getSamplingFrequency(), uint64_t(m_stimulationDates[iStimulation])).time();

            stimSet.setId(iStimulation, identifier);
            stimSet.setDate(iStimulation, date);
            stimSet.setDuration(iStimulation, 1);

            //std::cout << "Trigger " << std::hex << m_stimulationIDs[iStimulation] << " ";
        }

        //m_callback->setStimulationSet(stimSet);
    }

    // Drift correction from GUI
    m_driverCtx.correctDriftSampleCount(m_driverCtx.getSuggestedDriftCorrectionSampleCount());

    // SEND SAMPLES & STIMULATIONS
    m_callback->setSamples(&m_oSamples[0], uint32_t(SAMPLES_NB));


    msleep(1); // free CPU ressources
    return true;
}


bool CDriverMitsarEEG202A::stop()
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "CDriverMitsarEEG202A::stop\n";

    if (!m_driverCtx.isConnected() || !m_driverCtx.isStarted()) { return false; }

    return (mitsarDLLStop() ? false : true);
}

bool CDriverMitsarEEG202A::uninitialize()
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "CDriverMitsarEEG202A::uninitialize\n";

    if (!m_driverCtx.isConnected() || m_driverCtx.isStarted()) { return false; }

    FreeLibrary(mitsarDLLInstance);
    m_callback            = nullptr;
    mitsarDLLInstance     = nullptr;
    mitsarDLLInitialize   = nullptr;
    mitsarDLLStart        = nullptr;
    mitsarDLLStop         = nullptr;
    mitsarDLLUninitialize = nullptr;
    mitsarDLLLoop         = nullptr;

    return true;
}

//___________________________________________________________________//
//                                                                   //

bool CDriverMitsarEEG202A::isConfigurable()
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "CDriverMitsarEEG202A::isConfigurable\n";
    return true;
}

bool CDriverMitsarEEG202A::configure()
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "CDriverMitsarEEG202A::configure\n";
    CConfigurationMitsarEEG202A config(Directories::getDataDir() + "/applications/acquisition-server/interface-Mitsar-EEG202.ui", m_refIdx, m_eventAndBioChannelsState);
    if (!config.configure(m_header)) { return false; }
    m_settings.save();
    return true;
}

}  // namespace AcquisitionServer
}  // namespace OpenViBE
#endif
#endif
