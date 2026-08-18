#pragma once

#include "../ovasCConfigurationBuilder.h"
#include "ovasIDriver.h"

namespace OpenViBE {
namespace AcquisitionServer {
/**
 * \class CConfigurationMBTSmarting
 * \author mBrainTrain dev team (mBrainTrain)
 * \date Tue Oct 14 16:09:43 2014
 * \brief The CConfigurationMBTSmarting handles the configuration dialog specific to the MBTSmarting device.
 *
 * TODO: details
 *
 * \sa CDriverMBTSmarting
 */
class CConfigurationMBTSmarting final : public CConfigurationBuilder
{
public:

    // you may have to add to your constructor some reference parameters
    // for example, a connection ID:
    CConfigurationMBTSmarting(IDriverContext& ctx, const char* gtkBuilderFilename, uint32_t& rConnectionId);
    //CConfigurationMBTSmarting(IDriverContext& ctx, const char* gtkBuilderFilename);

    bool preConfigure() override;
    bool postConfigure() override;
    ~CConfigurationMBTSmarting() override;

protected:

    IDriverContext& m_driverCtx;

private:

    /*
     * Insert here all specific attributes, such as a connection ID.
     * use references to directly modify the corresponding attribute of the driver
     * Example:
     */
    uint32_t& m_connectionID;
    //::GtkListStore* m_listStore;
};
}  // namespace AcquisitionServer
}  // namespace OpenViBE
