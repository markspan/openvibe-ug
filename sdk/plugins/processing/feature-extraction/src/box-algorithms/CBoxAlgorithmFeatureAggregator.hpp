///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmFeatureAggregator.hpp
/// \brief Classes for the Box Feature aggregator.
/// \author Bruno Renier (Inria).
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

#include <string>
#include <vector>
#include <queue>

namespace OpenViBE {
namespace Plugins {
namespace FeatureExtraction {
/**
 * Main plugin class of the feature aggregator plugins.
 * Aggregates the features received in a feature vector then outputs it.
 * */
class CBoxAlgorithmFeatureAggregator final : public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    CBoxAlgorithmFeatureAggregator() { }
    void release() override { delete this; }
    bool initialize() override;
    bool uninitialize() override;
    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, Box_FeatureAggregator)

protected:
    //codecs
    Toolkit::TFeatureVectorEncoder<CBoxAlgorithmFeatureAggregator>* m_encoder = nullptr;
    std::vector<Toolkit::TStreamedMatrixDecoder<CBoxAlgorithmFeatureAggregator>*> m_decoder;

    // contains the labels for each dimension for each input
    std::vector<std::vector<std::vector<std::string>>> m_featureNames;

    // contains the dimension size for each dimension of each input
    std::vector<std::vector<size_t>> m_dimSize;

    // contains the input buffer's total size for each input
    std::vector<size_t> m_iBufferSizes;

    //start time and end time of the last arrived chunk
    uint64_t m_lastChunkStartTime = 0;
    uint64_t m_lastChunkEndTime   = 0;

    // number of inputs
    size_t m_nInput = 0;

    bool m_headerSent = false;
};

class CBoxAlgorithmFeatureAggregatorListener final : public Toolkit::TBoxListener<IBoxListener>
{
    bool check(Kernel::IBox& box) const
    {
        for (size_t i = 0; i < box.getInputCount(); ++i) {
            box.setInputName(i, ("Input stream " + std::to_string(i + 1)).c_str());
            box.setInputType(i, OV_TypeId_StreamedMatrix);
        }

        return true;
    }

public:
    bool onInputRemoved(Kernel::IBox& box, const size_t /*index*/) override { return this->check(box); }
    bool onInputAdded(Kernel::IBox& box, const size_t /*index*/) override { return this->check(box); }

    _IsDerivedFromClass_Final_(Toolkit::TBoxListener<IBoxListener>, CIdentifier::undefined())
};

/**
* Plugin's description
*/
class CBoxAlgorithmFeatureAggregatorDesc final : public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Feature aggregator"; }
    CString getAuthorName() const override { return "Bruno Renier"; }
    CString getAuthorCompanyName() const override { return "INRIA/IRISA"; }
    CString getShortDescription() const override { return "Aggregates input to feature vectors"; }
    CString getDetailedDescription() const override { return "Each chunk of input will be catenated into one feature vector."; }
    CString getCategory() const override { return "Feature extraction"; }
    CString getVersion() const override { return "1.0"; }

    CIdentifier getCreatedClass() const override { return Box_FeatureAggregator; }
    IPluginObject* create() override { return new CBoxAlgorithmFeatureAggregator(); }
    IBoxListener* createBoxListener() const override { return new CBoxAlgorithmFeatureAggregatorListener; }
    void releaseBoxListener(IBoxListener* listener) const override { delete listener; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Input stream 1", OV_TypeId_StreamedMatrix);
        // prototype.addInput("Input stream 2", OV_TypeId_StreamedMatrix);
        prototype.addOutput("Feature vector stream", OV_TypeId_FeatureVector);
        prototype.addFlag(Kernel::BoxFlag_CanAddInput);

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, Box_FeatureAggregatorDesc)
};
}  // namespace FeatureExtraction
}  // namespace Plugins
}  // namespace OpenViBE
