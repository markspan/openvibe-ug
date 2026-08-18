///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmSignalDecimation.hpp
/// \brief Classes for the Box Signal Decimation.
/// \author Yann Renard (Inria).
/// \version 1.0.
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

#include "defines.hpp"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {
class CBoxAlgorithmSignalDecimation final : public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }
    bool initialize() override;
    bool uninitialize() override;
    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_SignalDecimation)

protected:
    size_t m_decimationFactor = 0;
    size_t m_nChannel         = 0;
    size_t m_iSampleIdx       = 0;
    size_t m_iNSamplePerBlock = 0;
    size_t m_iSampling        = 0;
    size_t m_oSampleIdx       = 0;
    size_t m_oNSamplePerBlock = 0;
    size_t m_oSampling        = 0;

    size_t m_nTotalSample    = 0;
    uint64_t m_startTimeBase = 0;
    uint64_t m_lastStartTime = 0;
    uint64_t m_lastEndTime   = 0;

    Kernel::IAlgorithmProxy* m_decoder = nullptr;
    Kernel::TParameterHandler<const CMemoryBuffer*> ip_buffer;
    Kernel::TParameterHandler<CMatrix*> op_pMatrix;
    Kernel::TParameterHandler<uint64_t> op_sampling;

    Kernel::IAlgorithmProxy* m_encoder = nullptr;
    Kernel::TParameterHandler<uint64_t> ip_sampling;
    Kernel::TParameterHandler<CMatrix*> ip_pMatrix;
    Kernel::TParameterHandler<CMemoryBuffer*> op_buffer;
};

class CBoxAlgorithmSignalDecimationDesc final : public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Signal Decimation"; }
    CString getAuthorName() const override { return "Yann Renard"; }
    CString getAuthorCompanyName() const override { return "Inria"; }
    CString getShortDescription() const override { return "Reduces the sampling frequency to a divider of the original sampling frequency"; }

    CString getDetailedDescription() const override
    {
        return "No pre filtering applied - Number of samples per block have to be a multiple of the decimation factor";
    }

    CString getCategory() const override { return "Signal processing/Temporal Filtering"; }
    CString getVersion() const override { return "1.0"; }

    CIdentifier getCreatedClass() const override { return Box_SignalDecimation; }
    IPluginObject* create() override { return new CBoxAlgorithmSignalDecimation; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Input signal", OV_TypeId_Signal);
        prototype.addOutput("Output signal", OV_TypeId_Signal);
        prototype.addSetting("Decimation factor", OV_TypeId_Integer, "8");
        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_SignalDecimationDesc)
};
}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
