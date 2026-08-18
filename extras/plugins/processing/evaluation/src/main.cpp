///-------------------------------------------------------------------------------------------------
/// 
/// \file main.cpp
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

#include "defines.hpp"

#include "box-algorithms/CBoxAlgorithmStatisticGenerator.hpp"
#include "box-algorithms/CBoxAlgorithmKappaCoefficient.hpp"
#include "box-algorithms/CBoxAlgorithmConfusionMatrix.hpp"
#include "box-algorithms/CBoxAlgorithmROCCurve.hpp"
#include "box-algorithms/CBoxAlgorithmClassifierAccuracyMeasure.hpp"

#include "algorithms/CAlgorithmConfusionMatrix.hpp"

namespace OpenViBE {
namespace Plugins {
namespace Evaluation {

OVP_Declare_Begin()
    context.getTypeManager().registerEnumerationEntry(OV_TypeId_BoxAlgorithmFlag, OV_AttributeId_Box_FlagIsUnstable.toString(), OV_AttributeId_Box_FlagIsUnstable.id());

    OVP_Declare_New(CBoxAlgorithmStatisticGeneratorDesc)

    OVP_Declare_New(CBoxAlgorithmKappaCoefDesc)
    OVP_Declare_New(CBoxAlgorithmROCCurveDesc)

    OVP_Declare_New(CAlgorithmConfusionMatrixDesc)
    OVP_Declare_New(CBoxAlgorithmConfusionMatrixDesc)
    OVP_Declare_New(CBoxAlgorithmClassifierAccuracyMeasureDesc)
OVP_Declare_End()

}  // namespace Evaluation
}  // namespace Plugins
}  // namespace OpenViBE
