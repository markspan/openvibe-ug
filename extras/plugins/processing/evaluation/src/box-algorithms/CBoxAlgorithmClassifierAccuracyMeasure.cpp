///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmClassifierAccuracyMeasure.cpp
/// \author Laurent Bonnet (INRIA/IRISA)
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

#include "CBoxAlgorithmClassifierAccuracyMeasure.hpp"
#include <visualization-toolkit/ovviz_all.h>

#include <sstream>
#include <iomanip>

namespace OpenViBE {
namespace Plugins {
namespace Evaluation {

bool CBoxAlgorithmClassifierAccuracyMeasure::initialize()
{
    const size_t nInput = this->getStaticBoxContext().getInputCount();

    CIdentifier windowManagerID = CIdentifier(getConfigurationManager().lookUpConfigurationTokenValue("VisualizationContext_WindowManagerID").toASCIIString());
    auto visualizationCtx = dynamic_cast<VisualizationToolkit::IVisualizationContext*>(this->createPluginObject(OVP_ClassId_Plugin_VisualizationCtx));

    m_accuracyView = new ClassifierAccuracyMeasureView(this->getLogManager(), visualizationCtx->getWindowManager(windowManagerID)->getQmlEngine());

    for (size_t i = 1; i < nInput; ++i) {
        //classifier decoders
        m_classifierStimDecoders.push_back(new Toolkit::TStimulationDecoder<CBoxAlgorithmClassifierAccuracyMeasure>());
        m_classifierStimDecoders.back()->initialize(*this, i);

        //visu
        CString inputName;
        getStaticBoxContext().getInputName(i, inputName);
        m_accuracyView->addProgressBar(inputName.toASCIIString());
    }

    m_targetStimDecoder.initialize(*this, 0);
    if (visualizationCtx) {
        CIdentifier boxID(getStaticBoxContext().getIdentifier());
        visualizationCtx->getWindowManager(windowManagerID)->setVisualizationItem(boxID, m_accuracyView->visualItem());
    } else {
        getLogManager() << Kernel::LogLevel_Error << "Could not get visualization context\n";
    }

    return true;
}

bool CBoxAlgorithmClassifierAccuracyMeasure::uninitialize()
{
    const size_t nInput = this->getStaticBoxContext().getInputCount();
    //decoders
    for (size_t i = 0; i < nInput - 1; ++i) {
        m_classifierStimDecoders[i]->uninitialize();
        delete m_classifierStimDecoders[i];
    }
    m_classifierStimDecoders.clear();

    m_targetStimDecoder.uninitialize();

    CIdentifier windowManagerID = CIdentifier(getConfigurationManager().lookUpConfigurationTokenValue("VisualizationContext_WindowManagerID").toASCIIString());
    auto visualizationCtx = dynamic_cast<VisualizationToolkit::IVisualizationContext*>(this->createPluginObject(OVP_ClassId_Plugin_VisualizationCtx));
    if (visualizationCtx) {
        CIdentifier boxID(getStaticBoxContext().getIdentifier());
        visualizationCtx->getWindowManager(windowManagerID)->clearVisualizationItem(boxID);
    }
    delete m_accuracyView;
    m_accuracyView = nullptr;

    return true;
}

bool CBoxAlgorithmClassifierAccuracyMeasure::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();

    return true;
}

bool CBoxAlgorithmClassifierAccuracyMeasure::process()
{
    Kernel::IBoxIO& boxContext           = this->getDynamicBoxContext();
    const Kernel::IBox& staticBoxContext = this->getStaticBoxContext();
    const size_t nInput                  = staticBoxContext.getInputCount();

    //input chunk 0 = targets
    // we iterate over the "target" chunks and update the timeline
    for (size_t i = 0; i < boxContext.getInputChunkCount(0); ++i) {
        m_targetStimDecoder.decode(i);

        if (m_targetStimDecoder.isHeaderReceived()) {
            //header received
            //adding the progress bars to the window            
            for (uint j = 0; j < nInput - 1; ++j) {
                if (j >= m_accuracyView->nbProgressBars()) {
                    CString inputName;
                    staticBoxContext.getInputName(i + 1, inputName);
                    m_accuracyView->addProgressBar(inputName.toASCIIString());
                }
            }

            m_currentProcessingTimeLimit = 0;
        }

        if (m_targetStimDecoder.isBufferReceived()) {
            //buffer received
            //A new target comes, let's update the timeline with it
            const CStimulationSet* dstStimSet = m_targetStimDecoder.getOutputStimulationSet();
            for (size_t s = 0; s < dstStimSet->size(); ++s) {
                const uint64_t id   = dstStimSet->getId(s);
                const uint64_t date = dstStimSet->getDate(s);
                m_targetsTimeLines.insert(std::pair<uint64_t, uint64_t>(date, id));
                getLogManager() << Kernel::LogLevel_Trace << "New target inserted (" << id << "," << CTime(date) << ")\n";
            }

            //we update the time limit for processing classifier stim
            const uint64_t chunkEndTime  = boxContext.getInputChunkEndTime(0, i);
            m_currentProcessingTimeLimit = std::max(chunkEndTime, m_currentProcessingTimeLimit);
        }

        if (m_targetStimDecoder.isEndReceived()) { }

        boxContext.markInputAsDeprecated(0, i);
    }

    //input index 1-n = n classifier results
    for (size_t ip = 1; ip < staticBoxContext.getInputCount(); ++ip) {
        for (size_t i = 0; i < boxContext.getInputChunkCount(ip); ++i) {
            // lets get the chunck end time
            const uint64_t chunkEndTime = boxContext.getInputChunkEndTime(ip, i);
            // if the incoming chunk is in the timeline
            if (chunkEndTime <= m_currentProcessingTimeLimit) {
                if (!m_targetsTimeLines.empty()) {
                    // we can process it
                    m_classifierStimDecoders[ip - 1]->decode(i);

                    if (m_classifierStimDecoders[ip - 1]->isHeaderReceived()) { } //header received
                    if (m_classifierStimDecoders[ip - 1]->isBufferReceived()) {
                        //buffer received
                        const CStimulationSet* stimSet = m_classifierStimDecoders[ip - 1]->getOutputStimulationSet();
                        for (size_t s = 0; s < stimSet->size(); ++s) {
                            //We need to locate the stimulation on the timeline
                            uint64_t id         = stimSet->getId(s);
                            const uint64_t date = stimSet->getDate(s);

                            getLogManager() << Kernel::LogLevel_Trace << "New Classifier state received (" << id << "," << CTime(date) << ") from Classifier " << ip << "\n";

                            auto it   = m_targetsTimeLines.begin();
                            bool cont = true;
                            while (it != m_targetsTimeLines.end() && cont) {
                                auto nextTarget = it;
                                ++nextTarget;
                                if ((nextTarget == m_targetsTimeLines.end() || date < nextTarget->first) && date > it->first) {
                                    if (id == it->second) {
                                        //+1 for this classifier !
                                        m_accuracyView->incrementScore(ip-1);
                                    }
                                    m_accuracyView->incrementNStimulation(ip-1);
                                    cont = false;
                                }
                                ++it;
                            }

                            //auto it = m_targetsTimeLines.lower_bound(l_stimulationFromClassifierDate);
                        }
                    }

                    if (m_targetStimDecoder.isEndReceived()) { }
                }

                boxContext.markInputAsDeprecated(ip, i);
            }
        }
    }
    return true;
}

}  // namespace Evaluation
}  // namespace Plugins
}  // namespace OpenViBE
