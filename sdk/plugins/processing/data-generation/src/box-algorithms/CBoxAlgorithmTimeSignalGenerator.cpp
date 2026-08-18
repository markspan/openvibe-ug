///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmTimeSignalGenerator.cpp
/// \brief Classes implementation for the Box Time signal.
/// \author Yann Renard (Inria).
/// \version 1.1.
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

#include "CBoxAlgorithmTimeSignalGenerator.hpp"

namespace OpenViBE {
namespace Plugins {
namespace DataGeneration {

bool CBoxAlgorithmTimeSignalGenerator::initialize()
{
    m_encoder.initialize(*this, 0);

    // Parses box settings to try connecting to server
    m_sampling              = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
    m_nGeneratedEpochSample = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);
    m_headerSent            = false;

    m_nSentSample = 0;

    return true;
}

bool CBoxAlgorithmTimeSignalGenerator::uninitialize()
{
    m_encoder.uninitialize();
    return true;
}

bool CBoxAlgorithmTimeSignalGenerator::processClock(Kernel::CMessageClock& /*msg*/)
{
    this->getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

bool CBoxAlgorithmTimeSignalGenerator::process()
{
    Kernel::IBoxIO* boxContext = getBoxAlgorithmContext()->getDynamicBoxContext();

    if (!m_headerSent) {
        m_encoder.getInputSamplingRate() = m_sampling;

        CMatrix* matrix = m_encoder.getInputMatrix();

        matrix->resize(1, m_nGeneratedEpochSample);
        matrix->setDimensionLabel(0, 0, "Time signal");

        m_encoder.encodeHeader();

        m_headerSent = true;

        boxContext->markOutputAsReadyToSend(0, 0, 0);
    }
    else {
        // Create sample chunks up until the next step (current time + 1/128) but do not overshoot it
        // This way we will always create the correct number of samples for frequencies that are above 128Hz
        const uint64_t nextStepDate = CTime(uint64_t(this->getPlayerContext().getCurrentTime() + (1ULL << 25))).toSampleCount(m_sampling);
        while (m_nSentSample + m_nGeneratedEpochSample < nextStepDate) {
            double* buffer = m_encoder.getInputMatrix()->getBuffer();

            for (size_t i = 0; i < m_nGeneratedEpochSample; ++i) { buffer[i] = double(i + m_nSentSample) / double(m_sampling); }

            m_encoder.encodeBuffer();

            const uint64_t tStart = CTime(m_sampling, m_nSentSample).time();
            m_nSentSample += m_nGeneratedEpochSample;
            const uint64_t tEnd = CTime(m_sampling, m_nSentSample).time();

            boxContext->markOutputAsReadyToSend(0, tStart, tEnd);
        }
    }

    return true;
}

}  // namespace DataGeneration
}  // namespace Plugins
}  // namespace OpenViBE
