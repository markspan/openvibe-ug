/* This driver uses the FieldTrip buffer open source library. 
 * See http://www.ru.nl/fcdonders/fieldtrip for details.
 */
#if defined(TARGET_HAS_PThread)

#include "ovasCDriverFieldtrip.h"
#include "ovasCConfigurationFieldtrip.h"

#include <toolkit/ovtk_all.h>

#include <pthread.h>
#include "fieldtrip/buffer.h"
#include "fieldtrip/extern.h"
#include "fieldtrip/extern.c"
#include "fieldtrip/util.c"
#include "fieldtrip/printstruct.c"
#include "fieldtrip/tcprequest.c"
#include "fieldtrip/dmarequest.c"
#include "fieldtrip/clientrequest.c"

#include <system/ovCTime.h>
//#include "GetCpuTime.h"

namespace OpenViBE {
namespace AcquisitionServer {

CDriverFieldtrip::CDriverFieldtrip(IDriverContext& ctx)
    : IDriver(ctx), m_settings("AcquisitionServer_Driver_FieldTrip", m_driverCtx.getConfigurationManager())
{
    m_header.setSamplingFrequency(0);
    m_header.setChannelCount(0);

    m_waitDataRequest      = new message_t();
    m_waitDataRequest->def = new messagedef_t();
    m_waitDataRequest->buf = nullptr;

    m_getDataRequest      = new message_t();
    m_getDataRequest->def = new messagedef_t();
    m_getDataRequest->buf = nullptr;

    m_settings.add("Header", &m_header);
    m_settings.add("MinSamples", &m_minSamples);
    m_settings.add("PortNumber", &m_portNumber);
    m_settings.add("HostName", &m_hostName);
    m_settings.add("CorrectNonIntegerSR", &m_correctNonIntegerSR);
    m_settings.load();
}

CDriverFieldtrip::~CDriverFieldtrip()
{
    if (m_waitDataRequest)
    {
        //m_waitDataRequest->buf deleted with m_waitDataRequest->def
        if (m_waitDataRequest->def) { delete m_waitDataRequest->def; }
        delete m_waitDataRequest;
    }

    if (m_getDataRequest)
    {
        //m_getDataRequest->buf deleted with m_getDataRequest->def
        if (m_getDataRequest->def) { delete m_getDataRequest->def; }
        delete m_getDataRequest;
    }
}

const char* CDriverFieldtrip::getName() { return "Fieldtrip Driver"; }

//___________________________________________________________________//
//                                                                   //

bool CDriverFieldtrip::initialize(const uint32_t nSamplePerSentBlock, IDriverCallback& callback)
{
    if (m_driverCtx.isConnected()) { return false; }

    // ...
    // initialize hardware and get available header information
    // from it :

    // connect to buffer
    if (m_connectionID != -1)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Already connected to Fieldtrip buffer " << m_hostName << ":" << m_portNumber << "\n";
        return false;
    }

    m_connectionID = open_connection(m_hostName.toASCIIString(), int(m_portNumber));
    if (m_connectionID < 0)
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Failed to connect to Fieldtrip buffer :\n" << m_hostName << ":" << m_portNumber << "\n";
        m_connectionID = -1;
        return false;
    }
    else { m_driverCtx.getLogManager() << Kernel::LogLevel_Info << "Connection to Fieldtrip buffer succeeded !\n"; }

    // request header
    if (!requestHeader())
    {
        m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Request header failed, disconnecting.\n";
        if (close_connection(m_connectionID) != 0)
        {
            m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Failed to disconnect correctly from Fieldtrip buffer\n";
        }
        m_connectionID = -1;
        return false;
    }


    if (!m_header.isChannelCountSet() || !m_header.isSamplingFrequencySet()) { return false; }

    // Builds up a buffer to store acquired samples. This buffer
    // will be sent to the acquisition server later...
    m_sample = new float[m_header.getChannelCount() * nSamplePerSentBlock];
    if (!m_sample)
    {
        delete [] m_sample;
        m_sample = nullptr;
        return false;
    }

    // Saves parameters
    m_callback            = &callback;
    m_nSamplePerSentBlock = nSamplePerSentBlock;

    if (m_minSamples < 1) { m_minSamples = 1; }
    if (m_minSamples > m_nSamplePerSentBlock) { m_minSamples = m_nSamplePerSentBlock; }

    return true;
}

