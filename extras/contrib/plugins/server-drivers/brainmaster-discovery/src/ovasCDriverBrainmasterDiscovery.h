///-------------------------------------------------------------------------------------------------
/// 
/// \file ovasCDriverBrainmasterDiscovery.h
/// \copyright Copyright (C) 2012, Yann Renard. All rights reserved.
/// 
/// This library is free software; you can redistribute it and/or
/// modify it under the terms of the GNU Lesser General Public
/// License as published by the Free Software Foundation; either
/// version 2.1 of the License, or (at your option) any later version.
/// 
/// This library is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
/// Lesser General Public License for more details.
/// 
/// You should have received a copy of the GNU Lesser General Public
/// License along with this library; if not, write to the Free Software
/// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
/// MA 02110-1301  USA
/// 
///-------------------------------------------------------------------------------------------------

#pragma once

#if defined TARGET_HAS_ThirdPartyBrainmasterCodeMakerAPI

#include "ovasIDriver.h"
#include "../ovasCHeader.h"

#include <vector>
#include <map>
#include <string>

namespace OpenViBE {
namespace AcquisitionServer {
/**
 * \class CDriverBrainmasterDiscovery
 * \author Yann Renard
 */
class CDriverBrainmasterDiscovery final : public IDriver
{
public:

    explicit CDriverBrainmasterDiscovery(IDriverContext& ctx);
    void release() { delete this; }
    const char* getName() override { return "Brainmaster Discovery and Atlantis"; }

    bool initialize(const uint32_t nSamplePerSentBlock, IDriverCallback& callback) override;
    bool uninitialize() override;

    bool start() override;
    bool stop() override;
    bool loop() override;
    bool read(unsigned char* frame, uint32_t size);
    uint32_t sync();

    bool isConfigurable() override { return true; }
    bool configure() override;
    const IHeader* getHeader() override { return &m_header; }

protected:

    IDriverCallback* m_callback = nullptr;
    CHeader m_header;
    uint32_t m_nSamplePerSentBlock = 0;

    std::string m_deviceSerial;
    std::string m_devicePasskey;
    std::string m_frameDumpFilename;
    bool m_frameDumpFlag = false;

    uint32_t m_port             = 0;
    uint32_t m_portReal         = 0;
    uint32_t m_preset           = 0;
    uint32_t m_type             = 0;
    uint32_t m_baudRate         = 0;
    uint32_t m_baudRateReal     = 0;
    uint32_t m_samplingRate     = 0;
    uint32_t m_samplingRateReal = 0;
    uint32_t m_bitDepth         = 0;
    uint32_t m_bitDepthReal     = 0;
    uint32_t m_notchFilters     = 0;
    uint32_t m_notchFiltersReal = 0;
    uint32_t m_frameSize        = 0;
    uint32_t m_dataOffset       = 0;

    std::map<uint32_t, uint32_t> m_baudRates;
    std::map<uint32_t, uint32_t> m_baudRateValues;
    std::map<uint32_t, uint32_t> m_notchsFilters;
    std::map<uint32_t, uint32_t> m_notchFiltersValues;
    std::map<uint32_t, uint32_t> m_bitDepths;
    std::map<uint32_t, uint32_t> m_bitDepthValues;
    std::map<uint32_t, uint32_t> m_channelTypes;
    std::map<uint32_t, uint32_t> m_channelSelectionMasks;
    std::map<uint32_t, std::map<uint32_t, float (*)(unsigned char*&)>> m_bitDepthDecoders;
    std::map<uint32_t, int (*)()> m_startModules;
    std::map<uint32_t, int (*)()> m_stopModules;

    std::vector<float> m_samples;
    std::vector<unsigned char> m_buffers;

    uint32_t m_syncByte        = 0;
    bool m_wasLastFrameCorrect = false;
};
}  // namespace AcquisitionServer
}  // namespace OpenViBE

#endif // TARGET_HAS_ThirdPartyBrainmasterCodeMakerAPI
