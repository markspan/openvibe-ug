/*
 * \author Christoph Veigl, Yann Renard
 *
 * \copyright AGPL3
 *
 */
#pragma once

#include "../ovasCConfigurationBuilder.h"

#include <gtk/gtk.h>

namespace OpenViBE {
namespace AcquisitionServer {
class CConfigurationOpenEEGModularEEG final : public CConfigurationBuilder
{
public:
    CConfigurationOpenEEGModularEEG(const char* gtkBuilderFilename, uint32_t& usbIdx);
    ~CConfigurationOpenEEGModularEEG() override;

    bool preConfigure() override;
    bool postConfigure() override;

protected:
    uint32_t& m_usbIdx;
    GtkListStore* m_listStore = nullptr;
};
}  // namespace AcquisitionServer
}  // namespace OpenViBE
