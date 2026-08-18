///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmConnectivityMeasure.hpp
/// \brief Classes of the Box Connectivity Measure.
/// \author Arthur DESBOIS (Inria).
/// \version 0.0.1.
/// \date Fri Oct 30 16:18:49 2020.
///
/// \copyright Copyright (C) 2020-2022 Inria
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
///-------------------------------------------------------------------------------------------------

#pragma once

#include <Eigen/Dense>

#include "defines.hpp"
#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>
#include "connectivityMeasure.hpp"

#define OV_AttributeId_Box_FlagIsUnstable                OpenViBE::CIdentifier(0x666FFFFF, 0x666FFFFF)

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {

///
/// \brief The class CBoxAlgorithmConnectivityMeasure describes the box Connectivity Measure.
///
class CBoxAlgorithmConnectivityMeasure final : virtual public Toolkit::TBoxAlgorithm<IBoxAlgorithm>
{
public:

    EIGEN_MAKE_ALIGNED_OPERATOR_NEW

    void release() override { delete this; }

    bool initialize() override;
    bool uninitialize() override;

    bool processInput(const size_t index) override;
    bool process() override;

    _IsDerivedFromClass_Final_(Toolkit::TBoxAlgorithm<IBoxAlgorithm>, OVP_ClassId_BoxAlgorithm_ConnectivityMeasure)

protected:

    // Algo class instance
    ConnectivityMeasure connectivityMeasure;

    // Codecs
    Toolkit::TSignalDecoder<CBoxAlgorithmConnectivityMeasure> m_signalDecoder;
    Toolkit::TStreamedMatrixEncoder<CBoxAlgorithmConnectivityMeasure> m_matrixEncoder;    // Output Matrix Codec

    // Matrices
    CMatrix* m_iMatrix = nullptr;        // Input Matrix pointer
    CMatrix* m_oMatrix = nullptr;        // Output Matrix pointer

    std::vector<Eigen::VectorXd> m_vectorXdBuffer; // Vector buffer, for connectivity segments
    std::vector<std::vector<double>> m_signalChannelBuffers;

    Eigen::VectorXd m_window;

    // Settings
    EConnectMetric m_metric = EConnectMetric::Coherence;
    EPsdMode m_psdMode = EPsdMode::Burg;
    EConnectWindowMethod m_windowMethod = EConnectWindowMethod::Hann;

    // Specific to Welch's method
    double m_windowLengthSeconds = 0.25; // size of one welch windowing (sec)
    int m_windowLength = 128; // size of one welch windowing (samples)
    int m_windowOverlap = 50; // overlap btw welch windows (%)
    int m_windowOverlapSamples = 64; // overlap btw welch windows (samples)

    // Specific to Burg's method
    int m_autoRegOrder = 11; // auto-regressive model order, in samples

    // Common
    std::vector<size_t> m_vLookup;
    size_t m_nbChanOutput = 0;
    //uint64_t totalDuration = 0; //for measuring performance

    CString m_selectedChannels;  // subselection of channels, separated with ";". Blank = select all
    int m_connectLength = 256; // size of one full connectivity estimation occurrence (samples)
    double m_connectLengthSeconds = 0.5; // size of one full connectivity estimation occurrence (sec)
    int m_connectOverlap = 50;  // overlap btw connectivity measurements (%)
    int m_connectOverlapSamples = 128; // overlap btw connectivity measurements (samples)
    int m_connectShiftSamples = 64; // shift btw connectivity windows (samples)
    size_t m_fftSize = 128; // FFT/PSD size (and nb of freq taps at the output)
    size_t m_nbChannels = 0;
    bool m_dcRemoval = false;

    uint64_t m_timeIncrement = 0;

    std::vector<uint64_t> m_startTimes;

};

///
/// \brief Listener class for the box Connectivity Measure.
///
class CBoxAlgorithmConnectivityMeasureListener final : public Toolkit::TBoxListener<IBoxListener>
{
public:

    bool onSettingValueChanged(Kernel::IBox& box, const size_t index) override
    {
        CString name;
        CString value;
        box.getSettingName(index, name);
        box.getSettingValue(index, value);
        if(name == CString("psd Method")) {
            updateSettingPsdMode(box, value);
        }
        return true;
    };

private:
    static const int NUM_COMMON_SETTINGS = 6;

    void updateSettingPsdMode(Kernel::IBox& box, const CString& mode)
    {
        if (mode == CString("Welch")) {
            setPsdModeWelch(box);
        } else if (mode == CString("Burg")) {
            setPsdModeBurg(box);
        } else {
            this->getLogManager() << Kernel::LogLevel_Error<< "CBoxAlgorithmConnectivityMeasureListener::updateSettings: Unrecognized mode: " << mode << "\n";
        }
    }

    void setPsdModeBurg(Kernel::IBox& box)
    {
        // If there is more than the common settings
        while (box.getSettingCount() > NUM_COMMON_SETTINGS) {
            box.removeSetting(NUM_COMMON_SETTINGS);
        }
        box.addSetting("Auto-regressive model order", OV_TypeId_Integer, "11");

    }

    void setPsdModeWelch(Kernel::IBox& box)
    {
        // If there is more than the common settings
        while (box.getSettingCount() > NUM_COMMON_SETTINGS) {
            box.removeSetting(NUM_COMMON_SETTINGS);
        }
        box.addSetting("Windowing method", OV_TypeId_ConnectivityMeasure_WindowMethod, "Hann");
        box.addSetting("Window Length (in sec)", OV_TypeId_Float, "0.25"); // s
        box.addSetting("Windowing Overlap (in %)", OV_TypeId_Integer, "50"); // percent
    }
    _IsDerivedFromClass_Final_(Toolkit::TBoxListener<IBoxListener>, CIdentifier::undefined())
};



///
/// \brief Descriptor of the box Connectivity Measure.
///
class CBoxAlgorithmConnectivityMeasureDesc final : virtual public IBoxAlgorithmDesc
{
public:

    void release() override {}
    CString getName() const override { return "Connectivity Measure"; }
    CString getAuthorName() const override { return "Arthur DESBOIS"; }
    CString getAuthorCompanyName() const override { return "Inria"; }
    CString getShortDescription() const override { return "Connectivity Measure"; }
    CString getDetailedDescription() const override { return "Measure connectivity between pairs of channel"; }
    CString getCategory() const override { return "Signal processing/Connectivity"; }
    CString getVersion() const override { return "0.0.1"; }
    CString getStockItemName() const override { return ""; }
    CIdentifier getCreatedClass() const override { return OVP_ClassId_BoxAlgorithm_ConnectivityMeasure; }
    IPluginObject* create() override { return new CBoxAlgorithmConnectivityMeasure; }

    IBoxListener* createBoxListener() const override { return new CBoxAlgorithmConnectivityMeasureListener; }
    void releaseBoxListener(IBoxListener* listener) const override { delete listener; }

    bool getBoxPrototype(Kernel::IBoxProto& prototype) const override
    {
        prototype.addInput("Input signal", OV_TypeId_Signal);
        prototype.addOutput("Connectivity Matrix", OV_TypeId_StreamedMatrix);

        prototype.addSetting("Channel subselection", OV_TypeId_String, "");

        prototype.addSetting("Metric", OVP_TypeId_Connectivity_Metric, toString(EConnectMetric::Coherence).c_str());

        prototype.addSetting("Connectivity Measure Length (in sec)", OV_TypeId_Float, "0.5"); //s
        prototype.addSetting("Connectivity Measure Overlap (in %)", OV_TypeId_Integer, "50"); // percent
        prototype.addSetting("DC removal", OV_TypeId_Boolean, "false");
        prototype.addSetting("FFT size (frequency taps)", OV_TypeId_Integer, "128");
        prototype.addSetting("psd Method", OV_TypeId_ConnectivityMeasure_PsdMethod, toString(EPsdMode::Burg).c_str());

        // Default mode for PSD is Burg's method, add its settings
        prototype.addSetting("Auto-regressive model order", OV_TypeId_Integer, "11");

        prototype.addFlag(OV_AttributeId_Box_FlagIsUnstable);

        return true;
    }

    _IsDerivedFromClass_Final_(IBoxAlgorithmDesc, OVP_ClassId_BoxAlgorithm_ConnectivityMeasureDesc)
};

}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
