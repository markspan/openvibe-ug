#pragma once

#include "ovasCDriverGenericRawReader.h"

#include "../ovasCSettingsHelper.h"

#include <cstdio>

namespace OpenViBE {
namespace AcquisitionServer {
/**
 * \class CDriverGenericRawFileReader
 * \author Yann Renard (INRIA)
 */
class CDriverGenericRawFileReader final : public CDriverGenericRawReader
{
public:
    explicit CDriverGenericRawFileReader(IDriverContext& ctx);

    const char* getName() override { return "Generic Raw File Reader"; }
    bool isConfigurable() override { return true; }
    bool configure() override;

protected:
    bool open() override;
    bool close() override;
    bool read() override;

    SettingsHelper m_settings;
    FILE* m_file = nullptr;
    CString m_filename;
};
}  // namespace AcquisitionServer
}  // namespace OpenViBE
