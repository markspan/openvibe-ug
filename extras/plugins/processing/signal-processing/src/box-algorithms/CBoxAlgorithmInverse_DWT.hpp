#pragma once

#if defined(TARGET_HAS_ThirdPartyFFTW3) // required by wavelet2s

//You may have to change this path to match your folder organisation
#include "defines.hpp"

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>
#include <string>

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {
/**
 * \class CBoxAlgorithmInverse_DWT
 * \author Joao-Pedro Berti-Ligabo / Inria
 * \date Thu Jul 24 10:57:05 2014
 * \brief The class CBoxAlgorithmInverse_DWT describes the box Inverse DWT.
 *
 */
class CBoxAlgorithmInverse_DWT final : virtual public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    CBoxAlgorithmInverse_DWT() { }

    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;

    bool processInput(const size_t index) override;
    bool process() override;


    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, OVP_ClassId_BoxAlgorithm_Inverse_DWT)

protected:
    // Codec algorithms specified in the skeleton-generator:
    // Signal stream encoder
    Toolkit::TSignalEncoder<CBoxAlgorithmInverse_DWT> m_encoder;
    Toolkit::TSignalDecoder<CBoxAlgorithmInverse_DWT> m_algoInfoDecoder;
    Toolkit::TSignalDecoder<CBoxAlgorithmInverse_DWT>* m_algoXDecoder = nullptr;

    std::string m_waveletType;
    std::string m_decompositionLevel;
    bool m_hasHeader = false;
};


class CBoxAlgorithmInverse_DWTListener final : public Toolkit::TBoxListener<IBoxListener>
{
public:
    bool onSettingValueChanged(Kernel::IBox& box, const size_t index) override
    {
        if (index == 0) { return true; }

        if (index == 1) {
            const size_t nInput = box.getInputCount();

            CString nDecompositionLevels;
            box.getSettingValue(1, nDecompositionLevels);
            std::stringstream ss(nDecompositionLevels.toASCIIString());
            size_t nDecompositionLevel;
            ss >> nDecompositionLevel;

            if (nInput != nDecompositionLevel + 2) {
                for (size_t i = 0; i < nInput; ++i) { box.removeInput(nInput - i - 1); }

                box.addInput("Info",OV_TypeId_Signal);
                box.addInput("A",OV_TypeId_Signal);
                for (size_t i = nDecompositionLevel; i > 0; i--) { box.addInput(("D" + std::to_string(i)).c_str(),OV_TypeId_Signal); }
            }
        }

        return true;
    }

    _IsDerivedFromClass_Final_(Toolkit::TBoxListener<IBoxListener>, CIdentifier::undefined())
};


/**
 * \class CBoxAlgorithmInverse_DWTDesc
 * \author Joao-Pedro Berti-Ligabo / Inria
 * \date Thu Jul 24 10:57:05 2014
 * \brief Descriptor of the box Inverse DWT.
 *
 */
class CBoxAlgorithmInverse_DWTDesc final : virtual public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Inverse DWT"; }
    CString getAuthorName() const override { return "Joao-Pedro Berti-Ligabo"; }
    CString getAuthorCompanyName() const override { return "Inria"; }
    CString getShortDescription() const override { return "Calculate Inverse DiscreteWaveletTransform"; }
    CString getDetailedDescription() const override { return "Calculate Inverse DiscreteWaveletTransform using different types of wavelets"; }
    CString getCategory() const override { return "Signal processing/Wavelets"; }
    CString getVersion() const override { return "1.0"; }
    CString getStockItemName() const override { return "gnome-fs-regular.png"; }

    CIdentifier getCreatedClass() const override { return OVP_ClassId_BoxAlgorithm_Inverse_DWT; }
    IPluginObject* create() override { return new CBoxAlgorithmInverse_DWT; }


    IBoxListener* createBoxListener() const override { return new CBoxAlgorithmInverse_DWTListener; }
    void releaseBoxListener(IBoxListener* listener) const override { delete listener; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Info",OV_TypeId_Signal);
        prototype.addInput("A",OV_TypeId_Signal);
        prototype.addInput("D1",OV_TypeId_Signal);

        prototype.addOutput("Signal",OV_TypeId_Signal);

        prototype.addSetting("Wavelet type",OVP_TypeId_WaveletType, "haar");
        prototype.addSetting("Wavelet decomposition levels",OVP_TypeId_WaveletLevel, "1");

        prototype.addFlag(OV_AttributeId_Box_FlagIsUnstable);

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, OVP_ClassId_BoxAlgorithm_Inverse_DWTDesc)
};
}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE


#endif
