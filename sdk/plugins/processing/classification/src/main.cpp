///-------------------------------------------------------------------------------------------------
/// 
/// \file main.cpp
/// \brief Main file for classification plugin, registering the boxes to OpenViBE
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

#include <vector>

#include "defines.hpp"
#include "toolkit/algorithms/classification/ovtkCAlgorithmPairingStrategy.h" //For comparision mecanism

#include "algorithms/CAlgorithmClassifierNULL.hpp"
#include "algorithms/CAlgorithmClassifierOneVsAll.hpp"
#include "algorithms/CAlgorithmClassifierOneVsOne.hpp"

#include "algorithms/CAlgorithmPairwiseStrategyPKPD.hpp"
#include "algorithms/CAlgorithmPairwiseDecisionVoting.hpp"
#include "algorithms/CAlgorithmPairwiseDecisionHT.hpp"

#include "box-algorithms/CBoxAlgorithmVotingClassifier.hpp"
#include "box-algorithms/CBoxAlgorithmClassifierTrainer.hpp"
#include "box-algorithms/CBoxAlgorithmClassifierProcessor.hpp"

#include "algorithms/CAlgorithmConditionedCovariance.hpp"
#include "algorithms/CAlgorithmClassifierLDA.hpp"

#include<cmath>


namespace OpenViBE {
namespace Plugins {
namespace Classification {

OVP_Declare_Begin()
    context.getTypeManager().registerEnumerationEntry(OVTK_TypeId_ClassificationStrategy, "Native", CIdentifier::undefined().id());
    context.getTypeManager().registerEnumerationEntry(OVTK_TypeId_ClassificationStrategy, "OneVsAll", Algorithm_ClassifierOneVsAll.id());
    context.getTypeManager().registerEnumerationEntry(OVTK_TypeId_ClassificationStrategy, "OneVsOne", Algorithm_ClassifierOneVsOne.id());

    //  context.getTypeManager().registerEnumerationEntry(OVTK_TypeId_ClassificationAlgorithm, "NULL Classifier (does nothing)",Algorithm_ClassifierNULL.id());
    //  OVP_Declare_New(CAlgorithmClassifierNULLDesc)

    OVP_Declare_New(CBoxAlgorithmVotingClassifierDesc)
    OVP_Declare_New(CBoxAlgorithmClassifierTrainerDesc)
    OVP_Declare_New(CBoxAlgorithmClassifierProcessorDesc)

    OVP_Declare_New(CAlgorithmClassifierOneVsAllDesc)
    OVP_Declare_New(CAlgorithmClassifierOneVsOneDesc)

    // Functions related to deciding winner in OneVsOne multiclass decision strategy
    context.getTypeManager().registerEnumerationType(TypeId_ClassificationPairwiseStrategy, PAIRWISE_STRATEGY_ENUMERATION_NAME);

    OVP_Declare_New(CAlgorithmPairwiseStrategyPKPDDesc)
    context.getTypeManager().registerEnumerationEntry(TypeId_ClassificationPairwiseStrategy, "PKPD", Algorithm_PairwiseStrategy_PKPD.id());
    OVP_Declare_New(CAlgorithmPairwiseDecisionVotingDesc)
    context.getTypeManager().registerEnumerationEntry(TypeId_ClassificationPairwiseStrategy, "Voting", Algorithm_PairwiseDecision_Voting.id());
    OVP_Declare_New(CAlgorithmPairwiseDecisionHTDesc)
    context.getTypeManager().registerEnumerationEntry(TypeId_ClassificationPairwiseStrategy, "HT", Algorithm_PairwiseDecision_HT.id());

    OVP_Declare_New(CAlgorithmConditionedCovarianceDesc)

    context.getTypeManager().registerEnumerationEntry(OVTK_TypeId_ClassificationAlgorithm, "Linear Discrimimant Analysis (LDA)", Algorithm_ClassifierLDA.id());
    Toolkit::registerClassificationComparisonFunction(Algorithm_ClassifierLDA, LDAClassificationCompare);
    OVP_Declare_New(CAlgorithmClassifierLDADesc)
    context.getTypeManager().registerEnumerationType(Algorithm_ClassifierLDA_DecisionAvailable, PAIRWISE_STRATEGY_ENUMERATION_NAME);
    context.getTypeManager().registerEnumerationEntry(Algorithm_ClassifierLDA_DecisionAvailable, "PKPD", Algorithm_PairwiseStrategy_PKPD.id());
    context.getTypeManager().registerEnumerationEntry(Algorithm_ClassifierLDA_DecisionAvailable, "Voting", Algorithm_PairwiseDecision_Voting.id());
    context.getTypeManager().registerEnumerationEntry(Algorithm_ClassifierLDA_DecisionAvailable, "HT", Algorithm_PairwiseDecision_HT.id());

    context.getTypeManager().registerEnumerationType(TypeId_OneVsOne_DecisionAlgorithms, "One vs One Decision Algorithms");
    context.getTypeManager().registerEnumerationEntry(TypeId_OneVsOne_DecisionAlgorithms, "Linear Discrimimant Analysis (LDA)",
                                                      Algorithm_ClassifierLDA_DecisionAvailable.id());

OVP_Declare_End()

}  // namespace Classification
}  // namespace Plugins
}  // namespace OpenViBE

bool OVFloatEqual(const double first, const double second)
{
    const double epsilon = 0.000001;
    return epsilon > fabs(first - second);
}