bool CDriverFieldtrip::start()
{
    if (!m_driverCtx.isConnected() || m_driverCtx.isStarted()) { return false; }

    // ...
    // request hardware to start
    // sending data
    // ...
    m_firstGetDataRequest = true;
    m_waitingTimeMs       = (m_header.getSamplingFrequency() > 1000 ? 1 : (1000 / m_header.getSamplingFrequency())
    );  //time for 1 sample if >= 1ms //(1000*m_nSamplePerSentBlock)
    m_nTotalSample = 0;

    m_diffPerSample = (m_realSampling - m_header.getSamplingFrequency()) / m_realSampling;
    if (m_diffPerSample <= 0.0) { m_diffPerSample = 0.0; }
    m_driftSinceLastCorrection = 0.0;

    return true;
}

bool CDriverFieldtrip::loop()
{
    if (!m_driverCtx.isConnected()) { return false; }
    if (!m_driverCtx.isStarted()) { return true; }

    CStimulationSet stimSet;
    stimSet.resize(0);

    // ...
    // receive samples from hardware
    // put them the correct way in the sample array
    // whether the buffer is full, send it to the acquisition server
    //...
    const int count = requestChunk(stimSet);
    if (count < 0) { return false; }
    if (count == 0) { return true; }
    m_callback->setSamples(m_sample, count);
    m_callback->setStimulationSet(stimSet);

    m_driverCtx.correctDriftSampleCount(m_driverCtx.getSuggestedDriftCorrectionSampleCount());

    return true;
}

bool CDriverFieldtrip::stop()
{
    if (!m_driverCtx.isConnected()) { return false; }
    if (!m_driverCtx.isStarted()) { return false; }

    return true;
}

bool CDriverFieldtrip::uninitialize()
{
    if (!m_driverCtx.isConnected()) { return false; }
    if (m_driverCtx.isStarted()) { return false; }

    if (close_connection(m_connectionID) != 0) { m_driverCtx.getLogManager() << Kernel::LogLevel_Error << "Failed to disconnect correctly from Fieldtrip buffer\n"; }
    m_connectionID = -1;

    delete [] m_sample;
    m_sample   = nullptr;
    m_callback = nullptr;

    return true;
}

//___________________________________________________________________//
//                                                                   //
bool CDriverFieldtrip::isConfigurable()
{
    return true; // change to false if your device is not configurable
}

bool CDriverFieldtrip::configure()
{
    CConfigurationFieldtrip config(Directories::getDataDir() + "/applications/acquisition-server/interface-Fieldtrip.ui");
    config.setMinSamples(m_minSamples);
    config.setHostPort(m_portNumber);
    config.setHostName(m_hostName);
    config.setSRCorrection(m_correctNonIntegerSR);

    if (config.configure(m_header))
    {
        m_minSamples          = config.getMinSamples();
        m_portNumber          = config.getHostPort();
        m_hostName            = config.getHostName();
        m_correctNonIntegerSR = config.getSRCorrection();

        m_settings.save();

        return true;
    }

    return false;
}

//___________________________________________________________________//
//                                                                   //
bool CDriverFieldtrip::requestHeader()
{
    m_waitDataRequest->def->command = GET_HDR;
    m_waitDataRequest->def->version = VERSION;
    m_waitDataRequest->def->bufsize = 0;
    m_waitDataRequest->buf          = nullptr;

    message_t* response = nullptr;

    const int res = clientrequest(m_connectionID, m_waitDataRequest, &response);

    if (res != 0)
    {
        FreeResponse(response, "Error while asking for header. Buffer aborted ?");
        return false;
    }
    else if (response == nullptr || response->def == nullptr)
    {
        FreeResponse(response, "Error while asking for header");
        return false;
    }
    else if (response->def->command != GET_OK || response->def->bufsize == 0)
    {
        FreeResponse(response, "No header in the buffer");
        return false;
    }
    else
    {
        const unsigned int size      = response->def->bufsize;
        headerdef_t* headerDef = (headerdef_t*)response->buf;

        if (size < sizeof(headerdef_t))
        {
            FreeResponse(response, "Header received has wrong format");
            return false;
        }

        m_header.setSamplingFrequency(uint32_t(headerDef->fsample));
        m_realSampling = headerDef->fsample;
        m_header.setChannelCount(headerDef->nchans);
        m_dataType = headerDef->data_type;

        if (m_dataType != DATATYPE_FLOAT32 && m_dataType != DATATYPE_FLOAT64)
        {
            FreeResponse(response, "Data type is not supported");
            return false;
        }

        if (size == sizeof(headerdef_t)) //no chunk attached to the header
        {
            for (uint32_t i = 0; i < headerDef->nchans; i++) { m_header.setChannelName(i, ("Channel " + std::to_string(i)).c_str()); }
        }
        else //chunk(s) attached to the header, maybe channel names
        {
            int bytesInHeaderBuffer = headerDef->bufsize;
            void* chunk             = (headerdef_t*)headerDef + 1;
            bool foundChannelNames  = false;

            while (bytesInHeaderBuffer > 0)
            {
                if (((ft_chunk_t*)chunk)->def.type == FT_CHUNK_CHANNEL_NAMES)
                {
                    foundChannelNames = true;
                    char* chunkdata   = ((ft_chunk_t*)chunk)->data;
                    for (uint32_t i = 0; i < headerDef->nchans; i++)
                    {
                        std::string name = chunkdata;
                        m_header.setChannelName(i, name.c_str());
                        chunkdata = (char*)chunkdata + name.size() + 1;
                    }
                }

                bytesInHeaderBuffer -= ((ft_chunk_t*)chunk)->def.size + sizeof(ft_chunkdef_t);
                if (bytesInHeaderBuffer > 0) { chunk = (char*)chunk + ((ft_chunk_t*)chunk)->def.size + sizeof(ft_chunkdef_t); }
            }

            if (!foundChannelNames)
            {
                for (uint32_t i = 0; i < headerDef->nchans; i++) { m_header.setChannelName(i, ("Channel " + std::to_string(i)).c_str()); }
            }
        }
    } /* end valid header */

    FreeResponse(response, nullptr);

    return true;
}


