///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmSignalResampling.cpp
/// \brief Classes implementation for the Box Signal Resampling.
/// \author Quentin Barthelemy (Mensia Technologies).
/// \version 2.0.
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

// UG fork: the errors about settings and files here say what is wrong and what to do, politely. See UG.md, 2.23.
#include "CBoxAlgorithmSignalResampling.hpp"

#include <iostream>

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {

namespace SigProSTD {
template <typename T, typename std::enable_if<std::is_integral<T>::value>::type* = nullptr,
          typename std::enable_if<std::is_unsigned<T>::value>::type* = nullptr>
T gcd(T a, T b)
{
    T t;

    if (a > b) // ensure b > a
    {
        t = b;
        b = a;
        a = t;
    }

    while (b != 0) {
        t = a % b;
        a = b;
        b = t;
    }

    return a;
}
}  // namespace SigProSTD

bool CBoxAlgorithmSignalResampling::initialize()
{
    m_decoder.initialize(*this, 0);
    m_encoder.initialize(*this, 0);

    const int64_t oSampling  = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), SignalResampling_SettingId_NewSampling);
    const int64_t nOutSample = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), SignalResampling_SettingId_SampleCountPerBuffer);

    OV_ERROR_UNLESS_KRF(oSampling > 0, "this box's setting \"New Sampling Frequency\" is " << oSampling << ", while it must be above 0. Would you be so very kind as to check it? Thank you for your patience.", Kernel::ErrorType::BadSetting);
    OV_ERROR_UNLESS_KRF(nOutSample > 0, "this box's setting \"Sample Count Per Buffer\" is " << nOutSample << ", while it must be above 0. Would you be so very kind as to check it? Thank you for your patience.", Kernel::ErrorType::BadSetting);

    m_oSampling = size_t(oSampling);
    m_oNSample  = size_t(nOutSample);

    m_nFractionalDelayFilterSample = 6;
    m_transitionBandPercent        = 45;
    m_stopBandAttenuation          = 49;

    m_iSampling = 0;

    m_encoder.getInputSamplingRate() = uint64_t(m_oSampling);

    return true;
}

bool CBoxAlgorithmSignalResampling::uninitialize()
{
    m_decoder.uninitialize();
    m_encoder.uninitialize();
    return true;
}

bool CBoxAlgorithmSignalResampling::processInput(const size_t /*index*/)
{
    this->getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

bool CBoxAlgorithmSignalResampling::process()
{
    m_boxContext = &this->getDynamicBoxContext();

    for (size_t i = 0; i < m_boxContext->getInputChunkCount(0); ++i) {
        m_decoder.decode(i);

        CMatrix* iMatrix = m_decoder.getOutputMatrix();
        CMatrix* oMatrix = m_encoder.getInputMatrix();

        const size_t nChannel = iMatrix->getDimensionSize(0);
        const size_t nSample  = iMatrix->getDimensionSize(1);

        if (m_decoder.isHeaderReceived()) {
            m_iSampling = size_t(m_decoder.getOutputSamplingRate());

            OV_ERROR_UNLESS_KRF(m_iSampling > 0, "Invalid input sampling rate [" << m_iSampling << "] (expected value > 0)", Kernel::ErrorType::BadInput);

            this->getLogManager() << Kernel::LogLevel_Info << "Resampling from [" << m_iSampling << "] Hz to [" << m_oSampling << "] Hz.\n";

            double src                = double(m_oSampling) / double(m_iSampling);
            const size_t gcd          = size_t(SigProSTD::gcd(m_iSampling, m_oSampling));
            size_t factorUpsampling   = m_oSampling / gcd;
            size_t factorDownsampling = m_iSampling / gcd;
            if (src <= 0.5 || src > 1.0) {
                this->getLogManager() << Kernel::LogLevel_Info << "Sampling rate conversion [" << src << "] : upsampling by a factor of [" << factorUpsampling
                        << "], low-pass filtering, and downsampling by a factor of [" << factorDownsampling << "].\n";
            }
            else {
                OV_WARNING_K("Sampling rate conversion [" << src << "] : upsampling by a factor of [" << factorUpsampling
                    << "], low-pass filtering, and downsampling by a factor of [" << factorDownsampling << "]");
            }

            m_resampler.setFractionalDelayFilterSampleCount(m_nFractionalDelayFilterSample);
            m_resampler.setTransitionBand(m_transitionBandPercent);
            m_resampler.setStopBandAttenuation(m_stopBandAttenuation);
            m_resampler.reset(nChannel, m_iSampling, m_oSampling);

            double builtInLatency = m_resampler.getBuiltInLatency();
            if (builtInLatency <= 0.15) {
                this->getLogManager() << Kernel::LogLevel_Trace << "Latency induced by the resampling is [" << builtInLatency << "] s.\n";
            }
            else if (0.15 < builtInLatency && builtInLatency <= 0.5) {
                this->getLogManager() << Kernel::LogLevel_Info << "Latency induced by the resampling is [" << builtInLatency << "] s.\n";
            }
            else if (0.5 < builtInLatency) { OV_WARNING_K("Latency induced by the resampling is [" << builtInLatency << "] s."); }

            oMatrix->copyDescription(*iMatrix);
            oMatrix->setDimensionSize(1, m_oNSample);

            m_oTotalSample = 0;

            m_encoder.encodeHeader();
            m_boxContext->markOutputAsReadyToSend(0, 0, 0);
        }
        if (m_decoder.isBufferReceived()) {
            // re-sampling sample-wise via a callback
            m_resampler.resample(*this, iMatrix->getBuffer(), nSample);
            //this->getLogManager() << Kernel::LogLevel_Info << "count = " << count << ".\n";
            // encoding made in the callback (see next function)
        }
        if (m_decoder.isEndReceived()) {
            m_encoder.encodeEnd();
            m_boxContext->markOutputAsReadyToSend(0, (uint64_t((m_oTotalSample % m_oNSample) << 32) / m_oSampling),
                                                  (uint64_t((m_oTotalSample % m_oNSample) << 32) / m_oSampling));
        }
    }

    return true;
}

void CBoxAlgorithmSignalResampling::processResampler(const double* sample, const size_t nChannel) const
{
    double* buffer            = m_encoder.getInputMatrix()->getBuffer();
    const uint64_t oSampleIdx = m_oTotalSample % m_oNSample;

    for (size_t j = 0; j < nChannel; ++j) { buffer[j * m_oNSample + oSampleIdx] = sample[j]; }
    m_oTotalSample++;

    if ((m_oTotalSample % m_oNSample) == 0) {
        m_encoder.encodeBuffer();
        m_boxContext->markOutputAsReadyToSend(0, (uint64_t((m_oTotalSample - m_oNSample) << 32) / m_oSampling),
                                              (uint64_t((m_oTotalSample) << 32) / m_oSampling));
    }
}

}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
