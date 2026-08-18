///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmStimulationListener.cpp
/// \brief Classes implementation for the Box Stimulation listener.
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

#include "CBoxAlgorithmStimulationListener.hpp"

namespace OpenViBE {
namespace Plugins {
namespace Tools {

bool CBoxAlgorithmStimulationListener::initialize()
{
    const size_t nInput = this->getStaticBoxContext().getInputCount();
    for (size_t i = 0; i < nInput; ++i) { m_stimulationDecoders.push_back(new Toolkit::TStimulationDecoder<CBoxAlgorithmStimulationListener>(*this, i)); }

    m_logLevel = Kernel::ELogLevel(uint64_t(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0)));

    return true;
}

bool CBoxAlgorithmStimulationListener::uninitialize()
{
    const size_t nInput = this->getStaticBoxContext().getInputCount();
    for (size_t i = 0; i < nInput; ++i) {
        m_stimulationDecoders[i]->uninitialize();
        delete m_stimulationDecoders[i];
    }
    m_stimulationDecoders.clear();

    return true;
}

bool CBoxAlgorithmStimulationListener::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

bool CBoxAlgorithmStimulationListener::process()
{
    const Kernel::IBox& staticboxCtx = this->getStaticBoxContext();
    Kernel::IBoxIO& boxCtx           = this->getDynamicBoxContext();
    const size_t nInput              = this->getStaticBoxContext().getInputCount();

    for (size_t i = 0; i < nInput; ++i) {
        for (size_t j = 0; j < boxCtx.getInputChunkCount(i); ++j) {
            m_stimulationDecoders[i]->decode(j);
            if (m_stimulationDecoders[i]->isHeaderReceived()) { }
            if (m_stimulationDecoders[i]->isBufferReceived()) {
                const CStimulationSet* stimSet = m_stimulationDecoders[i]->getOutputStimulationSet();

                CString inputName;
                staticboxCtx.getInputName(i, inputName);
                for (size_t k = 0; k < stimSet->size(); ++k) {
                    this->getLogManager() << m_logLevel << "For input " << i << " with name " << inputName << " got stimulation " << stimSet->getId(k)
                            << "[" << getTypeManager().getEnumerationEntryNameFromValue(OV_TypeId_Stimulation, stimSet->getId(k)) << "]"
                            << " at date " << CTime(stimSet->getDate(k)) << " and duration " << CTime(stimSet->getDuration(k)) << "\n";

                    OV_WARNING_UNLESS_K(stimSet->getDate(k) >= boxCtx.getInputChunkStartTime(i, j) && stimSet->getDate(k) <= boxCtx.getInputChunkEndTime(i, j),
                                        "Invalid out of range date [" << CTime(stimSet->getDate(k)) << "] (expected value between ["
                                        << CTime(boxCtx.getInputChunkStartTime(i, j)) << "] and [" << CTime(boxCtx.getInputChunkEndTime(i, j)) << "])");
                }
            }
            if (m_stimulationDecoders[i]->isEndReceived()) { }
            boxCtx.markInputAsDeprecated(i, j);
        }
    }

    return true;
}

}  // namespace Tools
}  // namespace Plugins
}  // namespace OpenViBE
