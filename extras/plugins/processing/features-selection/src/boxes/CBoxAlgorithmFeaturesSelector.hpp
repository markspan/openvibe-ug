///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmFeaturesSelector.hpp
/// \brief Classes of the Box Features Selector.
/// \author Thibaut Monseigne (Inria).
/// \version 1.0.
/// \date 12/02/2020.
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

#include "ovp_defines.h"

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include "algorithm/CMRMR.hpp"
#include <vector>

namespace OpenViBE {
namespace Plugins {
namespace FeaturesSelection {
/// <summary>   The class CBoxAlgorithmFeaturesSelector describes the box Features Selector. </summary>
class CBoxAlgorithmFeaturesSelector final : virtual public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;

    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, OVP_ClassId_BoxAlgorithm_FeaturesSelector)

protected:
    //***** Codecs *****
    Toolkit::TFeatureVectorDecoder<CBoxAlgorithmFeaturesSelector> m_decoder;
    Toolkit::TFeatureVectorEncoder<CBoxAlgorithmFeaturesSelector> m_encoder;
    CMatrix *m_iMatrix = nullptr, *m_oMatrix = nullptr;

    //***** Settings *****
    std::vector<size_t> m_lookup;

    /// <summary> Parse the setting. </summary>
    /// <param name="setting"> Setting to parse. </param>
    /// <returns> True if Setting is correctly parsed.</returns>
    bool parseSetting(const std::string& setting);
};

/// <summary> Descriptor of the box Features Selector. </summary>
class CBoxAlgorithmFeaturesSelectorDesc final : virtual public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Features Selector"; }
    CString getAuthorName() const override { return "Thibaut Monseigne"; }
    CString getAuthorCompanyName() const override { return "Inria"; }
    CString getShortDescription() const override { return "Select a subset of features vector."; }
    CString getDetailedDescription() const override { return "Select Features with index starting from 0."; }
    CString getCategory() const override { return "Features Selection"; }
    CString getVersion() const override { return "1.0"; }
    CString getStockItemName() const override { return "gtk-sort-ascending"; }

    CIdentifier getCreatedClass() const override { return OVP_ClassId_BoxAlgorithm_FeaturesSelector; }
    IPluginObject* create() override { return new CBoxAlgorithmFeaturesSelector; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Input", OV_TypeId_FeatureVector);
        prototype.addOutput("Output", OV_TypeId_FeatureVector);
        prototype.addSetting("Features List", OV_TypeId_String, ":");
        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, OVP_ClassId_BoxAlgorithm_FeaturesSelectorDesc)
};
}  // namespace FeaturesSelection
}  // namespace Plugins
}  // namespace OpenViBE
