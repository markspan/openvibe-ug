///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxInputStreamSwitch.cpp
/// \brief Classes implementation for the box Input Stream Switch.
/// \author Thibaut Monseigne (Inria).
/// \version 1.0.
/// \date 14/10/2022.
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

#include "CBoxInputStreamSwitch.hpp"

namespace OpenViBE {
namespace Plugins {
namespace Streaming {


///-------------------------------------------------------------------------------------------------
bool CBoxInputStreamSwitch::initialize()
{
    // Getting the settings to build the map Stim code / input index
    for (size_t i = 1; i < this->getStaticBoxContext().getSettingCount(); ++i) {
        const uint64_t stimCode = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), i);
        const size_t idx        = i;
        if (!m_stimInputIndexes.insert(std::make_pair(stimCode, idx)).second) {
            this->getLogManager() << Kernel::LogLevel_Warning << "The stimulation code ["
                    << this->getTypeManager().getEnumerationEntryNameFromValue(OV_TypeId_Stimulation, stimCode)
                    << "] for the input [" << idx << "] is already used by a previous input.\n";
        }
        else {
            this->getLogManager() << Kernel::LogLevel_Trace << "The stimulation code ["
                    << this->getTypeManager().getEnumerationEntryNameFromValue(OV_TypeId_Stimulation, stimCode)
                    << "] is registered for the input [" << idx << "]\n";
        }
    }

    const bool defaultToFirstInput = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
    if (defaultToFirstInput) { m_activeInputIdx = 1; }
    else { m_activeInputIdx = size_t(-1); } // At start, no input is active.

    // Stimulation stream decoder
    m_stimDecoder.initialize(*this, 0);

    //initializing the decoder depending on the input type.
    CIdentifier typeID;
    this->getStaticBoxContext().getInputType(1, typeID);

    m_streamDecoder.clear();
    m_streamDecoder.resize(this->getStaticBoxContext().getInputCount() - 1);

    for (size_t i = 1; i < this->getStaticBoxContext().getInputCount(); ++i) {
        if (typeID == OV_TypeId_StreamedMatrix) { m_streamDecoder[i - 1] = new Toolkit::TStreamedMatrixDecoder<CBoxInputStreamSwitch>(*this, i); }
        else if (typeID == OV_TypeId_Signal) { m_streamDecoder[i - 1] = new Toolkit::TSignalDecoder<CBoxInputStreamSwitch>(*this, i); }
        else if (typeID == OV_TypeId_Spectrum) { m_streamDecoder[i - 1] = new Toolkit::TSpectrumDecoder<CBoxInputStreamSwitch>(*this, i); }
        else if (typeID == OV_TypeId_FeatureVector) { m_streamDecoder[i - 1] = new Toolkit::TFeatureVectorDecoder<CBoxInputStreamSwitch>(*this, i); }
        else if (typeID == OV_TypeId_Stimulations) { m_streamDecoder[i - 1] = new Toolkit::TStimulationDecoder<CBoxInputStreamSwitch>(*this, i); }
        else {
            this->getLogManager() << Kernel::LogLevel_Error << "Unsupported type " << this->getTypeManager().getTypeName(typeID) << " (" << typeID << ")\n";
            return false;
        }
    }

    return true;
}

///-------------------------------------------------------------------------------------------------
bool CBoxInputStreamSwitch::uninitialize()
{
    m_stimDecoder.uninitialize();

    for (size_t i = 0; i < this->getStaticBoxContext().getInputCount() - 1; ++i) {
        if (m_streamDecoder[i]) {
            m_streamDecoder[i]->uninitialize();
            delete m_streamDecoder[i];
        }
    }
    m_streamDecoder.clear();

    return true;
}

///-------------------------------------------------------------------------------------------------
bool CBoxInputStreamSwitch::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

///-------------------------------------------------------------------------------------------------
bool CBoxInputStreamSwitch::process()
{
    // the dynamic box context describes the current state of the box inputs and outputs (i.e. the chunks)
    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();

    const size_t nInput   = this->getStaticBoxContext().getInputCount();
    size_t chunkSize      = 0;
    uint64_t start        = 0, end = 0;
    const uint8_t* buffer = nullptr;

    //iterate over all chunk on input 0 (Stimulation)
    for (size_t i = 0; i < boxContext.getInputChunkCount(0); ++i) {
        m_stimDecoder.decode(i);
        if (m_stimDecoder.isBufferReceived()) {
            // we update the active output index and time if needed
            const CStimulationSet* stimSet = m_stimDecoder.getOutputStimulationSet();
            for (size_t j = 0; j < stimSet->size(); j++) {
                if (m_stimInputIndexes.find(stimSet->getId(j)) != m_stimInputIndexes.end()) {
                    m_activeInputIdx = m_stimInputIndexes[stimSet->getId(j)];
                    this->getLogManager() << Kernel::LogLevel_Trace << "Switching with ["
                            << this->getTypeManager().getEnumerationEntryNameFromValue(OV_TypeId_Stimulation, stimSet->getId(j))
                            << "] to output [" << m_activeInputIdx << "].\n";
                }
            }
        }
    }

    for (size_t i = 1; i < nInput; ++i) {
        for (size_t j = 0; j < boxContext.getInputChunkCount(i); ++j) {
            m_streamDecoder[i - 1]->decode(j);
            boxContext.getInputChunk(i, j, start, end, chunkSize, buffer);

            if ((!m_headerSent && m_streamDecoder[i - 1]->isHeaderReceived())
                || (!m_endSent && m_streamDecoder[i - 1]->isEndReceived())
                || (i == m_activeInputIdx && m_streamDecoder[i - 1]->isBufferReceived())) {
                boxContext.appendOutputChunkData(0, buffer, chunkSize);
                boxContext.markOutputAsReadyToSend(0, start, end);
            }

            if (m_streamDecoder[i - 1]->isHeaderReceived()) { m_headerSent = true; }
            if (m_streamDecoder[i - 1]->isEndReceived()) { m_endSent = true; }
        }
    }

    return true;
}
}  // namespace Streaming
}  // namespace Plugins
}  // namespace OpenViBE
