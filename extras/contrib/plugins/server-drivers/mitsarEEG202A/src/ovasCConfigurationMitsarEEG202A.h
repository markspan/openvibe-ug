#pragma once

#if defined(TARGET_HAS_ThirdPartyMitsar)
#if defined TARGET_OS_Windows

#include "../ovasCConfigurationBuilder.h"

#include <gtk/gtk.h>

namespace OpenViBE {
namespace AcquisitionServer {
/**
 * \class CConfigurationMitsarEEG202A
 * \author Gelu Ionescu (GIPSA-lab)
 * \date 26 April 2012
 * \brief The CConfigurationMitsarEEG202A class handles configuration specific to the Mitsar EEG 202A amplifier.
 *
 * submitted by Anton Andreev (GIPSA-lab)
 */
class CConfigurationMitsarEEG202A final : public CConfigurationBuilder
{
public:
    CConfigurationMitsarEEG202A(const char* gtKbuilderXMLFileName, uint32_t& refIndex, bool& rHardwareTaggingState);

    bool preConfigure() override;
    bool postConfigure() override;
    bool& m_rEventAndBioChannelsState;

protected:
    uint32_t& m_rRefIdx;
};
}  // namespace AcquisitionServer
}  // namespace OpenViBE

#endif
#endif
