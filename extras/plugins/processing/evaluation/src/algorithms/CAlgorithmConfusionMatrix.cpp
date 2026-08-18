///-------------------------------------------------------------------------------------------------
/// 
/// \file CAlgorithmConfusionMatrix.cpp
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

#include "CAlgorithmConfusionMatrix.hpp"

namespace OpenViBE {
namespace Plugins {
namespace Evaluation {

#ifdef DEBUG
static void dumpMatrix(Kernel::ILogManager& mng, const CMatrix& mat, const CString& desc)
{
    mng << Kernel::LogLevel_Info << desc << "\n";
    for (size_t i = 0; i < mat.getDimensionSize(0); i++)
    {
        mng << Kernel::LogLevel_Info << "Row " << i << ": ";
        for (size_t j = 0; j < mat.getDimensionSize(1); j++) { mng << mat.getBuffer()[i * mat.getDimensionSize(1) + j] << " "; }
        mng << "\n";
    }
}
#endif

bool CAlgorithmConfusionMatrix::initialize()
{
    ip_targetStimSet.initialize(getInputParameter(Algorithm_ConfusionMatrixAlgorithm_InputParameterId_TargetStimulationSet));
    ip_classifierStimSet.initialize(getInputParameter(Algorithm_ConfusionMatrixAlgorithm_InputParameterId_ClassifierStimulationSet));
    ip_classesCodes.initialize(getInputParameter(Algorithm_ConfusionMatrixAlgorithm_InputParameterId_ClassCodes));
    ip_usePercentages.initialize(getInputParameter(Algorithm_ConfusionMatrixAlgorithm_InputParameterId_Percentage));
    ip_useSums.initialize(getInputParameter(Algorithm_ConfusionMatrixAlgorithm_InputParameterId_Sums));
    op_confusionMatrix.initialize(getOutputParameter(Algorithm_ConfusionMatrixAlgorithm_OutputParameterId_ConfusionMatrix));

    return true;
}

bool CAlgorithmConfusionMatrix::uninitialize()
{
#ifdef DEBUG
    dumpMatrix(this->getLogManager(), m_confusionMatrix, "Confusion matrix");
#endif

    ip_targetStimSet.uninitialize();
    ip_classifierStimSet.uninitialize();
    ip_classesCodes.uninitialize();
    ip_usePercentages.uninitialize();
    ip_useSums.uninitialize();
    op_confusionMatrix.uninitialize();

    return true;
}

bool CAlgorithmConfusionMatrix::process()
{
    const size_t nClass = size_t(ip_classesCodes->size());

    if (this->isInputTriggerActive(Algorithm_ConfusionMatrixAlgorithm_InputTriggerId_ResetTarget)) {
        for (size_t i = 0; i < ip_classesCodes->size(); ++i) { this->getLogManager() << Kernel::LogLevel_Trace << "class code " << i << ": " << ip_classesCodes->getId(i) << "\n"; }

        m_nClassificationAttemptPerClass.clear();
        for (size_t i = 0; i < ip_classesCodes->size(); ++i) { m_nClassificationAttemptPerClass.insert(std::make_pair(ip_classesCodes->getId(i), 0)); }

        if (ip_useSums) { op_confusionMatrix->resize(nClass + 1, nClass + 1); }
        else { op_confusionMatrix->resize(nClass, nClass); }

        for (size_t i = 0; i < nClass; ++i) {
            const char* name = this->getTypeManager().getEnumerationEntryNameFromValue(OV_TypeId_Stimulation, ip_classesCodes->getId(i)).toASCIIString();
            op_confusionMatrix->setDimensionLabel(0, i, (std::string("Target Class\n") + name));
            op_confusionMatrix->setDimensionLabel(1, i, (std::string("Result Class\n") + name));
        }

        if (ip_useSums) {
            op_confusionMatrix->setDimensionLabel(0, nClass, "Sums");
            op_confusionMatrix->setDimensionLabel(1, nClass, "Sums");
        }

        m_confusionMatrix.resize(nClass, nClass);

        // initialization
        for (size_t i = 0; i < op_confusionMatrix->getDimensionSize(0); ++i) {
            for (size_t j = 0; j < op_confusionMatrix->getDimensionSize(1); ++j) {
                op_confusionMatrix->getBuffer()[i * op_confusionMatrix->getDimensionSize(1) + j] = 0.0;
                if (i < m_confusionMatrix.getDimensionSize(0) && j < m_confusionMatrix.getDimensionSize(1)) { m_confusionMatrix.getBuffer()[i * m_confusionMatrix.getDimensionSize(1) + j] = 0.0; }
            }
        }
    }

    if (this->isInputTriggerActive(Algorithm_ConfusionMatrixAlgorithm_InputTriggerId_ResetClassifier)) { }

    if (this->isInputTriggerActive(Algorithm_ConfusionMatrixAlgorithm_InputTriggerId_FeedTarget)) {
        for (size_t s = 0; s < ip_targetStimSet->size(); ++s) {
            uint64_t id = ip_targetStimSet->getId(s);
            if (isClass(id)) {
                uint64_t date = ip_targetStimSet->getDate(s);
                m_targetsTimeLines.insert(std::pair<uint64_t, uint64_t>(date, id));
                getLogManager() << Kernel::LogLevel_Trace << "Current target is " << m_targetsTimeLines.rbegin()->second << "\n";
            }
            else { getLogManager() << Kernel::LogLevel_Trace << "The target received is not a valid class: " << id << "\n"; }
        }
    }

    if (this->isInputTriggerActive(Algorithm_ConfusionMatrixAlgorithm_InputTriggerId_FeedClassifier)) {
        for (size_t s = 0; s < ip_classifierStimSet->size(); ++s) {
            //We need to locate the stimulation on the timeline
            uint64_t id = ip_classifierStimSet->getId(s);
            if (!isClass(id))//If we don't have
            {
                getLogManager() << Kernel::LogLevel_Trace << "The result received is not a valid class: " << id << "\n";
                continue;
            }
            uint64_t targeted   = 0;
            const uint64_t date = ip_classifierStimSet->getDate(s);

            bool found = false;
            for (auto it = m_targetsTimeLines.begin(); it != m_targetsTimeLines.end() && !found; ++it) {
                auto nextTarget = it;
                ++nextTarget;
                if ((nextTarget == m_targetsTimeLines.end() || date < nextTarget->first) && date > it->first) {
                    targeted = it->second;
                    found    = true;
                }
            }
            if (found) {
                this->getLogManager() << Kernel::LogLevel_Trace << "Result received : " << id << ". Corresponding target : " << targeted << ".\n";

                if (!op_confusionMatrix->getBuffer()) {
                    this->getLogManager() << Kernel::LogLevel_Error << "The confusion matrix buffer has not yet been initialized\n";
                    return false;
                }

                // now we found the target, let's update the confusion matrix
                // we need to update the whole line vector for the targeted class
                const size_t nOldAttempt = m_nClassificationAttemptPerClass[targeted];
                m_nClassificationAttemptPerClass[targeted]++; // the confusion matrix can treat this result

                size_t i               = getClassIndex(targeted);// the good line index
                const size_t resultIdx = getClassIndex(id);
                for (size_t j = 0; j < nClass; ++j) {
                    double newValue;
                    const double oldValue = op_confusionMatrix->getBuffer()[i * op_confusionMatrix->getDimensionSize(0) + j];
                    if (j == resultIdx) {
                        newValue = (oldValue * nOldAttempt + 1) / (m_nClassificationAttemptPerClass[targeted]);
                        m_confusionMatrix.getBuffer()[i * nClass + j]++;
                    }
                    else { newValue = (oldValue * nOldAttempt) / (m_nClassificationAttemptPerClass[targeted]); }
                    if (ip_usePercentages) { op_confusionMatrix->getBuffer()[i * op_confusionMatrix->getDimensionSize(0) + j] = newValue; }
                    else // the count value
                    {
                        op_confusionMatrix->getBuffer()[i * op_confusionMatrix->getDimensionSize(0) + j] = m_confusionMatrix.getBuffer()[i * nClass + j];
                    }
                }

                //we compute the sums if needed
                if (ip_useSums) {
                    const size_t size = op_confusionMatrix->getDimensionSize(0);
                    double total      = 0.0;
                    for (i = 0; i < nClass; ++i) {
                        double sumRow = 0.0;
                        double sumCol = 0.0;
                        for (size_t j = 0; j < nClass; ++j) {
                            sumRow += op_confusionMatrix->getBuffer()[i * size + j];
                            sumCol += op_confusionMatrix->getBuffer()[j * size + i];
                        }
                        op_confusionMatrix->getBuffer()[i * size + size - 1]   = sumRow;
                        op_confusionMatrix->getBuffer()[(size - 1) * size + i] = sumCol;
                        total += sumRow;
                    }
                    op_confusionMatrix->getBuffer()[(size - 1) * size + size - 1] = total; // the lower-right entry, i.e. the last in the buffer
                }
            }
            else { getLogManager() << Kernel::LogLevel_Warning << " No target available.\n"; }
        }
        this->activateOutputTrigger(Algorithm_ConfusionMatrixAlgorithm_OutputTriggerId_ConfusionPerformed, true);
    }

    return true;
}

bool CAlgorithmConfusionMatrix::isClass(const uint64_t id) const
{
    for (size_t i = 0; i < ip_classesCodes->size(); ++i) { if (ip_classesCodes->getId(i) == id) { return true; } }
    return false;
}

size_t CAlgorithmConfusionMatrix::getClassIndex(const uint64_t id) const
{
    for (size_t i = 0; i < ip_classesCodes->size(); ++i) { if (ip_classesCodes->getId(i) == id) { return i; } }
    return -1;
}

}  // namespace Evaluation
}  // namespace Plugins
}  // namespace OpenViBE