int CDriverFieldtrip::requestChunk(CStimulationSet& oStimulationSet)
{
    //There are two basic opertations:
    // - configure m_getDataRequest and m_waitDataRequest
    // - use m_getDataRequest to call fieldtrip clientrequest() that gets the data from fieldtrip
    // - transpose the data to fill m_sample in the correct way expected by OpenVube
    //The rest are data validity checks and correction for non-integer sampling frequency

    //configure "wait data" request
    m_waitDataRequest->def->command = WAIT_DAT;
    m_waitDataRequest->def->version = VERSION;

    if (m_waitDataRequest->buf == nullptr)
    {
        m_waitDataRequest->def->bufsize = 0;
        waitdef_t* waitDef              = new waitdef_t();
        unsigned int requestSize        = 0;
        requestSize                     = append((void**)&m_waitDataRequest->def, sizeof(messagedef_t), waitDef, sizeof(waitdef_t));
        m_waitDataRequest->def->bufsize = requestSize - sizeof(messagedef_t);
        m_waitDataRequest->buf          = (messagedef_t*)m_waitDataRequest->def + 1;
    }

    waitdef_t* waitDef          = (waitdef_t*)m_waitDataRequest->buf;
    waitDef->threshold.nevents  = 0xFFFFFFFF;
    waitDef->threshold.nsamples = m_nSamplePerSentBlock;
    waitDef->milliseconds       = m_waitingTimeMs;

    message_t* response = nullptr;
    int res             = clientrequest(m_connectionID, m_waitDataRequest, &response);

    uint32_t nDataReceived = 0;
    uint32_t nDataToSend   = 0;

    if (res)
    {
        FreeResponse(response, "Error while asking for data. Buffer aborted ?");
        return -1;
    }
    else if (response == nullptr || response->def == nullptr)
    {
        FreeResponse(response, "Error while asking for data");
        return -1;
    }
    else if (response->def->command != WAIT_OK || response->def->bufsize != 8 || response->buf == nullptr)
    {
        FreeResponse(response, "No header in buffer anymore");
        return -1;
    }
    else
    {
        // new header received ? stop acquisition
        if (((samples_events_t*)response->buf)->nsamples < m_nTotalSample)
        {
            FreeResponse(response, "End of data");
            return -1;
        }

        // no new data
        if (((samples_events_t*)response->buf)->nsamples <= m_nTotalSample + m_minSamples)
        {
            m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "No new data\n";
            FreeResponse(response, nullptr);
            return 0;
        }

        // get data
        uint32_t lastSample = ((samples_events_t*)response->buf)->nsamples;
        if (lastSample > m_nTotalSample + m_nSamplePerSentBlock)
        {
            if (m_firstGetDataRequest) { m_nTotalSample = lastSample - m_nSamplePerSentBlock; }
            else { lastSample = m_nTotalSample + m_nSamplePerSentBlock; }
        }

        if (m_firstGetDataRequest) { m_firstGetDataRequest = false; }

        FreeResponse(response, nullptr); //prevents memory leak

        // "get data" request
        m_getDataRequest->def->command = GET_DAT;
        m_getDataRequest->def->version = VERSION;

        if (m_getDataRequest->buf == nullptr)
        {
            m_getDataRequest->def->bufsize = 0;
            datasel_t* dataSel             = new datasel_t();
            unsigned int requestSize       = 0;
            requestSize                    = append((void**)&m_getDataRequest->def, sizeof(messagedef_t), dataSel, sizeof(datasel_t));
            m_getDataRequest->def->bufsize = requestSize - sizeof(messagedef_t);
            m_getDataRequest->buf          = (messagedef_t*)m_getDataRequest->def + 1;
        }

        datasel_t* dataSel = (datasel_t*)m_getDataRequest->buf;
        dataSel->begsample = m_nTotalSample;
        dataSel->endsample = lastSample - 1;
        
        //actual data acquisition
        res = clientrequest(m_connectionID, m_getDataRequest, &response);

        if (res || !response || !response->def || response->def->version != VERSION)
        {
            FreeResponse(response, "Error while asking for data");
            return -1;
        }
        else if (response->def->command != GET_OK || response->def->bufsize == 0 || response->buf == nullptr)
        {
            FreeResponse(response, "Data are not available anymore");
            return -1;
        }
        else // data received
        {
            datadef_t* datadef = (datadef_t*)response->buf;
            void* databuf      = (datadef_t*)response->buf + 1;
            if (datadef->bufsize / (wordsize_from_type(datadef->data_type) * datadef->nchans) != lastSample - m_nTotalSample)
            {
                FreeResponse(response, "Data received from buffer are invalid");
                return -1;
            }
            else // data correct
            {
                nDataReceived = lastSample - m_nTotalSample;

                // Delete some samples if necessary.
                // Sampling rate is converted into integer in openvibe,
                // so we can have up to 1 sample too many per second.
                nDataToSend = nDataReceived;

                if (m_correctNonIntegerSR)
                {
                    m_driftSinceLastCorrection += (m_diffPerSample * nDataReceived);
                    if (m_driftSinceLastCorrection >= 1.0)
                    {
                        // delete samples
                        const uint32_t diffSamples = uint32_t(m_driftSinceLastCorrection);
                        nDataToSend -= diffSamples;
                        m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "Correction for non-integer sampling rate : " << diffSamples << " samples deleted\n";
                        m_driftSinceLastCorrection -= double(diffSamples);
                        //oStimulationSet.push_back(OVTK_GDF_Missing, CTime(m_header.getSamplingFrequency(), nDataToSend).time(), CTime(m_header.getSamplingFrequency(), diffSamples)).time();
                    }
                }

                // set data in m_sample
                double* buffer64;
                float* buffer32;
                switch (m_dataType)
                {
                    case DATATYPE_FLOAT64:
                        buffer64 = (double*)databuf;
                        for (size_t j = 0; j < m_header.getChannelCount(); j++)
                        {
                            for (uint32_t i = 0; i < nDataToSend; i++)
                            {
                                const double value = buffer64[i * m_header.getChannelCount() + j];
                                /*if ( _isnan(value) || !_finite(value) || value==DBL_MAX )
                                {
                                    m_sample[j*nDataToSend + i] = FLT_MAX;
                                    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "NaN or infinite sample received.\n";
                                }
                                else
                                {
                                    //data from IHM implant are in volts, must be in uvolts in openvibe
                                    m_sample[j*nDataToSend + i] = (float) 1000000.0f*value;
                                }*/
                                m_sample[j * nDataToSend + i] = float(value);
                            }
                        }
                        break;
                    case DATATYPE_FLOAT32:

                        buffer32 = (float*)databuf;
                        for (size_t j = 0; j < m_header.getChannelCount(); j++)
                        {
                            for (uint32_t i = 0; i < nDataToSend; i++)
                            {
                                const float value = buffer32[i * m_header.getChannelCount() + j];
                                /*if ( _isnan(value) || !_finite(value) || value==FLT_MAX )
                                {
                                    m_sample[j*nDataToSend + i] = FLT_MAX;
                                    m_driverCtx.getLogManager() << Kernel::LogLevel_Trace << "NaN or infinite sample received.\n";
                                }
                                else
                                {
                                    //data from IHM implant are in volts, must be in uvolts in openvibe
                                    m_sample[j*nDataToSend + i] = 1000000.0f*value;
                                }*/
                                m_sample[j * nDataToSend + i] = value;
                            }
                        }
                        break;
                    default:
                        FreeResponse(response, "DEV ERROR : data type not suppported");
                        return -1;
                }//end switch
            }//end data correct
        }//end data received

        FreeResponse(response, nullptr);//we copied the data from response, so now we need to release this memory to avoid memory leak

        m_nTotalSample = lastSample;
    }

    return nDataToSend; // no error
}

void CDriverFieldtrip::FreeResponse(message_t* response, const char* message)
{
    if (message != nullptr) { m_driverCtx.getLogManager() << Kernel::LogLevel_Error << message << "\n"; }

    if (response->buf) { free(response->buf); }
    if (response->def) { free(response->def); }
    free(response);
    response = nullptr;
}

}  // namespace AcquisitionServer
}  // namespace OpenViBE
#endif