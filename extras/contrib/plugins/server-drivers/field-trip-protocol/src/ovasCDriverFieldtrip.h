/* This driver uses the FieldTrip buffer open source library. 
 * See http://www.ru.nl/fcdonders/fieldtrip for details.
 */
#pragma once

#if defined(TARGET_HAS_PThread)

#include "ovasIDriver.h"
#include "../ovasCHeader.h"
#include <openvibe/ov_all.h>

#include "fieldtrip/message.h"

#include "../ovasCSettingsHelper.h"
#include "../ovasCSettingsHelperOperators.h"

// for GET_CPU_TIME
#include <iostream>
#include <fstream>

namespace OpenViBE
{
    namespace AcquisitionServer
    {
        /**
         * \class CDriverFieldtrip
         * \author Amelie Serpollet (CEA/LETI/CLINATEC)
         * \date Mon May 23 09:48:21 2011
         * \brief The CDriverFieldtrip allows the acquisition server to acquire data from a Fieldtrip buffer.
         *
         * TODO: details
         *
         */
        class CDriverFieldtrip final : public IDriver
        {
        public:

            CDriverFieldtrip(IDriverContext& ctx);
            ~CDriverFieldtrip() override;
            const char* getName() override;

            bool initialize(const uint32_t nSamplePerSentBlock, IDriverCallback& callback) override;
            bool uninitialize() override;

            bool start() override;
            bool stop() override;
            bool loop() override;

            bool isConfigurable() override;
            bool configure() override;
            const IHeader* getHeader() override { return &m_header; }

            void FreeResponse(message_t* response, const char* message);

        protected:

            bool requestHeader();
            int requestChunk(CStimulationSet& oStimulationSet);

            IDriverCallback* m_callback = nullptr;
            CHeader m_header;
            SettingsHelper m_settings;
            uint32_t m_nSamplePerSentBlock = 0;
            float* m_sample                = nullptr;
            uint32_t m_dataType            = DATATYPE_UNKNOWN;
    
            // Connection to Fieldtrip buffer
            CString m_hostName    = "localhost";
            uint32_t m_portNumber = 1979;
            int m_connectionID    = -1;
            uint32_t m_minSamples = 1;

            // Avoid frequent memory allocation
            message_t* m_waitDataRequest = nullptr;
            message_t* m_getDataRequest  = nullptr;

            uint32_t m_nTotalSample  = 0;
            uint32_t m_waitingTimeMs = 0;

            bool m_firstGetDataRequest = false;

            bool m_correctNonIntegerSR        = true; // ???
            double m_realSampling             = 0;
            double m_diffPerSample            = 0; // ???
            double m_driftSinceLastCorrection = 0;

            // count time lost for "get cpu time" :
            //double m_mesureLostTime = 0;
            //uint32_t m_mesureNumber = 0;
        };
    }  // namespace AcquisitionServer
}  // namespace OpenViBE
#endif // #if defined(TARGET_HAS_PThread)
