///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmConfusionMatrix.cpp
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

#include "CBoxAlgorithmConfusionMatrix.hpp"

#include "../algorithms/CAlgorithmConfusionMatrix.hpp"

#include <vector>

namespace OpenViBE {
namespace Plugins {
namespace Evaluation {

bool CBoxAlgorithmConfusionMatrix::initialize()
{
    //Initialize input/output
    m_targetStimDecoder.initialize(*this, 0);
    m_classifierStimDecoder.initialize(*this, 1);

    m_encoder.initialize(*this, 0);

    //CONFUSION MATRIX ALGORITHM
    m_algorithm = &this->getAlgorithmManager().getAlgorithm(this->getAlgorithmManager().createAlgorithm(Algorithm_ConfusionMatrix));
    m_algorithm->initialize();

    Kernel::TParameterHandler<bool> percentHandler(m_algorithm->getInputParameter(Algorithm_ConfusionMatrixAlgorithm_InputParameterId_Percentage));
    percentHandler = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);

    Kernel::TParameterHandler<bool> sumsHandler(m_algorithm->getInputParameter(Algorithm_ConfusionMatrixAlgorithm_InputParameterId_Sums));
    if (!bool(percentHandler)) { sumsHandler = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1); }
    else {
        this->getLogManager() << Kernel::LogLevel_Debug << "Asking for percentage. The value of the setting \"Sums\" will be ignored.\n";
        sumsHandler = false;
    }


    const size_t nClass = getBoxAlgorithmContext()->getStaticBoxContext()->getSettingCount() - FIRST_CLASS_SETTING_INDEX;
    std::vector<size_t> classCodes;
    classCodes.resize(nClass);
    for (size_t i = 0; i < nClass; ++i) {
        // classes are settings from 2 to n
        classCodes[i] = uint64_t(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), i + FIRST_CLASS_SETTING_INDEX));
    }
    // verification...
    for (size_t i = 0; i < nClass; ++i) {
        for (size_t j = i + 1; j < nClass; ++j) {
            if (classCodes[i] == classCodes[j]) {
                const CString classValue = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), i + FIRST_CLASS_SETTING_INDEX);
                getLogManager() << Kernel::LogLevel_Error << "You must use unique classes to compute a confusion matrix. Class " << i + 1 << " and " << j + 1 << " are the same (" << classValue << ").\n";
                return false;
            }
        }
    }

    const Kernel::TParameterHandler<CStimulationSet*> classesCodesHandler(m_algorithm->getInputParameter(Algorithm_ConfusionMatrixAlgorithm_InputParameterId_ClassCodes));
    for (size_t i = 0; i < classCodes.size(); ++i) { classesCodesHandler->push_back(classCodes[i], 0, 0); }

    //Link all input/output
    Kernel::TParameterHandler<CStimulationSet*> classifierStimSetHandler(m_algorithm->getInputParameter(Algorithm_ConfusionMatrixAlgorithm_InputParameterId_ClassifierStimulationSet));
    classifierStimSetHandler.setReferenceTarget(m_classifierStimDecoder.getOutputStimulationSet());

    Kernel::TParameterHandler<CStimulationSet*> targetStimSetHandler(m_algorithm->getInputParameter(Algorithm_ConfusionMatrixAlgorithm_InputParameterId_TargetStimulationSet));
    targetStimSetHandler.setReferenceTarget(m_targetStimDecoder.getOutputStimulationSet());

    Kernel::TParameterHandler<CMatrix*> matrixHandler(m_algorithm->getOutputParameter(Algorithm_ConfusionMatrixAlgorithm_OutputParameterId_ConfusionMatrix));
    m_encoder.getInputMatrix().setReferenceTarget(matrixHandler);

    return true;
}

bool CBoxAlgorithmConfusionMatrix::uninitialize()
{
    m_algorithm->uninitialize();
    this->getAlgorithmManager().releaseAlgorithm(*m_algorithm);

    m_encoder.uninitialize();
    m_targetStimDecoder.uninitialize();
    m_classifierStimDecoder.uninitialize();

    return true;
}

bool CBoxAlgorithmConfusionMatrix::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

bool CBoxAlgorithmConfusionMatrix::process()
{
    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();

    //Input 0: Targets
    for (size_t i = 0; i < boxContext.getInputChunkCount(0); ++i) {
        m_targetStimDecoder.decode(i);

        if (m_targetStimDecoder.isHeaderReceived()) {
            m_algorithm->process(Algorithm_ConfusionMatrixAlgorithm_InputTriggerId_ResetTarget);

            m_encoder.encodeHeader();
            boxContext.markOutputAsReadyToSend(0, boxContext.getInputChunkStartTime(0, i), boxContext.getInputChunkEndTime(0, i));
            m_currentProcessingTimeLimit = 0;
        }

        if (m_targetStimDecoder.isBufferReceived()) {
            uint64_t chunkEndTime        = boxContext.getInputChunkEndTime(0, i);
            m_currentProcessingTimeLimit = (chunkEndTime > m_currentProcessingTimeLimit ? chunkEndTime : m_currentProcessingTimeLimit);
            m_algorithm->process(Algorithm_ConfusionMatrixAlgorithm_InputTriggerId_FeedTarget);
        }

        if (m_targetStimDecoder.isEndReceived()) {
            m_encoder.encodeEnd();
            boxContext.markOutputAsReadyToSend(0, boxContext.getInputChunkStartTime(0, i), boxContext.getInputChunkEndTime(0, i));
        }
    }

    //Input 1: Classifier results
    for (size_t i = 0; i < boxContext.getInputChunkCount(1); ++i) {
        const uint64_t tEnd = boxContext.getInputChunkEndTime(1, i);
        if (tEnd <= m_currentProcessingTimeLimit) {
            m_classifierStimDecoder.decode(i);

            if (m_classifierStimDecoder.isHeaderReceived()) { m_algorithm->process(Algorithm_ConfusionMatrixAlgorithm_InputTriggerId_ResetClassifier); }

            if (m_classifierStimDecoder.isBufferReceived()) {
                m_algorithm->process(Algorithm_ConfusionMatrixAlgorithm_InputTriggerId_FeedClassifier);
                if (m_algorithm->isOutputTriggerActive(Algorithm_ConfusionMatrixAlgorithm_OutputTriggerId_ConfusionPerformed)) {
                    m_encoder.encodeBuffer();
                    boxContext.markOutputAsReadyToSend(0, boxContext.getInputChunkStartTime(1, i), boxContext.getInputChunkEndTime(1, i));
                }
            }

            if (m_classifierStimDecoder.isEndReceived()) {
                m_encoder.encodeEnd();
                boxContext.markOutputAsReadyToSend(0, boxContext.getInputChunkStartTime(1, i), boxContext.getInputChunkEndTime(1, i));
            }

            boxContext.markInputAsDeprecated(1, i);
        }
    }

    return true;
}
}  // namespace Evaluation
}  // namespace Plugins
}  // namespace OpenViBE
