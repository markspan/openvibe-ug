///-------------------------------------------------------------------------------------------------
/// 
/// \file ovasCDriverEncephalan.h
/// \brief The CDriverEncephalan allows the acquisition server to acquire data from a Encephalan device.
/// \author Alexey Minin (UrFU)
/// \version 1.0.
/// \date 02/01/2019.
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

#if defined TARGET_OS_Windows

#include "ovasIDriver.h"
#include "../ovasCHeader.h"
#include <openvibe/ov_all.h>

#include "../ovasCSettingsHelper.h"
#include "../ovasCSettingsHelperOperators.h"

#include "winsock.h"
#include <string>

namespace OpenViBE {
namespace AcquisitionServer {
/// <summary>   The CDriverEncephalan allows the acquisition server to acquire data from a Encephalan device. </summary>
/// <seealso cref="CConfigurationEncephalan" />
class CDriverEncephalan final : public IDriver
{
public:
    explicit CDriverEncephalan(IDriverContext& driverContext);
    ~CDriverEncephalan() override = default;
    const char* getName() override { return "Encephalan"; }

    bool initialize(const uint32_t sampleCountPerBlock, IDriverCallback& callback) override;
    bool uninitialize() override;

    bool start() override;
    bool stop() override;
    bool loop() override;

    bool isConfigurable() override { return true; } // change to false if your device is not configurable
    bool configure() override;
    const IHeader* getHeader() override { return &m_header; }

    bool isFlagSet(const EDriverFlag flag) const override { return flag == EDriverFlag::IsUnstable; }

protected:
    bool connectEncephalan();
    bool sendRequestForData() const;
    bool sendData(BYTE* data, int dataSize) const;
    bool receiveData();
    void readData(BYTE* data, int dataSize);
    static void getData(BYTE* & data, int& dataSize, void* dstData, int dstSize);
    void receiveEEGData(BYTE* curData, int curDataSize);

    SettingsHelper m_settings;
    IDriverCallback* m_callback = nullptr;
    CHeader m_header;
    uint32_t m_nSamplePerBlock = 0;
    float* m_sample            = nullptr;

    std::string m_connectionIp;
    uint32_t m_connectionPort = 120;
    uint32_t m_currentPoint   = 0;
    SOCKET m_client           = 0;
};
}  // namespace AcquisitionServer
}  // namespace OpenViBE
#endif // TARGET_OS_Windows
