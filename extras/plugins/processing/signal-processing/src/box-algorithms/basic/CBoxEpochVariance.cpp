///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxEpochVariance.cpp
/// \brief Definition of Classes of the box Epoch variance.
/// \author Dieter Devlaminck & Thibaut Monseigne (Inria).
/// \version 1.0.
/// \date 11/11/2021.
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

#include "CBoxEpochVariance.hpp"

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {

bool CBoxEpochVariance::initialize()
{
    // ---------- Input/Outputs ----------
    CIdentifier type;
    getStaticBoxContext().getInputType(0, type);
    if (type == OV_TypeId_Signal) {
        m_decoder           = new Toolkit::TSignalDecoder<CBoxEpochVariance>(*this, 0);
        m_encoderAverage    = new Toolkit::TSignalEncoder<CBoxEpochVariance>(*this, 0);
        m_encoderVariance   = new Toolkit::TSignalEncoder<CBoxEpochVariance>(*this, 1);
        m_encoderConfidence = new Toolkit::TSignalEncoder<CBoxEpochVariance>(*this, 2);
    }
    else if (type == OV_TypeId_StreamedMatrix) {
        m_decoder           = new Toolkit::TStreamedMatrixDecoder<CBoxEpochVariance>(*this, 0);
        m_encoderAverage    = new Toolkit::TStreamedMatrixEncoder<CBoxEpochVariance>(*this, 0);
        m_encoderVariance   = new Toolkit::TStreamedMatrixEncoder<CBoxEpochVariance>(*this, 1);
        m_encoderConfidence = new Toolkit::TStreamedMatrixEncoder<CBoxEpochVariance>(*this, 2);
    }
    else if (type == OV_TypeId_Spectrum) {
        m_decoder           = new Toolkit::TSpectrumDecoder<CBoxEpochVariance>(*this, 0);
        m_encoderAverage    = new Toolkit::TSpectrumEncoder<CBoxEpochVariance>(*this, 0);
        m_encoderVariance   = new Toolkit::TSpectrumEncoder<CBoxEpochVariance>(*this, 1);
        m_encoderConfidence = new Toolkit::TSpectrumEncoder<CBoxEpochVariance>(*this, 2);
    }
    else if (type == OV_TypeId_FeatureVector) {
        m_decoder           = new Toolkit::TFeatureVectorDecoder<CBoxEpochVariance>(*this, 0);
        m_encoderAverage    = new Toolkit::TFeatureVectorEncoder<CBoxEpochVariance>(*this, 0);
        m_encoderVariance   = new Toolkit::TFeatureVectorEncoder<CBoxEpochVariance>(*this, 1);
        m_encoderConfidence = new Toolkit::TFeatureVectorEncoder<CBoxEpochVariance>(*this, 2);
    }
    else { return false; }

    // Links
    if (type == OV_TypeId_Signal || type == OV_TypeId_Spectrum) {
        m_encoderAverage.getInputSamplingRate().setReferenceTarget(m_decoder.getOutputSamplingRate());
        m_encoderVariance.getInputSamplingRate().setReferenceTarget(m_decoder.getOutputSamplingRate());
        m_encoderConfidence.getInputSamplingRate().setReferenceTarget(m_decoder.getOutputSamplingRate());

        if (type == OV_TypeId_Spectrum) {
            m_encoderAverage.getInputFrequencyAbcissa().setReferenceTarget(m_decoder.getOutputFrequencyAbcissa());
            m_encoderVariance.getInputFrequencyAbcissa().setReferenceTarget(m_decoder.getOutputFrequencyAbcissa());
            m_encoderConfidence.getInputFrequencyAbcissa().setReferenceTarget(m_decoder.getOutputFrequencyAbcissa());
        }
    }

    m_iMatrix           = m_decoder.getOutputMatrix();
    m_oMatrixAverage    = m_encoderAverage.getInputMatrix();
    m_oMatrixVariance   = m_encoderVariance.getInputMatrix();
    m_oMatrixConfidence = m_encoderConfidence.getInputMatrix();

    // ---------- Settings ----------

    const EEpochAverageMethod averagingMethod = EEpochAverageMethod(uint64_t(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0)));
    const size_t matrixCount                  = uint64_t(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1));
    const double significanceLevel            = double(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2));

    m_variance.initialize(averagingMethod, matrixCount, significanceLevel);

    if (matrixCount <= 0) {
        getLogManager() << Kernel::LogLevel_Error << "You should provide a positive number of epochs better than " << matrixCount << "\n";
        return false;
    }

    return true;
}

bool CBoxEpochVariance::uninitialize()
{
    m_decoder.uninitialize();
    m_encoderAverage.uninitialize();
    m_encoderVariance.uninitialize();
    m_encoderConfidence.uninitialize();
    return true;
}

bool CBoxEpochVariance::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

bool CBoxEpochVariance::process()
{
    Kernel::IBoxIO& boxCtx = getDynamicBoxContext();

    for (size_t i = 0; i < boxCtx.getInputChunkCount(0); ++i) {
        m_decoder.decode(i);
        const uint64_t start = boxCtx.getInputChunkStartTime(0, i), end = boxCtx.getInputChunkEndTime(0, i);

        if (m_decoder.isHeaderReceived()) {
            m_variance.processHeader(m_iMatrix->getBufferElementCount());

            m_oMatrixAverage->copyDescription(*m_iMatrix);
            m_oMatrixVariance->copyDescription(*m_iMatrix);
            m_oMatrixConfidence->copyDescription(*m_iMatrix);

            m_encoderAverage.encodeHeader();
            m_encoderVariance.encodeHeader();
            m_encoderConfidence.encodeHeader();

            boxCtx.markOutputAsReadyToSend(0, start, end);
            boxCtx.markOutputAsReadyToSend(1, start, end);
            boxCtx.markOutputAsReadyToSend(2, start, end);
        }
        if (m_decoder.isBufferReceived()) {
            if (m_variance.process(m_iMatrix, m_oMatrixAverage, m_oMatrixVariance, m_oMatrixConfidence)) {
                m_encoderAverage.encodeBuffer();
                m_encoderVariance.encodeBuffer();
                m_encoderConfidence.encodeBuffer();
                boxCtx.markOutputAsReadyToSend(0, start, end);
                boxCtx.markOutputAsReadyToSend(1, start, end);
                boxCtx.markOutputAsReadyToSend(2, start, end);
            }
        }
        if (m_decoder.isEndReceived()) {
            m_encoderAverage.encodeEnd();
            m_encoderVariance.encodeEnd();
            m_encoderConfidence.encodeEnd();

            boxCtx.markOutputAsReadyToSend(0, start, end);
            boxCtx.markOutputAsReadyToSend(1, start, end);
            boxCtx.markOutputAsReadyToSend(2, start, end);
        }
    }

    return true;
}
}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
