///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmChannelRename.cpp
/// \brief Classes implementation for the Box Channel Rename.
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

// UG fork: the errors about settings and files here say what is wrong and what to do, politely. See UG.md, 2.23.
#include "CBoxAlgorithmChannelRename.hpp"

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {

bool CBoxAlgorithmChannelRename::initialize()
{
    std::vector<CString> tokens;
    const CString setting = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
    const size_t nToken   = split(setting, Toolkit::String::TSplitCallback<std::vector<CString>>(tokens), OV_Value_EnumeratedStringSeparator);

    m_names.clear();
    for (size_t i = 0; i < nToken; ++i) { m_names.push_back(tokens[i].toASCIIString()); }

    this->getStaticBoxContext().getOutputType(0, m_typeID);

    if (m_typeID == OV_TypeId_Signal) {
        m_decoder = new Toolkit::TSignalDecoder<CBoxAlgorithmChannelRename>(*this, 0);
        m_encoder = new Toolkit::TSignalEncoder<CBoxAlgorithmChannelRename>(*this, 0);
    }
    else if (m_typeID == OV_TypeId_StreamedMatrix || m_typeID == OV_TypeId_CovarianceMatrix || m_typeID == OV_TypeId_TimeFrequency) {
        m_decoder = new Toolkit::TStreamedMatrixDecoder<CBoxAlgorithmChannelRename>(*this, 0);
        m_encoder = new Toolkit::TStreamedMatrixEncoder<CBoxAlgorithmChannelRename>(*this, 0);
    }
    else if (m_typeID == OV_TypeId_Spectrum) {
        m_decoder = new Toolkit::TSpectrumDecoder<CBoxAlgorithmChannelRename>(*this, 0);
        m_encoder = new Toolkit::TSpectrumEncoder<CBoxAlgorithmChannelRename>(*this, 0);
    }
    else { OV_ERROR_KRF("this box cannot rename the channels of a stream of the type " << m_typeID.str() << ", only of signal, matrix, covariance matrix, time-frequency and spectrum streams. Would you be so very kind as to choose another type for its input? Thank you for your patience.", Kernel::ErrorType::BadConfig); }

    ip_Matrix = m_encoder.getInputMatrix();
    op_Matrix = m_decoder.getOutputMatrix();

    m_encoder.getInputMatrix().setReferenceTarget(m_decoder.getOutputMatrix());

    if (m_typeID == OV_TypeId_Signal) { m_encoder.getInputSamplingRate().setReferenceTarget(m_decoder.getOutputSamplingRate()); }

    if (m_typeID == OV_TypeId_Spectrum) {
        m_encoder.getInputSamplingRate().setReferenceTarget(m_decoder.getOutputSamplingRate());
        m_encoder.getInputFrequencyAbcissa().setReferenceTarget(m_decoder.getOutputFrequencyAbcissa());
    }

    return true;
}

bool CBoxAlgorithmChannelRename::uninitialize()
{
    m_decoder.uninitialize();
    m_encoder.uninitialize();

    return true;
}

bool CBoxAlgorithmChannelRename::processInput(const size_t /*index*/)
{
    this->getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

bool CBoxAlgorithmChannelRename::process()
{
    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();

    for (size_t chunk = 0; chunk < boxContext.getInputChunkCount(0); ++chunk) {
        m_decoder.decode(chunk);
        if (m_decoder.isHeaderReceived()) {
            ip_Matrix->copyDescription(*op_Matrix);
            for (size_t channel = 0; channel < ip_Matrix->getDimensionSize(0) && channel < m_names.size(); ++channel) {
                ip_Matrix->setDimensionLabel(0, channel, m_names[channel].c_str());
            }
            m_encoder.encodeHeader();
        }
        if (m_decoder.isBufferReceived()) { m_encoder.encodeBuffer(); }
        if (m_decoder.isEndReceived()) { m_encoder.encodeEnd(); }

        boxContext.markOutputAsReadyToSend(0, boxContext.getInputChunkStartTime(0, chunk), boxContext.getInputChunkEndTime(0, chunk));
        boxContext.markInputAsDeprecated(0, chunk);
    }

    return true;
}

}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
