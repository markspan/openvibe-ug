///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmKappaCoefficient.cpp
/// \author Serriere Guillaume (Inria)
/// \version 1.0.
/// \date 05/05/2015.
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

#include "CBoxAlgorithmKappaCoefficient.hpp"
#include "../algorithms/CAlgorithmConfusionMatrix.hpp"

#include <sstream>
#include <vector>
#include <iomanip>

namespace OpenViBE {
namespace Plugins {
namespace Evaluation {

static const size_t CLASS_LABEL_OFFSET = 1;

bool CBoxAlgorithmKappaCoef::initialize()
{
    //Initialize input/output
    m_targetStimDecoder.initialize(*this, 0);
    m_classifierStimDecoder.initialize(*this, 1);

    m_encoder.initialize(*this, 0);

    //Confusion matrix algorithm
    m_algorithm = &this->getAlgorithmManager().getAlgorithm(this->getAlgorithmManager().createAlgorithm(Algorithm_ConfusionMatrix));
    m_algorithm->initialize();

    Kernel::TParameterHandler<bool> percentHandler(m_algorithm->getInputParameter(Algorithm_ConfusionMatrixAlgorithm_InputParameterId_Percentage));
    percentHandler = false;

    Kernel::TParameterHandler<bool> sumsHandler(m_algorithm->getInputParameter(Algorithm_ConfusionMatrixAlgorithm_InputParameterId_Sums));
    sumsHandler = true;

    m_amountClass = getBoxAlgorithmContext()->getStaticBoxContext()->getSettingCount() - CLASS_LABEL_OFFSET;
    std::vector<size_t> classCodes;
    classCodes.resize(m_amountClass);
    for (size_t i = 0; i < m_amountClass; ++i) {
        // classes are settings from 2 to n
        classCodes[i] = uint64_t(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), i + CLASS_LABEL_OFFSET));
    }

    // Let's check that each identifier is unique
    for (size_t i = 0; i < m_amountClass; ++i) {
        for (size_t j = i + 1; j < m_amountClass; ++j) {
            if (classCodes[i] == classCodes[j]) {
                const CString value = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), i + CLASS_LABEL_OFFSET);
                getLogManager() << Kernel::LogLevel_Error << "You must use unique classes to compute a Kappa coefficient. Class " << i + CLASS_LABEL_OFFSET << " and " << j + CLASS_LABEL_OFFSET
                        << " are the same (" << value.toASCIIString() << ").\n";
                return false;
            }
        }
    }

    const Kernel::TParameterHandler<CStimulationSet*> classesCodesHandler(m_algorithm->getInputParameter(Algorithm_ConfusionMatrixAlgorithm_InputParameterId_ClassCodes));
    for (size_t i = 0; i < classCodes.size(); ++i) { classesCodesHandler->push_back(classCodes[i], 0, 0); }

    //Link all input/output
    Kernel::TParameterHandler<CStimulationSet*> classifierStimSetHandler(m_algorithm->getInputParameter(Algorithm_ConfusionMatrixAlgorithm_InputParameterId_ClassifierStimulationSet));
    classifierStimSetHandler.setReferenceTarget(m_classifierStimDecoder.getOutputStimulationSet());

    Kernel::TParameterHandler<CStimulationSet*> targetStimSetHandler(
        m_algorithm->getInputParameter(Algorithm_ConfusionMatrixAlgorithm_InputParameterId_TargetStimulationSet));
    targetStimSetHandler.setReferenceTarget(m_targetStimDecoder.getOutputStimulationSet());

    op_confusionMatrix.initialize(m_algorithm->getOutputParameter(Algorithm_ConfusionMatrixAlgorithm_OutputParameterId_ConfusionMatrix));

    return true;
}

bool CBoxAlgorithmKappaCoef::uninitialize()
{
    //Log for the automatic test
    this->getLogManager() << Kernel::LogLevel_Info << "Final value of Kappa " << m_kappaCoef << "\n";
    m_algorithm->uninitialize();
    this->getAlgorithmManager().releaseAlgorithm(*m_algorithm);

    m_encoder.uninitialize();
    m_targetStimDecoder.uninitialize();
    m_classifierStimDecoder.uninitialize();

    return true;
}


