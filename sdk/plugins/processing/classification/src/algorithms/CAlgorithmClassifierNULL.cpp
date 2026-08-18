///-------------------------------------------------------------------------------------------------
/// 
/// \file CAlgorithmClassifierNULL.cpp
/// \brief Classes implementation for a classification algorithm example.
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

#include "CAlgorithmClassifierNULL.hpp"

#include <random>

namespace OpenViBE {
namespace Plugins {
namespace Classification {


bool CAlgorithmClassifierNULL::initialize()
{
    Kernel::TParameterHandler<bool> ip_bParameter1(this->getInputParameter(ClassifierNULL_InputParameterId_Parameter1));
    Kernel::TParameterHandler<double> ip_Parameter2(this->getInputParameter(ClassifierNULL_InputParameterId_Parameter2));
    Kernel::TParameterHandler<uint64_t> ip_parameter3(this->getInputParameter(ClassifierNULL_InputParameterId_Parameter3));

    ip_bParameter1 = true;
    ip_Parameter2  = 3.141592654;
    ip_parameter3  = OVTK_StimulationId_Label_00;

    Kernel::TParameterHandler<XML::IXMLNode*> op_configuration(this->getOutputParameter(OVTK_Algorithm_Classifier_OutputParameterId_Config));
    op_configuration = nullptr;

    return CAlgorithmClassifier::initialize();
}

bool CAlgorithmClassifierNULL::train(const Toolkit::IFeatureVectorSet& /*featureVectorSet*/)
{
    Kernel::TParameterHandler<bool> ip_bParameter1(this->getInputParameter(ClassifierNULL_InputParameterId_Parameter1));
    Kernel::TParameterHandler<double> ip_Parameter2(this->getInputParameter(ClassifierNULL_InputParameterId_Parameter2));
    Kernel::TParameterHandler<uint64_t> ip_parameter3(this->getInputParameter(ClassifierNULL_InputParameterId_Parameter3));

    OV_WARNING_K("Parameter 1 : " << ip_bParameter1);
    OV_WARNING_K("Parameter 2 : " << ip_Parameter2);
    OV_WARNING_K("Parameter 3 : " << ip_parameter3);

    return true;
}

bool CAlgorithmClassifierNULL::classify(const Toolkit::IFeatureVector& /*featureVector*/, double& classId, Toolkit::IVector& distance,
                                        Toolkit::IVector& probability)
{
    std::random_device rd;
    std::default_random_engine rng(rd());
    std::uniform_int_distribution<size_t> uni(0, std::numeric_limits<size_t>::max()); // no const on unix system
    classId = 1.0 + double(uni(rng) % 3);

    distance.setSize(1);
    probability.setSize(1);
    if (classId == 1) {
        distance[0]    = -1;
        probability[0] = 1;
    }
    else {
        distance[0]    = 1;
        probability[0] = 0;
    }
    return true;
}
}  // namespace Classification
}  // namespace Plugins
}  // namespace OpenViBE
