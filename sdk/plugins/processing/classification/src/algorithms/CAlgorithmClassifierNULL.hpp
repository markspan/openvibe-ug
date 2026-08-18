///-------------------------------------------------------------------------------------------------
/// 
/// \file CAlgorithmClassifierNULL.hpp
/// \brief Classes for a classification algorithm example.
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

#pragma once

#include "../defines.hpp"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <xml/IXMLNode.h>

namespace OpenViBE {
namespace Plugins {
namespace Classification {
class CAlgorithmClassifierNULL final : public Toolkit::CAlgorithmClassifier
{
public:
    CAlgorithmClassifierNULL() { }
    bool initialize() override;
    bool train(const Toolkit::IFeatureVectorSet& featureVectorSet) override;
    bool classify(const Toolkit::IFeatureVector& featureVector, double& classId, Toolkit::IVector& distance, Toolkit::IVector& probability) override;
    XML::IXMLNode* saveConfig() override { return nullptr; }
    bool loadConfig(XML::IXMLNode* /*configurationNode*/) override { return true; }
    size_t getNProbabilities() override { return 1; }
    size_t getNDistances() override { return 1; }

    _IsDerivedFromClass_Final_(CAlgorithmClassifier, Algorithm_ClassifierNULL)
};

class CAlgorithmClassifierNULLDesc final : public Toolkit::CAlgorithmClassifierDesc
{
public:
    void release() override { }

    CString getName() const override { return "NULL Classifier (does nothing)"; }
    CString getAuthorName() const override { return "Yann Renard"; }
    CString getAuthorCompanyName() const override { return "INRIA"; }
    CString getShortDescription() const override { return ""; }
    CString getDetailedDescription() const override { return ""; }
    CString getCategory() const override { return "Samples"; }
    CString getVersion() const override { return "1.0"; }

    CIdentifier getCreatedClass() const override { return Algorithm_ClassifierNULL; }
    IPluginObject* create() override { return new CAlgorithmClassifierNULL; }

    bool getAlgorithmPrototype(Kernel::IAlgorithmProto& prototype) const override
    {
        CAlgorithmClassifierDesc::getAlgorithmPrototype(prototype);
        prototype.addInputParameter(ClassifierNULL_InputParameterId_Parameter1, "Parameter 1", Kernel::ParameterType_Boolean);
        prototype.addInputParameter(ClassifierNULL_InputParameterId_Parameter2, "Parameter 2", Kernel::ParameterType_Float);
        prototype.addInputParameter(ClassifierNULL_InputParameterId_Parameter3, "Parameter 3", Kernel::ParameterType_Enumeration, OV_TypeId_Stimulation);
        return true;
    }

    _IsDerivedFromClass_Final_(CAlgorithmClassifierDesc, Algorithm_ClassifierNULLDesc)
};
}  // namespace Classification
}  // namespace Plugins
}  // namespace OpenViBE
