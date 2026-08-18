///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmStreamedMatrixSwitch.cpp
/// \author Laurent Bonnet (Inria)
/// \version 1.1.
/// \date 12/05/2011.
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

#include "CBoxAlgorithmStreamedMatrixSwitch.hpp"

namespace OpenViBE {
namespace Plugins {
namespace Streaming {


///-------------------------------------------------------------------------------------------------
bool CBoxAlgorithmStreamedMatrixSwitch::initialize()
{
    // Getting the settings to build the map Stim code / output index
    for (size_t i = 1; i < this->getStaticBoxContext().getSettingCount(); ++i) {
        const uint64_t stimCode = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), i);
        const size_t idx        = i - 1;
        if (!m_stimOutputIndexes.insert(std::make_pair(stimCode, idx)).second) {
            this->getLogManager() << Kernel::LogLevel_Warning << "The stimulation code ["
                    << this->getTypeManager().getEnumerationEntryNameFromValue(OV_TypeId_Stimulation, stimCode) << "] for the output ["
                    << idx << "] is already used by a previous output.\n";
        }
        else {
            this->getLogManager() << Kernel::LogLevel_Trace << "The stimulation code ["
                    << this->getTypeManager().getEnumerationEntryNameFromValue(OV_TypeId_Stimulation, stimCode) << "] is registered for the output ["
                    << idx << "]\n";
        }
    }

    const bool defaultToFirstOutput = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
    if (defaultToFirstOutput) { m_activeOutputIdx = 0; }
    else { m_activeOutputIdx = size_t(-1); }    // At start, no output is active.

    // Stimulation stream decoder
    m_stimDecoder.initialize(*this, 0);
    m_lastStimInputChunkEndTime = 0;

    //initializing the decoder depending on the input type.
    CIdentifier typeID;
    this->getStaticBoxContext().getInputType(1, typeID);

    m_streamDecoder = nullptr;

    if (typeID == OV_TypeId_StreamedMatrix) { m_streamDecoder = new Toolkit::TStreamedMatrixDecoder<CBoxAlgorithmStreamedMatrixSwitch>(*this, 1); }
    else if (typeID == OV_TypeId_Signal) { m_streamDecoder = new Toolkit::TSignalDecoder<CBoxAlgorithmStreamedMatrixSwitch>(*this, 1); }
    else if (typeID == OV_TypeId_Spectrum) { m_streamDecoder = new Toolkit::TSpectrumDecoder<CBoxAlgorithmStreamedMatrixSwitch>(*this, 1); }
    else if (typeID == OV_TypeId_FeatureVector) { m_streamDecoder = new Toolkit::TFeatureVectorDecoder<CBoxAlgorithmStreamedMatrixSwitch>(*this, 1); }
    else if (typeID == OV_TypeId_ChannelLocalisation) { m_streamDecoder = new Toolkit::TChannelLocalisationDecoder<CBoxAlgorithmStreamedMatrixSwitch>(*this, 1); }
    else if (typeID == OV_TypeId_Stimulations) { m_streamDecoder = new Toolkit::TStimulationDecoder<CBoxAlgorithmStreamedMatrixSwitch>(*this, 1); }
    else if (typeID == OV_TypeId_ExperimentInfo) { m_streamDecoder = new Toolkit::TExperimentInfoDecoder<CBoxAlgorithmStreamedMatrixSwitch>(*this, 1); }
    else if (typeID == OV_TypeId_ChannelUnits) { m_streamDecoder = new Toolkit::TChannelUnitsDecoder<CBoxAlgorithmStreamedMatrixSwitch>(*this, 1); }
    else {
        this->getLogManager() << Kernel::LogLevel_Error << "Unsupported stream type " << this->getTypeManager().getTypeName(typeID) << " (" << typeID.str() << ")\n";
        return false;
    }

    return true;
}

///-------------------------------------------------------------------------------------------------
bool CBoxAlgorithmStreamedMatrixSwitch::uninitialize()
{
    m_stimDecoder.uninitialize();
    if (m_streamDecoder) {
        m_streamDecoder->uninitialize();
        delete m_streamDecoder;
    }

    return true;
}

///-------------------------------------------------------------------------------------------------
bool CBoxAlgorithmStreamedMatrixSwitch::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

///-------------------------------------------------------------------------------------------------
bool CBoxAlgorithmStreamedMatrixSwitch::process()
{
    // the dynamic box context describes the current state of the box inputs and outputs (i.e. the chunks)
    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();

    const size_t nOutput  = this->getStaticBoxContext().getOutputCount();
    uint64_t start        = 0;
    uint64_t end          = 0;
    size_t chunkSize      = 0;
    bool gotStimulation   = false;
    const uint8_t* buffer = nullptr;

    //iterate over all chunk on input 0 (Stimulation)
    for (size_t i = 0; i < boxContext.getInputChunkCount(0); ++i) {
        m_stimDecoder.decode(i);

        if (m_stimDecoder.isHeaderReceived() || m_stimDecoder.isEndReceived()) { }  // nothing
        if (m_stimDecoder.isBufferReceived()) {
            // we update the active output index and time if needed
            const CStimulationSet* stimSet = m_stimDecoder.getOutputStimulationSet();
            for (size_t j = 0; j < stimSet->size(); j++) {
                if (m_stimOutputIndexes.find(stimSet->getId(j)) != m_stimOutputIndexes.end()) {
                    m_activeOutputIdx = m_stimOutputIndexes[stimSet->getId(j)];
                    this->getLogManager() << Kernel::LogLevel_Trace << "Switching with ["
                            << this->getTypeManager().getEnumerationEntryNameFromValue(OV_TypeId_Stimulation, stimSet->getId(j))
                            << "] to output [" << m_activeOutputIdx << "].\n";
                }
            }
            gotStimulation              = true;
            m_lastStimInputChunkEndTime = boxContext.getInputChunkEndTime(0, i);
        }
    }

    for (size_t i = 0; i < boxContext.getInputChunkCount(1); ++i) {
        //We decode the chunk but we don't automatically mark it as deprecated, as we may need to keep it.
        m_streamDecoder->decode(i, false);
        {
            boxContext.getInputChunk(1, i, start, end, chunkSize, buffer);
            if (m_streamDecoder->isHeaderReceived() || m_streamDecoder->isEndReceived()) {
                for (size_t j = 0; j < nOutput; ++j) {
                    boxContext.appendOutputChunkData(j, buffer, chunkSize);
                    boxContext.markOutputAsReadyToSend(j, start, end);
                }
                boxContext.markInputAsDeprecated(1, i);
            }
            if (m_streamDecoder->isBufferReceived()) {
                // we drop every chunk when no output is activated
                if (m_activeOutputIdx == size_t(-1)) { boxContext.markInputAsDeprecated(1, i); }
                else {
                    if (!gotStimulation || (start < m_lastStimInputChunkEndTime)) {
                        // the input chunk is in the good time range (we are sure that no stim has been received to change the active output)
                        boxContext.appendOutputChunkData(m_activeOutputIdx, buffer, chunkSize);
                        boxContext.markOutputAsReadyToSend(m_activeOutputIdx, start, end);
                        boxContext.markInputAsDeprecated(1, i);
                    }
                    // else : we keep the input chunk, no mark as deprecated !
                }
            }
        }
    }

    return true;
}
}  // namespace Streaming
}  // namespace Plugins
}  // namespace OpenViBE
