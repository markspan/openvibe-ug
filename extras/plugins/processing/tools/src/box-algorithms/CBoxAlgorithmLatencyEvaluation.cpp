///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmLatencyEvaluation.cpp
/// \author Yann Renard (Inria)
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

#include "CBoxAlgorithmLatencyEvaluation.hpp"
#include <system/ovCTime.h>

// @note by just repeatedly printing to the console, this box introduces significant latency by itself.
// @fixme If it makes sense to enable this box at all, it should be reimplemented as printing to a small GUI widget instead.

namespace OpenViBE {
namespace Plugins {
namespace Tools {

bool CBoxAlgorithmLatencyEvaluation::initialize()
{
    CString logLevel;
    getBoxAlgorithmContext()->getStaticBoxContext()->getSettingValue(0, logLevel);
    m_LogLevel = Kernel::ELogLevel(
        getBoxAlgorithmContext()->getPlayerContext()->getTypeManager().getEnumerationEntryValueFromName(OV_TypeId_LogLevel, logLevel));

    m_StartTime = System::Time::zgetTime();

    return true;
}

bool CBoxAlgorithmLatencyEvaluation::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();

    return true;
}

bool CBoxAlgorithmLatencyEvaluation::process()
{
    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();
    const uint64_t time        = getPlayerContext().getCurrentTime();

    for (size_t i = 0; i < boxContext.getInputChunkCount(0); ++i) {
        const uint64_t start      = boxContext.getInputChunkStartTime(0, i);
        const uint64_t end        = boxContext.getInputChunkEndTime(0, i);
        const double startLatency = (double((int64_t(time - start) >> 22) * 1000) / 1024.0);
        const double endLatency   = (double((int64_t(time - end) >> 22) * 1000) / 1024.0);

        // getLogManager() << Kernel::LogLevel_Debug << "Timing values [start:end:current]=[" << tStart << ":" << tEnd << ":" << time << "]\n";
        getLogManager() << m_LogLevel << "Current latency at chunk " << i << " [start:end]=[" << startLatency << ":" << endLatency << "] ms\n";

        boxContext.markInputAsDeprecated(0, i);
    }

    const uint64_t elapsed = System::Time::zgetTime() - m_StartTime;
    const double latency   = (double((int64_t(elapsed - time) >> 22) * 1000) / 1024.0);

    getLogManager() << m_LogLevel << "Inner latency : " << latency << " ms\n";

    return true;
}
}  // namespace Tools
}  // namespace Plugins
}  // namespace OpenViBE