bool CBoxAlgorithmKappaCoef::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

bool CBoxAlgorithmKappaCoef::process()
{
    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();

    //Input 0: Targets
    for (size_t i = 0; i < boxContext.getInputChunkCount(0); ++i) {
        m_targetStimDecoder.decode(i);

        if (m_targetStimDecoder.isHeaderReceived()) {
            m_algorithm->process(Algorithm_ConfusionMatrixAlgorithm_InputTriggerId_ResetTarget);

            m_encoder.getInputMatrix()->resize(1);
            m_encoder.getInputMatrix()->setDimensionLabel(0, 0, "Kappa");

            m_encoder.encodeHeader();
            boxContext.markOutputAsReadyToSend(0, boxContext.getInputChunkStartTime(0, i), boxContext.getInputChunkEndTime(0, i));
            m_currentProcessingTimeLimit = 0;
        }

        if (m_targetStimDecoder.isBufferReceived()) {
            uint64_t end                 = boxContext.getInputChunkEndTime(0, i);
            m_currentProcessingTimeLimit = (end > m_currentProcessingTimeLimit ? end : m_currentProcessingTimeLimit);
            m_algorithm->process(Algorithm_ConfusionMatrixAlgorithm_InputTriggerId_FeedTarget);
        }

        if (m_targetStimDecoder.isEndReceived()) {
            m_encoder.encodeEnd();
            boxContext.markOutputAsReadyToSend(0, boxContext.getInputChunkStartTime(0, i), boxContext.getInputChunkEndTime(0, i));
        }
    }

    //Input 1: Classifier results
    for (size_t i = 0; i < boxContext.getInputChunkCount(1); ++i) {
        const uint64_t end = boxContext.getInputChunkEndTime(1, i);
        if (end <= m_currentProcessingTimeLimit) {
            m_classifierStimDecoder.decode(i);

            if (m_classifierStimDecoder.isHeaderReceived()) { m_algorithm->process(Algorithm_ConfusionMatrixAlgorithm_InputTriggerId_ResetClassifier); }

            if (m_classifierStimDecoder.isBufferReceived()) {
                m_algorithm->process(Algorithm_ConfusionMatrixAlgorithm_InputTriggerId_FeedClassifier);
                if (m_algorithm->isOutputTriggerActive(Algorithm_ConfusionMatrixAlgorithm_OutputTriggerId_ConfusionPerformed)) {
                    //The confusion matrix has changed so we need to update the kappa coefficient
                    const double* matrix = op_confusionMatrix->getBuffer();
                    //First we need the amount of sample that have been classified
                    const size_t total = size_t(matrix[(m_amountClass + 1) * (m_amountClass + 1) - 1]);

                    //Now we gonna compute the two sum we need to compute the kappa coefficient
                    //It's more easy to use a double loop
                    double observed = 0;
                    double expected = 0;

                    for (size_t j = 0; j < m_amountClass; ++j) {
                        //We need to take the column sum in account
                        observed += matrix[j * (m_amountClass + 1) + j];
                        expected += (matrix[(m_amountClass + 1) * j + m_amountClass] * matrix[(m_amountClass + 1) * m_amountClass + j]);
                    }
                    observed /= double(total);
                    expected /= double(total * total);

                    m_kappaCoef = (observed - expected) / (1 - expected);

                    updateKappaValue();
                    m_encoder.getInputMatrix()->getBuffer()[0] = m_kappaCoef;
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

void CBoxAlgorithmKappaCoef::updateKappaValue()
{   
    std::stringstream ss;
    ss << std::fixed << std::setprecision(2) << m_kappaCoef;
    this->getLogManager() << Kernel::LogLevel_Info << "Kappa coefficient: " << ss.str() << "\n";

    //CString kappaCoeff = ss.str().c_str();
    //CString kappaLabel = "x";
    //this->getBoxAlgorithmContext()->getStaticBoxContext()->setSettingValue(kappaLabel, kappaCoeff);
}

}  // namespace Evaluation
}  // namespace Plugins
}  // namespace OpenViBE