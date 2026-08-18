#pragma once

#if defined(TARGET_HAS_ThirdPartyEEGOAPI)

#include <ovasIDriver.h>
#include <../ovasCConfigurationBuilder.h>
#include <gtk/gtk.h>

#include "ovasCHeaderEEGO.h"

namespace OpenViBE {
namespace AcquisitionServer {
/**
 * \class CConfigurationEEGO
 * \author Steffen Heimes (Eemagine GmbH)
 * \date Fri May 27 21:48:42 2011
 * \brief The CConfigurationEEGO handles the configuration dialog for setting specific for EEGO.
 * \sa CDriverEEGO
 */
class CConfigurationEEGO final : public CConfigurationBuilder
{
public:

    CConfigurationEEGO(IDriverContext& ctx, const char* gtkBuilderFilename, CHeaderEEGO& eegoHeader);

    bool preConfigure() override;
    bool postConfigure() override;

    // Data
protected:

    IDriverContext& m_driverCtx;

    // Methods
private:

    static void updateChannelNumCB(GtkWidget* widget, CConfigurationEEGO* pThis);

    // Data
    CHeaderEEGO& m_eegoHeader;

    GtkComboBox* m_eegRangeComboBox = nullptr;
    GtkComboBox* m_bipRangeComboBox = nullptr;
    GtkEntry* m_eegEntryMask        = nullptr;
    GtkEntry* m_bipEntryMask        = nullptr;
    GtkEntry* m_nChannelEntry       = nullptr;
};
}  // namespace AcquisitionServer
}  // namespace OpenViBE

#endif
