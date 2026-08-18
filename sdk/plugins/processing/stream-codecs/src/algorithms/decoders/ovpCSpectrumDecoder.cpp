#include "ovpCSpectrumDecoder.h"

namespace OpenViBE {
namespace Plugins {
namespace StreamCodecs {

// ________________________________________________________________________________________________________________
//

bool CSpectrumDecoder::initialize()
{
    CStreamedMatrixDecoder::initialize();
    op_frequencyAbscissa.initialize(getOutputParameter(OVP_Algorithm_SpectrumDecoder_OutputParameterId_FrequencyAbscissa));
    op_sampling.initialize(getOutputParameter(OVP_Algorithm_SpectrumDecoder_OutputParameterId_Sampling));
    return true;
}

bool CSpectrumDecoder::uninitialize()
{
    op_frequencyAbscissa.uninitialize();
    op_sampling.uninitialize();
    CStreamedMatrixDecoder::uninitialize();
    return true;
}

// ________________________________________________________________________________________________________________
//

bool CSpectrumDecoder::isMasterChild(const EBML::CIdentifier& identifier)
{
    if (identifier == OVTK_NodeId_Header_Spectrum) { return true; }
    if (identifier == OVTK_NodeId_Header_Spectrum_FrequencyBand_Deprecated) { return true; }
    if (identifier == OVTK_NodeId_Header_Spectrum_FrequencyBand_Start_Deprecated) { return false; }
    if (identifier == OVTK_NodeId_Header_Spectrum_FrequencyBand_Stop_Deprecated) { return false; }
    if (identifier == OVTK_NodeId_Header_Spectrum_FrequencyAbscissa) { return false; }
    if (identifier == OVTK_NodeId_Header_Spectrum_Sampling) { return false; }
    return CStreamedMatrixDecoder::isMasterChild(identifier);
}

void CSpectrumDecoder::openChild(const EBML::CIdentifier& identifier)
{
    m_nodes.push(identifier);

    EBML::CIdentifier& top = m_nodes.top();

    if (top == OVTK_NodeId_Header_Spectrum)
    {
        op_frequencyAbscissa->resize(op_pMatrix->getDimensionSize(1));
        m_frequencyBandIdx = 0;
    }
    else if (top == OVTK_NodeId_Header_Spectrum_FrequencyAbscissa) { }
    else { CStreamedMatrixDecoder::openChild(identifier); }
}

void CSpectrumDecoder::processChildData(const void* buffer, const size_t size)
{
    EBML::CIdentifier& top = m_nodes.top();
    if ((top == OVTK_NodeId_Header_Spectrum) || (top == OVTK_NodeId_Header_Spectrum_FrequencyBand_Deprecated)) { }
    else if (top == OVTK_NodeId_Header_Spectrum_FrequencyBand_Start_Deprecated) { m_lowerFreq = m_readerHelper->getDouble(buffer, size); }
    else if (top == OVTK_NodeId_Header_Spectrum_FrequencyBand_Stop_Deprecated)
    {
        const double upperFreq      = m_readerHelper->getDouble(buffer, size);
        double curFrequencyAbscissa = 0;
        if (op_frequencyAbscissa->getDimensionSize(0) > 1)
        {
            // In the old format, frequencies were separated into bins with lower and upper bounds.
            // These were calculated as lowerFreq = frequencyIndex/frequencyCount, upperFreq = (frequencyIndex + 1)/frequencyCount, with 0 based indexes.
            // This formula reverses the calculation and puts the 'middle' frequency into the right place
            curFrequencyAbscissa = m_lowerFreq + double(m_frequencyBandIdx) / (op_frequencyAbscissa->getDimensionSize(0) - 1) * (upperFreq - m_lowerFreq);
        }
        op_frequencyAbscissa->getBuffer()[m_frequencyBandIdx] = curFrequencyAbscissa;
        std::ostringstream s;
        s << std::setprecision(10);
        s << curFrequencyAbscissa;
        op_pMatrix->setDimensionLabel(1, m_frequencyBandIdx, s.str().c_str());

        op_sampling = uint64_t((m_frequencyBandIdx + 1) * (upperFreq - op_frequencyAbscissa->getBuffer()[0]));
    }
    else if (top == OVTK_NodeId_Header_Spectrum_FrequencyAbscissa)
    {
        op_frequencyAbscissa->getBuffer()[m_frequencyBandIdx] = m_readerHelper->getDouble(buffer, size);
    }
    else if (top == OVTK_NodeId_Header_Spectrum_Sampling) { op_sampling = m_readerHelper->getUInt(buffer, size); }
    else { CStreamedMatrixDecoder::processChildData(buffer, size); }
}

void CSpectrumDecoder::closeChild()
{
    EBML::CIdentifier& top = m_nodes.top();

    if ((top == OVTK_NodeId_Header_Spectrum)
        || (top == OVTK_NodeId_Header_Spectrum_FrequencyBand_Start_Deprecated)
        || (top == OVTK_NodeId_Header_Spectrum_FrequencyBand_Stop_Deprecated)) { }
    else if ((top == OVTK_NodeId_Header_Spectrum_FrequencyBand_Deprecated) || (top == OVTK_NodeId_Header_Spectrum_FrequencyAbscissa)) { m_frequencyBandIdx++; }
    else { CStreamedMatrixDecoder::closeChild(); }

    m_nodes.pop();
}

}  // namespace StreamCodecs
}  // namespace Plugins
}  // namespace OpenViBE
