///-------------------------------------------------------------------------------------------------
/// 
/// \file CAlgorithmPairwiseDecisionHT.hpp
/// \brief Classes for the Algorithm Pairwise Decision HT.
/// \author Guillaume Serrière (Inria).
/// \version 0.1.
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

#include "CAlgorithmPairwiseDecision.hpp"

namespace OpenViBE {
namespace Plugins {
namespace Classification {
/**
 * @brief The CAlgorithmPairwiseDecisionHT class is a decision strategy for the One Vs One pairwise decision that implement the
 * method describe in the article Hastie, Trevor; Tibshirani, Robert. Classification by pairwise coupling. The Annals of Statistics 26 (1998), no. 2, 451--471
 *
 * Probability required
 */
class CAlgorithmPairwiseDecisionHT final : virtual public CAlgorithmPairwiseDecision
{
public:
    CAlgorithmPairwiseDecisionHT() { }
    void release() override { delete this; }
    bool initialize() override { return true; }
    bool uninitialize() override { return true; }
    bool Parameterize() override;
    bool Compute(std::vector<classification_info_t>& classifications, CMatrix* probabilities) override;
    XML::IXMLNode* SaveConfig() override;
    bool LoadConfig(XML::IXMLNode& node) override;

    _IsDerivedFromClass_Final_(CAlgorithmPairwiseDecision, Algorithm_PairwiseDecision_HT)

private:
    size_t m_nClass = 0;
};

class CAlgorithmPairwiseDecisionHTDesc final : virtual public CAlgorithmPairwiseDecisionDesc
{
public:
    void release() override { }

    CString getName() const override { return "Pairwise decision strategy based on HT"; }
    CString getAuthorName() const override { return "Guillaume Serrière"; }
    CString getAuthorCompanyName() const override { return "Inria"; }
    CString getShortDescription() const override { return "."; }

    CString getDetailedDescription() const override
    {
        return "This method is based on the method describe in the article "
                "Hastie, Trevor; Tibshirani, Robert. Classification by pairwise coupling."
                "The Annals of Statistics 26 (1998), no. 2, 451--471";
    }

    CString getCategory() const override { return ""; }
    CString getVersion() const override { return "0.1"; }

    CIdentifier getCreatedClass() const override { return Algorithm_PairwiseDecision_HT; }
    IPluginObject* create() override { return new CAlgorithmPairwiseDecisionHT; }

    bool getAlgorithmPrototype(Kernel::IAlgorithmProto& prototype) const override
    {
        CAlgorithmPairwiseDecisionDesc::getAlgorithmPrototype(prototype);
        return true;
    }

    _IsDerivedFromClass_Final_(CAlgorithmPairwiseDecisionDesc, Algorithm_PairwiseDecision_HTDesc)
};
}  // namespace Classification
}  // namespace Plugins
}  // namespace OpenViBE
