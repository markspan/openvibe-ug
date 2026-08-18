///-------------------------------------------------------------------------------------------------
/// 
/// \file CAlgorithmPairwiseStrategyPKPD.hpp
/// \brief Classes for the Algorithm Pairwise Strategy PKPD.
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
 * @brief The CAlgorithmPairwiseStrategyPKPD class
 * This strategy relies on the algorithm describe in the article . Price, S. Knerr, L. Personnaz, and G. Dreyfus.
 * Pairwise neural network classifiers with probabilistic outputs. In G. Tesauro, D. Touretzky, and T. Leen (eds.)
 * Advances in Neural Information Processing Systems 7 (NIPS-94), pp. 1109-1116. MIT Press, 1995.
 */
class CAlgorithmPairwiseStrategyPKPD final : virtual public CAlgorithmPairwiseDecision
{
public:
    CAlgorithmPairwiseStrategyPKPD() { }
    void release() override { delete this; }
    bool initialize() override { return true; }
    bool uninitialize() override { return true; }
    bool Parameterize() override;
    bool Compute(std::vector<classification_info_t>& classifications, CMatrix* probabilities) override;
    XML::IXMLNode* SaveConfig() override;
    bool LoadConfig(XML::IXMLNode& /*node*/) override { return true; }

    _IsDerivedFromClass_Final_(CAlgorithmPairwiseDecision, Algorithm_PairwiseStrategy_PKPD)

private:
    size_t m_nClass = 0;
};

class CAlgorithmPairwiseStrategyPKPDDesc final : virtual public CAlgorithmPairwiseDecisionDesc
{
public:
    void release() override { }

    CString getName() const override { return "Pairwise decision strategy based on PKPD"; }
    CString getAuthorName() const override { return "Guillaume Serrière"; }
    CString getAuthorCompanyName() const override { return "Inria"; }
    CString getShortDescription() const override { return "."; }

    CString getDetailedDescription() const override
    {
        return "Price, S. Knerr, L. Personnaz, and G. Dreyfus."
                "Pairwise neural network classifiers with probabilistic outputs."
                " In G. Tesauro, D. Touretzky, and T. Leen (eds.)"
                "Advances in Neural Information Processing Systems 7 (NIPS-94), pp."
                " 1109-1116. MIT Press, 1995.";
    }

    CString getCategory() const override { return ""; }
    CString getVersion() const override { return "0.1"; }

    CIdentifier getCreatedClass() const override { return Algorithm_PairwiseStrategy_PKPD; }
    IPluginObject* create() override { return new CAlgorithmPairwiseStrategyPKPD; }

    bool getAlgorithmPrototype(Kernel::IAlgorithmProto& prototype) const override
    {
        CAlgorithmPairwiseDecisionDesc::getAlgorithmPrototype(prototype);
        return true;
    }

    _IsDerivedFromClass_Final_(CAlgorithmPairwiseDecisionDesc, Algorithm_PairwiseStrategy_PKPDDesc)
};
}  // namespace Classification
}  // namespace Plugins
}  // namespace OpenViBE
