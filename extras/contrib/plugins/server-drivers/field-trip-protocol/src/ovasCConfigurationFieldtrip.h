#pragma once

#if defined(TARGET_HAS_PThread)

#include "../ovasCConfigurationBuilder.h"

namespace OpenViBE
{
    namespace AcquisitionServer
    {
        /**
         * \class CConfigurationFieldtrip
         * \author Amelie Serpollet (CEA/LETI/CLINATEC)
         * \date Mon May 23 09:48:21 2011
         * \brief The CDriverFieldtrip allows the acquisition server to acquire data from a Fieldtrip buffer.
         *
         */
        class CConfigurationFieldtrip final : public CConfigurationBuilder
        {
        public:

            CConfigurationFieldtrip(const char* gtkBuilderFilename) : CConfigurationBuilder(gtkBuilderFilename) { }
            ~CConfigurationFieldtrip() override { }

            void setHostName(const CString& hostName) { m_hostName = hostName; }
            void setHostPort(const uint32_t hostPort) { m_hostPort = hostPort; }
            void setMinSamples(const uint32_t minSamples) { m_minSamples = minSamples; }
            void setSRCorrection(const bool bSRCorrection) { m_srCorrection = bSRCorrection; }

            CString getHostName() const { return m_hostName; }
            uint32_t getHostPort() const { return m_hostPort; }
            uint32_t getMinSamples() const { return m_minSamples; }
            bool getSRCorrection() const { return m_srCorrection; }

        protected:

            bool preConfigure() override;
            bool postConfigure() override;

        private:

            CConfigurationFieldtrip();

        protected:

            GtkWidget* m_pHostName     = nullptr;
            GtkWidget* m_pHostPort     = nullptr;
            GtkWidget* m_pMinSamples   = nullptr;
            GtkWidget* m_pSRCorrection = nullptr;

            CString m_hostName    = "localhost";
            uint32_t m_hostPort   = 4000;
            uint32_t m_minSamples = 1;
            bool m_srCorrection   = true;
        };
    }  // namespace AcquisitionServer
}  // namespace OpenViBE

#endif // #if defined(TARGET_HAS_PThread)
