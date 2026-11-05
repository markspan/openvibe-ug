///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmPlayerController.cpp
/// \brief Classes implementation for the Box Player Controller.
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

// UG fork: the errors about settings and files here say what is wrong and what to do, politely. See UG.md, 2.23.
#include "CBoxAlgorithmPlayerController.hpp"

namespace OpenViBE {
namespace Plugins {
namespace Stimulation {

///-------------------------------------------------------------------------------------------------
bool CBoxAlgorithmPlayerController::initialize()
{
    m_stimulationID = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
    m_actionID      = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);

    m_decoder = &this->getAlgorithmManager().getAlgorithm(this->getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_StimulationDecoder));
    m_decoder->initialize();

    ip_buffer.initialize(m_decoder->getInputParameter(OVP_GD_Algorithm_StimulationDecoder_InputParameterId_MemoryBufferToDecode));
    op_stimulationSet.initialize(m_decoder->getOutputParameter(OVP_GD_Algorithm_StimulationDecoder_OutputParameterId_StimulationSet));

    return true;
}

///-------------------------------------------------------------------------------------------------
bool CBoxAlgorithmPlayerController::uninitialize()
{
    op_stimulationSet.uninitialize();
    ip_buffer.uninitialize();

    if (m_decoder) {
        m_decoder->uninitialize();
        this->getAlgorithmManager().releaseAlgorithm(*m_decoder);
        m_decoder = nullptr;
    }

    return true;
}

///-------------------------------------------------------------------------------------------------
bool CBoxAlgorithmPlayerController::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

///-------------------------------------------------------------------------------------------------
bool CBoxAlgorithmPlayerController::process()
{
    Kernel::IBoxIO& boxCtx = this->getDynamicBoxContext();

    for (size_t i = 0; i < boxCtx.getInputChunkCount(0); ++i) {
        ip_buffer = boxCtx.getInputChunk(0, i);
        m_decoder->process();
        if (m_decoder->isOutputTriggerActive(OVP_GD_Algorithm_StimulationDecoder_OutputTriggerId_ReceivedHeader)) { }
        if (m_decoder->isOutputTriggerActive(OVP_GD_Algorithm_StimulationDecoder_OutputTriggerId_ReceivedBuffer)) {
            const CStimulationSet* stimSet = op_stimulationSet;
            for (size_t j = 0; j < stimSet->size(); ++j) {
                if (stimSet->getId(j) == m_stimulationID) {
                    this->getLogManager() << Kernel::LogLevel_Trace << "Received stimulation ["
                            << this->getTypeManager().getEnumerationEntryNameFromValue(OV_TypeId_Stimulation, m_stimulationID)
                            << "] causing action [" << this->getTypeManager().getEnumerationEntryNameFromValue(PlayerAction, m_actionID) << "]\n";

                    bool res = false;
                    if (m_actionID == PlayerAction_Play.id()) { res = this->getPlayerContext().play(); }
                    if (m_actionID == PlayerAction_Stop.id()) { res = this->getPlayerContext().stop(); }
                    if (m_actionID == PlayerAction_Pause.id()) { res = this->getPlayerContext().pause(); }
                    if (m_actionID == PlayerAction_Forward.id()) { res = this->getPlayerContext().forward(); }

                    OV_ERROR_UNLESS_KRF(res, "the player did not accept this box's setting \"Action to perform\", which is [" << this->getTypeManager().getEnumerationEntryNameFromValue(PlayerAction,m_actionID) << "]. We are terribly sorry for the inconvenience.",
                                        Kernel::ErrorType::BadConfig);
                }
            }
        }
        if (m_decoder->isOutputTriggerActive(OVP_GD_Algorithm_StimulationDecoder_OutputTriggerId_ReceivedEnd)) { }

        boxCtx.markInputAsDeprecated(0, i);
    }

    return true;
}

}  // namespace Stimulation
}  // namespace Plugins
}  // namespace OpenViBE
