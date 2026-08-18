///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmStimulationFilter.cpp
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

#include "CBoxAlgorithmStimulationFilter.hpp"

#include <cstddef>

namespace OpenViBE {
namespace Plugins {
namespace Stimulation {

bool CBoxAlgorithmStimulationFilter::initialize()
{
    m_defaultAction = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
    m_startTime     = uint64_t(double(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1)) * (1LL << 32));
    m_endTime       = uint64_t(double(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2)) * (1LL << 32));

    if (m_startTime > m_endTime) {
        this->getLogManager() << Kernel::LogLevel_ImportantWarning << "End time is lower than start time\n";
        return false;
    }

    // we iterate over all Rules. One rule has 3 settings
    const size_t nSetting = this->getStaticBoxContext().getSettingCount();
    for (size_t i = 3; i < nSetting; i += 3) {
        rule_t rule;
        rule.action      = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), i); // the action to perform
        rule.startStimID = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), i + 1); // first stim
        rule.EndStimID   = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), i + 2); // last stim in the desired range
        m_rules.push_back(rule);
    }

    m_encoder = &this->getAlgorithmManager().getAlgorithm(this->getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_StimulationEncoder));
    m_encoder->initialize();

    m_decoder = &this->getAlgorithmManager().getAlgorithm(this->getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_StimulationDecoder));
    m_decoder->initialize();

    ip_buffer.initialize(m_decoder->getInputParameter(OVP_GD_Algorithm_StimulationDecoder_InputParameterId_MemoryBufferToDecode));
    op_stimSet.initialize(m_decoder->getOutputParameter(OVP_GD_Algorithm_StimulationDecoder_OutputParameterId_StimulationSet));
    ip_stimSet.initialize(m_encoder->getInputParameter(OVP_GD_Algorithm_StimulationEncoder_InputParameterId_StimulationSet));
    op_buffer.initialize(m_encoder->getOutputParameter(OVP_GD_Algorithm_StimulationEncoder_OutputParameterId_EncodedMemoryBuffer));

    return true;
}

bool CBoxAlgorithmStimulationFilter::uninitialize()
{
    op_buffer.uninitialize();
    ip_stimSet.uninitialize();
    op_stimSet.uninitialize();
    ip_buffer.uninitialize();

    m_decoder->uninitialize();
    this->getAlgorithmManager().releaseAlgorithm(*m_decoder);

    m_encoder->uninitialize();
    this->getAlgorithmManager().releaseAlgorithm(*m_encoder);

    m_rules.clear();

    return true;
}

bool CBoxAlgorithmStimulationFilter::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();

    return true;
}

bool CBoxAlgorithmStimulationFilter::process()
{
    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();

    for (size_t i = 0; i < boxContext.getInputChunkCount(0); ++i) {
        ip_buffer = boxContext.getInputChunk(0, i);
        op_buffer = boxContext.getOutputChunk(0);
        m_decoder->process();
        if (m_decoder->isOutputTriggerActive(OVP_GD_Algorithm_StimulationDecoder_OutputTriggerId_ReceivedHeader)) {
            m_encoder->process(OVP_GD_Algorithm_StimulationEncoder_InputTriggerId_EncodeHeader);
        }
        if (m_decoder->isOutputTriggerActive(OVP_GD_Algorithm_StimulationDecoder_OutputTriggerId_ReceivedBuffer)) {
            ip_stimSet->resize(0);
            for (size_t s = 0; s < op_stimSet->size(); ++s) {
                const uint64_t id   = op_stimSet->getId(s);
                const uint64_t date = op_stimSet->getDate(s);
                uint64_t action     = m_defaultAction;

                if ((m_startTime == m_endTime) || (m_startTime != m_endTime && m_startTime <= date && date <= m_endTime)) {
                    for (size_t r = 0; r < m_rules.size(); ++r) {
                        const rule_t& rule = m_rules[r];
                        if (rule.startStimID <= id && id <= rule.EndStimID) {
                            action = rule.action;
                            this->getLogManager() << Kernel::LogLevel_Debug << "Switches to rule " << r << "\n";
                        }
                    }
                }

                if (action == TypeId_StimulationFilterAction_Select.id()) {
                    this->getLogManager() << Kernel::LogLevel_Trace << "Selects stimulation "
                            << this->getTypeManager().getEnumerationEntryNameFromValue(OV_TypeId_Stimulation, id) << " !\n";
                    ip_stimSet->push_back(id, date, op_stimSet->getDuration(s));
                }
                if (action == TypeId_StimulationFilterAction_Reject.id()) {
                    this->getLogManager() << Kernel::LogLevel_Trace << "Rejects stimulation "
                            << this->getTypeManager().getEnumerationEntryNameFromValue(OV_TypeId_Stimulation, id) << " !\n";
                }
            }

            m_encoder->process(OVP_GD_Algorithm_StimulationEncoder_InputTriggerId_EncodeBuffer);
        }
        if (m_decoder->isOutputTriggerActive(OVP_GD_Algorithm_StimulationDecoder_OutputTriggerId_ReceivedEnd)) {
            m_encoder->process(OVP_GD_Algorithm_StimulationEncoder_InputTriggerId_EncodeEnd);
        }
        boxContext.markInputAsDeprecated(0, i);
        boxContext.markOutputAsReadyToSend(0, boxContext.getInputChunkStartTime(0, i), boxContext.getInputChunkEndTime(0, i));
    }

    return true;
}
}  // namespace Stimulation
}  // namespace Plugins
}  // namespace OpenViBE
