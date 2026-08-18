///-------------------------------------------------------------------------------------------------
/// 
/// \file CAlgorithmClassifierOneVsOne.hpp
/// \brief Classes for the Algorithm One Vs One.
/// \author Guillaume Serriere (Inria).
/// \version 0.2.
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

#include <map>

namespace OpenViBE {
namespace Plugins {
namespace Classification {
//The aim of this structure is to record informations returned by the sub-classifier. They will be used by
// pairwise decision algorithms to compute probability vector.
// Should be use only by OneVsOne and pairwise decision algorithm
typedef struct SClassification
{
    double firstClass;
    double secondClass;
    double classLabel;
    //This output is probabilist
    CMatrix* classificationValue;
} classification_info_t;


class CAlgorithmClassifierOneVsOne final : public Toolkit::CAlgorithmPairingStrategy
{
public:
    bool initialize() override;
    bool uninitialize() override;
    bool train(const Toolkit::IFeatureVectorSet& dataset) override;
    bool classify(const Toolkit::IFeatureVector& sample, double& classId, Toolkit::IVector& distance, Toolkit::IVector& probability) override;
    bool designArchitecture(const CIdentifier& id, const size_t classCount) override;
    XML::IXMLNode* saveConfig() override;
    bool loadConfig(XML::IXMLNode* configNode) override;
    size_t getNProbabilities() override { return m_nClasses; }
    size_t getNDistances() override { return 0; }

    _IsDerivedFromClass_Final_(Toolkit::CAlgorithmPairingStrategy, Algorithm_ClassifierOneVsOne)

protected:
    bool createSubClassifiers();

private:
    size_t m_nClasses        = 0;
    size_t m_nSubClassifiers = 0;

    std::map<std::pair<size_t, size_t>, Kernel::IAlgorithmProxy*> m_subClassifiers;
    fClassifierComparison m_algorithmComparison = nullptr;

    Kernel::IAlgorithmProxy* m_decisionStrategyAlgorithm = nullptr;
    CIdentifier m_pairwiseDecisionID                     = CIdentifier::undefined();

    static XML::IXMLNode* getClassifierConfig(double firstClass, double secondClass, Kernel::IAlgorithmProxy* subClassifier);
    XML::IXMLNode* getPairwiseDecisionConfiguration() const;

    // size_t getClassCount() const;

    bool loadSubClassifierConfig(const XML::IXMLNode* node);

    // SSubClassifierDescriptor& getSubClassifierDescriptor(const size_t FirstClass, const size_t SecondClass);
    bool setSubClassifierIdentifier(const CIdentifier& id);
};

class CAlgorithmClassifierOneVsOneDesc final : public Toolkit::CAlgorithmPairingStrategyDesc
{
public:
    void release() override { }

    CString getName() const override { return "OneVsOne pairing classifier"; }
    CString getAuthorName() const override { return "Guillaume Serriere"; }
    CString getAuthorCompanyName() const override { return "INRIA/Loria"; }
    CString getShortDescription() const override { return ""; }
    CString getDetailedDescription() const override { return ""; }
    CString getCategory() const override { return ""; }
    CString getVersion() const override { return "0.2"; }

    CIdentifier getCreatedClass() const override { return Algorithm_ClassifierOneVsOne; }
    IPluginObject* create() override { return new CAlgorithmClassifierOneVsOne; }

    bool getAlgorithmPrototype(Kernel::IAlgorithmProto& prototype) const override
    {
        CAlgorithmPairingStrategyDesc::getAlgorithmPrototype(prototype);
        prototype.addInputParameter(OneVsOneStrategy_InputParameterId_DecisionType, "Pairwise Decision Strategy",
                                    Kernel::ParameterType_Enumeration, TypeId_ClassificationPairwiseStrategy);
        return true;
    }

    _IsDerivedFromClass_Final_(CAlgorithmPairingStrategyDesc, Algorithm_ClassifierOneVsOneDesc)
};
}  // namespace Classification
}  // namespace Plugins
}  // namespace OpenViBE
