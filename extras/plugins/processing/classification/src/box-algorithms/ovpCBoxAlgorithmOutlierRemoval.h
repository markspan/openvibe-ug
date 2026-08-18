#pragma once

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <vector>
#include <map>

#define OVP_ClassId_BoxAlgorithm_OutlierRemovalDesc     OpenViBE::CIdentifier(0x11DA1C24, 0x4C7A74C0)
#define OVP_ClassId_BoxAlgorithm_OutlierRemoval         OpenViBE::CIdentifier(0x09E41B92, 0x4291B612)

namespace OpenViBE {
namespace Plugins {
namespace Classification {
class CBoxAlgorithmOutlierRemoval final : public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;
    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, OVP_ClassId_BoxAlgorithm_OutlierRemoval)

protected:
    typedef struct SFeature
    {
        CMatrix* sampleMatrix;
        uint64_t startTime;
        uint64_t endTime;
    } feature_vector_t;

    bool pruneSet(std::vector<feature_vector_t>& pruned);

    Toolkit::TFeatureVectorDecoder<CBoxAlgorithmOutlierRemoval> m_sampleDecoder;
    Toolkit::TStimulationDecoder<CBoxAlgorithmOutlierRemoval> m_stimDecoder;

    Toolkit::TFeatureVectorEncoder<CBoxAlgorithmOutlierRemoval> m_sampleEncoder;
    Toolkit::TStimulationEncoder<CBoxAlgorithmOutlierRemoval> m_stimEncoder;

    std::vector<feature_vector_t> m_datasets;

    double m_lowerQuantile = 0;
    double m_upperQuantile = 0;
    uint64_t m_trigger     = 0;
    uint64_t m_triggerTime = 0;
};

class CBoxAlgorithmOutlierRemovalDesc final : public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Outlier removal"; }
    CString getAuthorName() const override { return "Jussi T. Lindgren"; }
    CString getAuthorCompanyName() const override { return "Inria"; }
    CString getShortDescription() const override { return "Discards feature vectors with extremal values"; }
    CString getDetailedDescription() const override { return "Simple outlier removal based on quantile estimation"; }
    CString getCategory() const override { return "Classification"; }
    CString getVersion() const override { return "1.0"; }

    CIdentifier getCreatedClass() const override { return OVP_ClassId_BoxAlgorithm_OutlierRemoval; }
    IPluginObject* create() override { return new CBoxAlgorithmOutlierRemoval; }
    CString getStockItemName() const override { return "gtk-cut"; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Input stimulations", OV_TypeId_Stimulations);
        prototype.addInput("Input features", OV_TypeId_FeatureVector);

        prototype.addOutput("Output stimulations", OV_TypeId_Stimulations);
        prototype.addOutput("Output features", OV_TypeId_FeatureVector);

        prototype.addSetting("Lower quantile", OV_TypeId_Float, "0.01");
        prototype.addSetting("Upper quantile", OV_TypeId_Float, "0.99");
        prototype.addSetting("Start trigger", OV_TypeId_Stimulation, "OVTK_StimulationId_Train");

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, OVP_ClassId_BoxAlgorithm_OutlierRemovalDesc)
};
}  // namespace Classification
}  // namespace Plugins
}  // namespace OpenViBE
