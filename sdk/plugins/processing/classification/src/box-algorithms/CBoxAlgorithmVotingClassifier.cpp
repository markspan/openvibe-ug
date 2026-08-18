///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmVotingClassifier.cpp
/// \brief Classes implementation for the Box Voting Classifier.
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

#include "CBoxAlgorithmVotingClassifier.hpp"

#include <vector>
#include <string>
#include <algorithm>

namespace OpenViBE {
namespace Plugins {
namespace Classification {

bool CBoxAlgorithmVotingClassifier::initialize()
{
    const Kernel::IBox& boxContext = this->getStaticBoxContext();

    m_classificationChoiceEncoder.initialize(*this, 0);

    CIdentifier typeID;
    boxContext.getInputType(0, typeID);
    m_matrixBased = (typeID == OV_TypeId_StreamedMatrix);

    for (size_t i = 0; i < boxContext.getInputCount(); ++i) {
        input_t& input = m_results[i];
        if (m_matrixBased) {
            auto* decoder = new Toolkit::TStreamedMatrixDecoder<CBoxAlgorithmVotingClassifier>();
            decoder->initialize(*this, i);
            input.decoder       = decoder;
            input.op_matrix     = decoder->getOutputMatrix();
            input.twoValueInput = false;
        }
        else {
            auto* decoder = new Toolkit::TStimulationDecoder<CBoxAlgorithmVotingClassifier>();
            decoder->initialize(*this, i);
            input.decoder       = decoder;
            input.op_stimSet    = decoder->getOutputStimulationSet();
            input.twoValueInput = false;
        }
    }

    m_nRepetitions         = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
    m_targetClassLabel     = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);
    m_nonTargetClassLabel  = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2);
    m_rejectClassLabel     = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 3);
    m_resultClassLabelBase = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 4);
    m_chooseOneIfExAequo   = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 5);

    m_lastTime = 0;

    m_classificationChoiceEncoder.encodeHeader();
    this->getDynamicBoxContext().markOutputAsReadyToSend(0, m_lastTime, this->getPlayerContext().getCurrentTime());

    return true;
}

bool CBoxAlgorithmVotingClassifier::uninitialize()
{
    const size_t nInput = this->getStaticBoxContext().getInputCount();

    for (size_t i = 0; i < nInput; ++i) {
        input_t& input = m_results[i];
        input.decoder->uninitialize();
        delete input.decoder;
    }

    m_classificationChoiceEncoder.uninitialize();

    return true;
}

bool CBoxAlgorithmVotingClassifier::processInput(const size_t /*index*/)
{
    this->getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

bool CBoxAlgorithmVotingClassifier::process()
{
    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();
    const size_t nInput        = this->getStaticBoxContext().getInputCount();

    bool canChoose = true;

    for (size_t i = 0; i < nInput; ++i) {
        input_t& input = m_results[i];
        for (size_t j = 0; j < boxContext.getInputChunkCount(i); ++j) {
            input.decoder->decode(j);

            if (input.decoder->isHeaderReceived()) {
                if (m_matrixBased) {
                    if (input.op_matrix->getBufferElementCount() != 1) {
                        OV_ERROR_UNLESS_KRF(input.op_matrix->getBufferElementCount() == 2,
                                            "Invalid input matrix with [" << input.op_matrix->getBufferElementCount() << "] (expected values must be 1 or 2)",
                                            Kernel::ErrorType::BadInput);

                        this->getLogManager() << Kernel::LogLevel_Debug <<
                                "Input got two dimensions, the value use for the vote will be the difference between the two values\n";
                        input.twoValueInput = true;
                    }
                }
            }
            if (input.decoder->isBufferReceived()) {
                if (m_matrixBased) {
                    double value;
                    if (input.twoValueInput) { value = input.op_matrix->getBuffer()[1] - input.op_matrix->getBuffer()[0]; }
                    else { value = input.op_matrix->getBuffer()[0]; }
                    input.scores.push_back(std::pair<double, uint64_t>(-value, boxContext.getInputChunkEndTime(i, j)));
                }
                else {
                    for (size_t k = 0; k < input.op_stimSet->size(); ++k) {
                        const uint64_t id = input.op_stimSet->getId(k);
                        if (id == m_targetClassLabel || id == m_nonTargetClassLabel || id == m_rejectClassLabel) {
                            input.scores.push_back(std::pair<double, uint64_t>(id == m_targetClassLabel ? 1 : 0, input.op_stimSet->getDate(k)));
                        }
                    }
                }
            }
            if (input.decoder->isEndReceived()) {
                m_classificationChoiceEncoder.encodeEnd();
                boxContext.markOutputAsReadyToSend(0, m_lastTime, this->getPlayerContext().getCurrentTime());
            }
        }

        if (input.scores.size() < m_nRepetitions) { canChoose = false; }
    }

    if (canChoose) {
        double score        = -1E100;
        uint64_t classLabel = m_rejectClassLabel;
        uint64_t time       = 0;

        std::map<size_t, double> scores;
        for (size_t i = 0; i < nInput; ++i) {
            input_t& input = m_results[i];
            scores[i]      = 0;
            for (size_t j = 0; j < m_nRepetitions; ++j) { scores[i] += input.scores[j].first; }

            if (scores[i] > score) {
                score      = scores[i];
                classLabel = m_resultClassLabelBase + i;
                time       = input.scores[size_t(m_nRepetitions - 1)].second;
            }
            else if (scores[i] == score) {
                if (!m_chooseOneIfExAequo) {
                    score      = scores[i];
                    classLabel = m_rejectClassLabel;
                    time       = input.scores[size_t(m_nRepetitions - 1)].second;
                }
            }

            input.scores.erase(input.scores.begin(), input.scores.begin() + int(m_nRepetitions));

            this->getLogManager() << Kernel::LogLevel_Debug << "Input " << i << " got score " << scores[i] << "\n";
        }

        if (classLabel != m_rejectClassLabel) {
            this->getLogManager() << Kernel::LogLevel_Debug << "Chosen "
                    << this->getTypeManager().getEnumerationEntryNameFromValue(OV_TypeId_Stimulation, classLabel) << " with score " << score << "\n";
        }
        else {
            this->getLogManager() << Kernel::LogLevel_Debug << "Chosen rejection "
                    << this->getTypeManager().getEnumerationEntryNameFromValue(OV_TypeId_Stimulation, classLabel) << "\n";
        }
        m_classificationChoiceEncoder.getInputStimulationSet()->clear();
        m_classificationChoiceEncoder.getInputStimulationSet()->push_back(classLabel, time, 0);

        m_classificationChoiceEncoder.encodeBuffer();
        boxContext.markOutputAsReadyToSend(0, m_lastTime, time);
        m_lastTime = time;
    }

    return true;
}

}  // namespace Classification
}  // namespace Plugins
}  // namespace OpenViBE
