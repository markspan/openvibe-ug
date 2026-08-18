///-------------------------------------------------------------------------------------------------
///
/// \file ovpCBoxAlgorithmARCoefficients.h
/// \brief Implementation of the box ARCoefficients
/// \author Alison Cellard / Arthur DESBOIS (Inria).
/// \version 0.0.1.
/// \date June 28 15:13:00 2022.
///
/// \copyright Copyright (C) 2013-2022 INRIA
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
///-------------------------------------------------------------------------------------------------
#pragma once

//You may have to change this path to match your folder organisation
#include "defines.hpp"

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>
#include <Eigen/Dense>

#define OV_AttributeId_Box_FlagIsUnstable               OpenViBE::CIdentifier(0x666FFFFF, 0x666FFFFF)

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {
/**
 * \class CBoxAlgorithmARCoefficients
 * \author Alison Cellard (Inria)
 * \date Wed Nov 28 10:40:52 2012
 * \brief The class CBoxAlgorithmARCoefficients describes the box AR Features.
 *
 */
class CBoxAlgorithmARCoefficients final : virtual public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;

    // Process callbacks on new input received
    bool processInput(const size_t index) override;
    bool process() override;

    // As we do with any class in openvibe, we use the macro below 
    // to associate this box to an unique identifier. 
    // The inheritance information is also made available, 
    // as we provide the superclass Toolkit::TBoxAlgorithm < IBoxAlgorithm >
    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, OVP_ClassId_BoxAlgorithm_ARCoefficients)

protected:
    // Codecs
    Toolkit::TSignalDecoder<CBoxAlgorithmARCoefficients> m_signalDecoder;
    Toolkit::TStreamedMatrixEncoder<CBoxAlgorithmARCoefficients> m_arEncoder;
    Toolkit::TSpectrumEncoder<CBoxAlgorithmARCoefficients> m_spectrumEncoder;

    // Channel samples for interfacing with algorithm
    std::vector< Eigen::VectorXd > m_samplesBuffer;
    std::vector <Eigen::VectorXd> m_arCoeffs;
    std::vector <Eigen::VectorXd> m_arPsd;

    // Settings
    uint64_t m_order = 1;
    uint64_t m_fftSize = 256;
    bool m_detrend = true;

private:
    CMatrix* m_iMatrix = nullptr;
    CMatrix* m_oArMatrix = nullptr;
    CMatrix* m_oSpectrum = nullptr;

    uint64_t m_sampRate = 512;
    size_t   m_nbChannels = 1;
    CMatrix* m_frequencyAbscissa;
};


/**
 * \class CBoxAlgorithmARCoefficientsDesc
 * \author Alison Cellard (Inria)
 * \date Wed Nov 28 10:40:52 2012
 * \brief Descriptor of the box AR Features.
 *
 */
class CBoxAlgorithmARCoefficientsDesc final : virtual public IBoxAlgorithmDesc
{
public:

    void release() override { }

    CString getName() const override { return CString("AutoRegressive Coefficients"); }
    CString getAuthorName() const override { return CString("Alison Cellard / Arthur Desbois"); }
    CString getAuthorCompanyName() const override { return CString("Inria"); }

    CString getShortDescription() const override { return CString("Estimates autoregressive (AR) coefficients from a set of signals"); }

    CString getDetailedDescription() const override { return CString("Estimates autoregressive (AR) linear model coefficients using Burg's method"); }

    CString getCategory() const override { return CString("Signal processing/Basic"); }
    CString getVersion() const override { return CString("1.0"); }
    CString getStockItemName() const override { return CString(""); }
    CIdentifier getCreatedClass() const override { return OVP_ClassId_BoxAlgorithm_ARCoefficients; }
    IPluginObject* create() override { return new CBoxAlgorithmARCoefficients; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("EEG Signal",OV_TypeId_Signal);
        prototype.addOutput("AR Features",OV_TypeId_StreamedMatrix);
        prototype.addOutput("PSD",OV_TypeId_Spectrum);

        prototype.addSetting("Order",OV_TypeId_Integer, "1");
        prototype.addSetting("PSD size", OV_TypeId_Integer, "256");
        prototype.addSetting("Detrend / DC removal", OV_TypeId_Boolean, "True");

        prototype.addFlag(OV_AttributeId_Box_FlagIsUnstable);

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, OVP_ClassId_BoxAlgorithm_ARCoefficientsDesc)
};
}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE


