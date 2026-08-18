///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmTemporalFilter.hpp
/// \brief Classes for the Box Temporal Filter.
/// \author Thibaut Monseigne (Inria).
/// \version 2.0.
/// \date 14/10/2021
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

#include <memory>
#include <dsp-filters/Dsp.h>

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {
//--------------------------------------------------------------------------------
/// <summary>   The class CBoxAlgorithmTemporalFilter describes the box Temporal Filter. </summary>
class CBoxAlgorithmTemporalFilter final : public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }
    bool initialize() override;
    bool uninitialize() override;
    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_TemporalFilter)

private:
    Toolkit::TSignalDecoder<CBoxAlgorithmTemporalFilter> m_decoder;                 ///< Decoder for input.
    std::vector<Toolkit::TSignalEncoder<CBoxAlgorithmTemporalFilter>> m_encoders;   ///< Encoders for outputs.
    std::vector<CMatrix*> m_oMatrix;                                                ///< Matrix links with outputs (avoid to relink at each process).

    size_t m_nOutput   = 0;                                                         ///< Number of outputs.
    EFilterType m_type = EFilterType::BandPass;                                     ///< Type of the filters.
    size_t m_order     = 0;                                                         ///< Order of the filters.

    std::vector<std::vector<double>> m_frenquencies;                                ///< List of frequencies for each outputs (Low and High).
    std::vector<Dsp::Params> m_parameters;                                          ///< List of parameters for each outputs.
    std::vector<std::vector<std::shared_ptr<Dsp::Filter>>> m_filters;               ///< List of filters for each outputs and each channels.

    std::vector<double> m_firstSamples;                                             ///< First sample of each channel.

    /// <summary> Sets the filter parameters depending on the Filter type. </summary>
    /// <param name="id">The identifier of the filter (one filter per output).</param>
    void setParameter(const size_t id);

    /// <summary> Creates the filter of the dsp filter library. </summary>
    /// <param name="nSmooth">The smooth of the filter (100*frequency of the signal).</param>
    /// <returns> The pointer of the filter (the object is destroyed when no object have his address (or use delete)). </returns>
    std::shared_ptr<Dsp::Filter> createFilter(const size_t nSmooth) const;
};

//--------------------------------------------------------------------------------
/// <summary> Listener of the box Temporal Filter. </summary>
class CBoxAlgorithmTemporalFilterListener final : public Toolkit::TBoxListener<IBoxListener>
{
    void renameOutputs(Kernel::IBox& box) const
    {
        for (size_t i = 1; i < box.getOutputCount(); ++i) { box.setOutputName(i, ("Output " + std::to_string(i + 1)).c_str()); }
    }

    void changeSettings(Kernel::IBox& box, const bool add) const
    {
        const size_t idx = 2 + 2 * (box.getOutputCount() - 1); // First Setting Idx to Add
        if (add) {
            box.addSetting(settingName(true, box.getOutputCount()).c_str(), OV_TypeId_Float, "1", idx);
            box.addSetting(settingName(false, box.getOutputCount()).c_str(), OV_TypeId_Float, "40", idx + 1);
        }
        else {
            box.removeSetting(idx + 2);
            box.removeSetting(idx + 2);
        }
    }

    std::string settingName(const bool low, const size_t i) const
    {
        return std::string(low ? "Low" : "High") + " Cut-off Frequency " + std::to_string(i) + " (Hz)";
    }

public:
    bool onOutputAdded(Kernel::IBox& box, const size_t index) override
    {
        box.setOutputType(index, OV_TypeId_Signal);
        box.setOutputName(index, ("Output signal " + std::to_string(index + 1)).c_str());
        changeSettings(box, true);
        return true;
    }

    bool onOutputRemoved(Kernel::IBox& box, const size_t /*index*/) override
    {
        renameOutputs(box);
        changeSettings(box, false);
        return true;
    }

    bool onSettingValueChanged(Kernel::IBox& /*box*/, const size_t /*index*/) override { return true; }

    _IsDerivedFromClass_Final_(Toolkit::TBoxListener<IBoxListener>, CIdentifier::undefined())
};

//--------------------------------------------------------------------------------
class CBoxAlgorithmTemporalFilterDesc final : public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Temporal Filter"; }
    CString getAuthorName() const override { return "Yann Renard & Laurent Bonnet, Thibaut Monseigne"; }
    CString getAuthorCompanyName() const override { return "Mensia Technologies, Inria"; }
    CString getShortDescription() const override { return "Temporal filtering based on various one-way IIR filter designs"; }
    CString getDetailedDescription() const override { return "Applies a temporal filter, based on various one-way IIR filter designs, to the input stream."; }
    CString getCategory() const override { return "Signal processing/Temporal Filtering"; }
    CString getVersion() const override { return "2.0"; }

    CIdentifier getCreatedClass() const override { return Box_TemporalFilter; }
    IPluginObject* create() override { return new CBoxAlgorithmTemporalFilter; }

    IBoxListener* createBoxListener() const override { return new CBoxAlgorithmTemporalFilterListener; }
    void releaseBoxListener(IBoxListener* listener) const override { delete listener; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Input signal", OV_TypeId_Signal);
        prototype.addOutput("Output signal", OV_TypeId_Signal);
        prototype.addSetting("Filter Type", TypeId_FilterType, toString(EFilterType::BandPass).c_str());
        prototype.addSetting("Filter Order", OV_TypeId_Integer, "4");
        prototype.addSetting("Low Cut-off Frequency (Hz)", OV_TypeId_Float, "1");
        prototype.addSetting("High Cut-off Frequency (Hz)", OV_TypeId_Float, "40");
        prototype.addFlag(Kernel::BoxFlag_CanAddOutput);

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_TemporalFilterDesc)
};
}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
