#include "ovpCSpectrumEncoder.h"

namespace OpenViBE {
namespace Plugins {
namespace StreamCodecs {

bool CSpectrumEncoder::initialize()
{
    CStreamedMatrixEncoder::initialize();
    ip_frequencyAbscissa.initialize(getInputParameter(OVP_Algorithm_SpectrumEncoder_InputParameterId_FrequencyAbscissa));
    ip_sampling.initialize(getInputParameter(OVP_Algorithm_SpectrumEncoder_InputParameterId_Sampling));
    return true;
}

bool CSpectrumEncoder::uninitialize()
{
    ip_frequencyAbscissa.uninitialize();
    ip_sampling.uninitialize();

    CStreamedMatrixEncoder::uninitialize();

    return true;
}

// ________________________________________________________________________________________________________________
//

bool CSpectrumEncoder::processHeader()
{
    // ip_frequencyAbscissa dimension count should be 1
    // ip_frequencyAbscissa dimension size 0 should be the same as streamed matrix dimension size 1

    CMatrix* frequencyAbscissa = ip_frequencyAbscissa;
    const uint64_t sampling    = ip_sampling;
    CStreamedMatrixEncoder::processHeader();
    m_writerHelper->openChild(OVTK_NodeId_Header_Spectrum);
    m_writerHelper->openChild(OVTK_NodeId_Header_Spectrum_Sampling);
    m_writerHelper->setUInt(sampling);
    m_writerHelper->closeChild();
    for (size_t i = 0; i < frequencyAbscissa->getDimensionSize(0); ++i)
    {
        m_writerHelper->openChild(OVTK_NodeId_Header_Spectrum_FrequencyAbscissa);
        m_writerHelper->setDouble(frequencyAbscissa->getBuffer()[i]);
        m_writerHelper->closeChild();
    }
    m_writerHelper->closeChild();

    return true;
}

}  // namespace StreamCodecs
}  // namespace Plugins
}  // namespace OpenViBE
