///-------------------------------------------------------------------------------------------------
/// 
/// \file ovasCDriverBrainProductsBrainampSeries.h
/// \brief Brain Products Brainamp Series driver for OpenViBE
/// \author Yann Renard
/// \copyright Copyright (C) 2022 Inria
///
/// This program is free software: you can redistribute it and/or modify
/// it under the terms of the GNU Affero General Public License as published
/// by the Free Software Foundation, either version 3 of the License, or
/// (at your option) any later version.
///
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU Affero General Public License for more details.
///
/// You should have received a copy of the GNU Affero General Public License
/// along with this program.  If not, see <https://www.gnu.org/licenses/>.
/// 
///-------------------------------------------------------------------------------------------------

#pragma once

#include "ovasIDriver.h"
#include "../ovasCHeader.h"

#include "../ovasCSettingsHelper.h"
#include "../ovasCSettingsHelperOperators.h"

#if defined TARGET_OS_Windows

#include "ovas_defines_brainamp_series.h"

#include "ovasCConfigurationBrainProductsBrainampSeries.h"
#include "ovasCHeaderAdapterBrainProductsBrainampSeries.h"

#include <vector>
#include <list>

namespace OpenViBE {
namespace AcquisitionServer {
/**
 * \class CDriverBrainProductsBrainampSeries
 * \author Mensia Technologies
 */
class CDriverBrainProductsBrainampSeries final : public IDriver
{
public:
    friend class CConfigurationBrainProductsBrainampSeries;

    static char* getErrorMessage(uint32_t error);

    explicit CDriverBrainProductsBrainampSeries(IDriverContext& ctx);
    void release() { delete this; }
    const char* getName() override { return "Brain Products BrainAmp Series"; }

    bool initialize(const uint32_t nSamplePerSentBlock, IDriverCallback& callback) override;
    bool start() override;
    bool loop() override;
    bool stop() override;
    bool uninitialize() override;

    bool startImpedanceCheck();
    bool stopImpedanceCheck();

    bool isConfigurable() override { return true; }
    bool configure() override;
    const IHeader* getHeader() override { return &m_headerAdapter; }

protected:
    bool getDeviceDetails(uint32_t index, uint32_t* nAmplifier, uint32_t* amplifierType);

    SettingsHelper m_settings;

    IDriverCallback* m_callback = nullptr;
    CHeader m_header;
    CHeaderAdapterBrainProductsBrainampSeries m_headerAdapter;

    void* m_Device                                            = nullptr;
    CBrainampSetup* m_deviceSetup                             = nullptr;
    CBrainampCalibrationSettings* m_deviceCalibrationSettings = nullptr;

    uint32_t m_impedanceCheckSignalFrequency = 0;
    uint32_t m_decimationFactor              = 10;
    uint32_t m_nSamplePerSentBlock           = 0;
    float* m_sample                          = nullptr;
    int16_t* m_buffer                        = nullptr;
    uint16_t m_marker                        = 0;

    std::list<std::vector<double>> m_impedanceBuffers;
    std::vector<double> m_impedances;

    unsigned long m_nBytesReturned;

    uint32_t m_usbIdx = 1;
    float m_resolutionFactor[256];
    EParameter m_channelSelected[256];
    EParameter m_lowPassFilterFull[256];
    EParameter m_resolutionFull[256];
    EParameter m_dcCouplingFull[256];
    EParameter m_lowPass    = LowPass_250;
    EParameter m_resolution = Resolution_100nV;
    EParameter m_dcCoupling = DCCouping_AC;
    EParameter m_impedance  = Impedance_Low;
};

inline std::ostream& operator<<(std::ostream& out, const EParameter& var)
{
    out << int(var);
    return out;
}

inline std::istream& operator>>(std::istream& in, EParameter& var)
{
    int tmp;
    in >> tmp;
    var = EParameter(tmp);
    return in;
}

inline std::ostream& operator<<(std::ostream& out, const EParameter var[256])
{
    for (int i = 0; i < 256; ++i) { out << int(var[i]) << " "; }
    return out;
}

inline std::istream& operator>>(std::istream& in, EParameter var[256])
{
    int tmp;
    for (int i = 0; i < 256; ++i) {
        in >> tmp;
        var[i] = EParameter(tmp);
    }

    return in;
}
}  // namespace AcquisitionServer
}  // namespace OpenViBE

#endif // TARGET_OS_Windows
