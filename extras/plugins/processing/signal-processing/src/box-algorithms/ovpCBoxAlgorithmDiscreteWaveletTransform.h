#pragma once

#if defined(TARGET_HAS_ThirdPartyFFTW3) // required by wavelet2s

//You may have to change this path to match your folder organisation
#include "defines.hpp"

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>
#include <iostream>
#include <string>

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {
/**
 * \class CBoxAlgorithmDiscreteWaveletTransform
 * \author Joao-Pedro Berti-Ligabo / Inria
 * \date Wed Jul 16 15:05:16 2014
 * \brief The class CBoxAlgorithmDiscreteWaveletTransform describes the box DiscreteWaveletTransform.
 *
 */
class CBoxAlgorithmDiscreteWaveletTransform final : virtual public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;

    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, OVP_ClassId_BoxAlgorithm_DiscreteWaveletTransform)

protected:
    // Codec algorithms specified in the skeleton-generator:
    // Signal stream decoder
    Toolkit::TSignalDecoder<CBoxAlgorithmDiscreteWaveletTransform> m_decoder;

    Toolkit::TSignalEncoder<CBoxAlgorithmDiscreteWaveletTransform> m_encoder;
    std::vector<Toolkit::TSignalEncoder<CBoxAlgorithmDiscreteWaveletTransform>*> m_encoders;

    CString m_waveletType;
    CString m_decompositionLevel;

    size_t m_infolength = 0;
    std::vector<std::vector<double>> m_sig;
};


// The box listener can be used to call specific callbacks whenever the box structure changes : input added, name changed, etc.
// Please uncomment below the callbacks you want to use.
class CBoxAlgorithmDiscreteWaveletTransformListener final : public Toolkit::TBoxListener<IBoxListener>
{
public:
    bool onSettingValueChanged(Kernel::IBox& box, const size_t index) override
    {
        if (index == 0) { return true; }

        if (index == 1) {
            const size_t nOutputs = box.getOutputCount();
            CString str;
            box.getSettingValue(1, str);
            const size_t nDecompositionLevels = atoi(str);
            if (nOutputs != nDecompositionLevels + 2) {
                for (size_t i = 0; i < nOutputs; ++i) { box.removeOutput(nOutputs - i - 1); }

                box.addOutput("Info",OV_TypeId_Signal);
                box.addOutput("A",OV_TypeId_Signal);
                for (size_t i = nDecompositionLevels; i > 0; i--) { box.addOutput(("D" + std::to_string(i)).c_str(),OV_TypeId_Signal); }
            }
        }

        return true;
    }

    _IsDerivedFromClass_Final_(Toolkit::TBoxListener<IBoxListener>, CIdentifier::undefined())
};


/**
 * \class CBoxAlgorithmDiscreteWaveletTransformDesc
 * \author Joao-Pedro Berti-Ligabo / Inria
 * \date Wed Jul 16 15:05:16 2014
 * \brief Descriptor of the box DiscreteWaveletTransform.
 *
 */
class CBoxAlgorithmDiscreteWaveletTransformDesc final : virtual public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "Discrete Wavelet Transform"; }
    CString getAuthorName() const override { return "Joao-Pedro Berti-Ligabo"; }
    CString getAuthorCompanyName() const override { return "Inria"; }
    CString getShortDescription() const override { return "Calculate DiscreteWaveletTransform"; }
    CString getDetailedDescription() const override { return "Calculate DiscreteWaveletTransform using different types of wavelets"; }
    CString getCategory() const override { return "Signal processing/Wavelets"; }
    CString getVersion() const override { return "1.0"; }
    CString getStockItemName() const override { return "gnome-fs-regular.png"; }

    CIdentifier getCreatedClass() const override { return OVP_ClassId_BoxAlgorithm_DiscreteWaveletTransform; }
    IPluginObject* create() override { return new CBoxAlgorithmDiscreteWaveletTransform; }


    IBoxListener* createBoxListener() const override { return new CBoxAlgorithmDiscreteWaveletTransformListener; }
    void releaseBoxListener(IBoxListener* listener) const override { delete listener; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Signal",OV_TypeId_Signal);

        prototype.addOutput("Info",OV_TypeId_Signal);
        prototype.addOutput("A",OV_TypeId_Signal);
        prototype.addOutput("D1",OV_TypeId_Signal);

        prototype.addSetting("Wavelet type",OVP_TypeId_WaveletType, "haar");
        prototype.addSetting("Wavelet decomposition levels",OVP_TypeId_WaveletLevel, "1");

        prototype.addFlag(OV_AttributeId_Box_FlagIsUnstable);

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, OVP_ClassId_BoxAlgorithm_DiscreteWaveletTransformDesc)
};
}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE


#endif
