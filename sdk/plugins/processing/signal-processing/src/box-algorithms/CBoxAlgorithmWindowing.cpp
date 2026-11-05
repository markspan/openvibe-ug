///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmWindowing.cpp
/// \brief Classes implementation for the Box Windowing.
/// \author Laurent Bonnet (Mensia Technologies).
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

// UG fork: the errors about settings and files here say what is wrong and what to do, politely. See UG.md, 2.23.
#include "CBoxAlgorithmWindowing.hpp"
#include <cmath>

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {

bool CBoxAlgorithmWindowing::initialize()
{
    //reads the plugin settings
    m_windowMethod = EWindowMethod(uint64_t(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0)));

    if (m_windowMethod != EWindowMethod::None && m_windowMethod != EWindowMethod::Hamming && m_windowMethod != EWindowMethod::Hanning
        && m_windowMethod != EWindowMethod::Hann && m_windowMethod != EWindowMethod::Blackman && m_windowMethod != EWindowMethod::Triangular
        && m_windowMethod != EWindowMethod::SquareRoot) { OV_ERROR_KRF("this box's setting \"Window method\" holds a method we regret to say we do not know. Would you be so very kind as to choose another? Thank you for your patience.", Kernel::ErrorType::BadSetting); }

    m_decoder.initialize(*this, 0);
    m_encoder.initialize(*this, 0);
    m_encoder.getInputMatrix().setReferenceTarget(m_decoder.getOutputMatrix());
    m_encoder.getInputSamplingRate().setReferenceTarget(m_decoder.getOutputSamplingRate());

    return true;
}

bool CBoxAlgorithmWindowing::uninitialize()
{
    m_decoder.uninitialize();
    m_encoder.uninitialize();

    return true;
}

bool CBoxAlgorithmWindowing::processInput(const size_t /*index*/)
{
    this->getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

bool CBoxAlgorithmWindowing::process()
{
    Kernel::IBoxIO* boxContext = getBoxAlgorithmContext()->getDynamicBoxContext();

    // Process input data
    for (size_t i = 0; i < boxContext->getInputChunkCount(0); ++i) {
        const uint64_t startTime = boxContext->getInputChunkStartTime(0, i);
        const uint64_t endTime   = boxContext->getInputChunkEndTime(0, i);

        m_decoder.decode(i);
        CMatrix* matrix = m_decoder.getOutputMatrix();

        if (m_decoder.isHeaderReceived()) {
            /*
             * Depending on the Window method, we compute the coefficient vector
             * To be applied on each channel.
             */
            m_windowCoefs.resize(matrix->getDimensionSize(1));
            const size_t n = m_windowCoefs.size();

            if (m_windowMethod == EWindowMethod::Hamming) {
                for (size_t k = 0; k < n; ++k) { m_windowCoefs[k] = 0.54 - 0.46 * cos(2. * M_PI * double(k) / (double(n) - 1.)); }
            }
            else if (m_windowMethod == EWindowMethod::Hann || m_windowMethod == EWindowMethod::Hanning) {
                for (size_t k = 0; k < n; ++k) { m_windowCoefs[k] = 0.5 * (1. - cos(2. * M_PI * double(k) / (double(n) - 1.))); }
            }
            else if (m_windowMethod == EWindowMethod::Blackman) {
                for (size_t k = 0; k < n; ++k) {
                    m_windowCoefs[k] = 0.42 - 0.5 * cos(2. * M_PI * double(k) / (double(n) - 1.)) + 0.08 * cos(4. * M_PI * double(k) / (double(n) - 1.));
                }
            }
            else if (m_windowMethod == EWindowMethod::Triangular) {
                /* from MATLAB implementation, as ITPP documentation seems to be flawed */
                for (size_t k = 1; k <= (n + 1) / 2; ++k) {
                    if (n % 2 == 1) { m_windowCoefs[k - 1] = double((2. * double(k)) / (double(n) + 1.)); }
                    else { m_windowCoefs[k - 1] = double((2. * double(k) - 1.) / double(n)); }
                }

                for (size_t k = n / 2 + 1; k <= n; ++k) {
                    if (n % 2 == 1) { m_windowCoefs[k - 1] = double(2. - (2. * double(k)) / (double(n) + 1.)); }
                    else { m_windowCoefs[k - 1] = double(2. - (2. * double(k) - 1.) / double(n)); }
                }
            }
            else if (m_windowMethod == EWindowMethod::SquareRoot) {
                for (size_t k = 1; k <= (n + 1) / 2; ++k) {
                    if (n % 2 == 1) { m_windowCoefs[k - 1] = sqrt(2. * double(k) / (double(n) + 1.)); }
                    else { m_windowCoefs[k - 1] = sqrt((2. * double(k) - 1.) / double(n)); }
                }

                for (size_t k = n / 2 + 1; k <= n; ++k) {
                    if (n % 2 == 1) { m_windowCoefs[k - 1] = sqrt(2. - (2. * double(k)) / (double(n) + 1.)); }
                    else { m_windowCoefs[k - 1] = sqrt(2. - (2. * double(k) - 1.) / double(n)); }
                }
            }
            else if (m_windowMethod == EWindowMethod::None) { for (size_t k = 0; k < n; ++k) { m_windowCoefs[k] = 1; } }
            else { OV_ERROR_KRF("this box's setting \"Window method\" holds a method we regret to say we do not know. Would you be so very kind as to choose another? Thank you for your patience.", Kernel::ErrorType::BadSetting); }

            m_encoder.encodeHeader();
        }

        if (m_decoder.isBufferReceived()) {
            /* We filter each channel with the window function */
            for (size_t j = 0; j < matrix->getDimensionSize(0); ++j) // channels
            {
                for (size_t k = 0; k < matrix->getDimensionSize(1); ++k) // samples
                {
                    matrix->getBuffer()[j * matrix->getDimensionSize(1) + k] *= m_windowCoefs[k];
                }
            }

            m_encoder.encodeBuffer();
        }

        if (m_decoder.isEndReceived()) { m_encoder.encodeEnd(); }

        boxContext->markOutputAsReadyToSend(0, startTime, endTime);
    }

    return true;
}

}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
