#pragma once

//You may have to change this path to match your folder organisation
#include "defines.hpp"

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>
#include <fstream>

// Verify Eigen Path
#include <Eigen/Dense>

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {
/**
 * \class CBoxAlgorithmEOG_Denoising
 * \author Joao-Pedro Berti-Ligabo / Inria
 * \date Tue May 20 15:33:22 2014
 * \brief The class CBoxAlgorithmEOG_Denoising describes the box Test.
 *
 */
class CBoxAlgorithmEOG_Denoising final : virtual public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:
    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;

    //Here is the different process callbacks possible
    // - On clock ticks :
    //virtual bool processClock(Kernel::CMessageClock& msg);
    // - On new input received (the most common behaviour for signal processing) :
    bool processInput(const size_t index) override;

    // If you want to use processClock, you must provide the clock frequency.
    //virtual uint64_t getClockFrequency();

    bool process() override;

    // As we do with any class in openvibe, we use the macro below
    // to associate this box to an unique identifier.
    // The inheritance information is also made available,
    // as we provide the superclass Toolkit::TBoxAlgorithm < IBoxAlgorithm >
    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, OVP_ClassId_BoxAlgorithm_EOG_Denoising)

protected:
    // Codec algorithms specified in the skeleton-generator:
    // Signal stream decoder
    Toolkit::TSignalDecoder<CBoxAlgorithmEOG_Denoising> m_algo0SignalDecoder;
    Toolkit::TSignalDecoder<CBoxAlgorithmEOG_Denoising> m_algo1SignalDecoder;

    //    Kernel::IAlgorithmProxy* m_matrixRegressionAlgorithm;
    //    Kernel::TParameterHandler < CMatrix* > ip_pMatrixRegressionAlgorithm_Matrix0;
    //    Kernel::TParameterHandler < CMatrix* > ip_pMatrixRegressionAlgorithm_Matrix1;

    //    Kernel::TParameterHandler < CMatrix* > op_pMatrixRegressionAlgorithm_Matrix;
    //    Kernel::TParameterHandler < CString > par_Filename;


    // Signal stream encoder
    Toolkit::TSignalEncoder<CBoxAlgorithmEOG_Denoising> m_algo2SignalEncoder;

    CString m_filename;
    std::ifstream m_fBMatrixFile;
    Eigen::MatrixXd m_noiseCoeff;

    Eigen::Index m_nChannels0 = 0;
    Eigen::Index m_nChannels1 = 0;
    Eigen::Index m_nSamples0  = 0;
    Eigen::Index m_nSamples1  = 0;
};


/**
 * \class CBoxAlgorithmEOG_DenoisingDesc
 * \author Joao-Pedro Berti-Ligabo / Inria
 * \date Tue May 20 15:33:22 2014
 * \brief Descriptor of the box Test.
 *
 */
class CBoxAlgorithmEOG_DenoisingDesc final : virtual public IBoxAlgorithmDesc
{
public:
    void release() override { }

    CString getName() const override { return "EOG Denoising"; }
    CString getAuthorName() const override { return "Joao-Pedro Berti-Ligabo"; }
    CString getAuthorCompanyName() const override { return "Inria"; }
    CString getShortDescription() const override { return "EOG Denoising using Regression Analysis"; }
    CString getDetailedDescription() const override { return "Algorithm implementation as suggested in Schlogl's article of 2007"; }
    CString getCategory() const override { return "Signal processing/Denoising"; }
    CString getVersion() const override { return "023"; }
    CString getStockItemName() const override { return "gnome-fs-regular.png"; }

    CIdentifier getCreatedClass() const override { return OVP_ClassId_BoxAlgorithm_EOG_Denoising; }
    IPluginObject* create() override { return new CBoxAlgorithmEOG_Denoising; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("EEG",OV_TypeId_Signal);
        prototype.addInput("EOG",OV_TypeId_Signal);

        prototype.addOutput("EEG_Corrected",OV_TypeId_Signal);
        prototype.addSetting("Filename b Matrix", OV_TypeId_Filename, "b-Matrix-EEG.txt");

        prototype.addFlag(OV_AttributeId_Box_FlagIsUnstable);

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, OVP_ClassId_BoxAlgorithm_EOG_DenoisingDesc)
};
}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
