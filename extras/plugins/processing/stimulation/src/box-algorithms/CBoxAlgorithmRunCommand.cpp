///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmRunCommand.cpp
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

#include "CBoxAlgorithmRunCommand.hpp"
#include <cstdlib>

namespace OpenViBE {
namespace Plugins {
namespace Stimulation {

bool CBoxAlgorithmRunCommand::initialize()
{
    m_decoder = &this->getAlgorithmManager().getAlgorithm(this->getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_StimulationDecoder));
    m_decoder->initialize();
    ip_buffer.initialize(m_decoder->getInputParameter(OVP_GD_Algorithm_StimulationDecoder_InputParameterId_MemoryBufferToDecode));
    op_stimSet.initialize(m_decoder->getOutputParameter(OVP_GD_Algorithm_StimulationDecoder_OutputParameterId_StimulationSet));

    const size_t nSetting = this->getStaticBoxContext().getSettingCount();
    for (size_t i = 0; i < nSetting; i += 2) {
        m_commands[uint64_t(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), i))].
                push_back(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), i + 1));
    }

    return true;
}

bool CBoxAlgorithmRunCommand::uninitialize()
{
    op_stimSet.uninitialize();
    ip_buffer.uninitialize();
    m_decoder->uninitialize();
    this->getAlgorithmManager().releaseAlgorithm(*m_decoder);

    return true;
}

bool CBoxAlgorithmRunCommand::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

bool CBoxAlgorithmRunCommand::process()
{
    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();

    for (size_t i = 0; i < boxContext.getInputChunkCount(0); ++i) {
        ip_buffer = boxContext.getInputChunk(0, i);
        m_decoder->process();
        if (m_decoder->isOutputTriggerActive(OVP_GD_Algorithm_StimulationDecoder_OutputTriggerId_ReceivedHeader)) { }
        if (m_decoder->isOutputTriggerActive(OVP_GD_Algorithm_StimulationDecoder_OutputTriggerId_ReceivedBuffer)) {
            const CStimulationSet* stimSet = op_stimSet;
            for (size_t j = 0; j < stimSet->size(); ++j) {
                uint64_t stimulationID = stimSet->getId(j);
                if (m_commands.find(stimulationID) != m_commands.end()) {
                    std::vector<CString>& command = m_commands[stimulationID];
                    auto it                       = command.begin();
                    while (it != command.end()) {
                        std::string str = it->toASCIIString();
#if defined(WIN32)
                        // On Windows, we pad the call with quotes. This addresses the situation where both the
                        // command and some of its arguments have spaces, e.g. the call being like
                        //
                        // "C:\Program Files\blah.exe" --something "C:\Program Files\duh.dat"
                        //
                        // Without padding the whole line with quotes, it doesn't seem work. 
                        str = std::string("\"") + str + "\"";
#else
                        // On other platforms, we pass the call as-is. We cannot pad the call in the scenario as the
                        // Linux shell - on the other hand - doesn't like calls like ""blah blah" "blah""
#endif
                        this->getLogManager() << Kernel::LogLevel_Debug << "Running command [" << str << "]\n";
                        if (system(str.c_str()) < 0) { this->getLogManager() << Kernel::LogLevel_Warning << "Could not run command " << str << "\n"; }
                        ++it;
                    }
                }
            }
        }
        if (m_decoder->isOutputTriggerActive(OVP_GD_Algorithm_StimulationDecoder_OutputTriggerId_ReceivedEnd)) { }
        boxContext.markInputAsDeprecated(0, i);
    }

    return true;
}
}  // namespace Stimulation
}  // namespace Plugins
}  // namespace OpenViBE
