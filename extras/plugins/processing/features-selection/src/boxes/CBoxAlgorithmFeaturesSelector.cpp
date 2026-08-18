#include "CBoxAlgorithmFeaturesSelector.hpp"

namespace OpenViBE {
namespace Plugins {
namespace FeaturesSelection {

///-------------------------------------------------------------------------------------------------
bool CBoxAlgorithmFeaturesSelector::initialize()
{
    m_decoder.initialize(*this, 0);
    m_iMatrix = m_decoder.getOutputMatrix();
    m_encoder.initialize(*this, 0);
    m_oMatrix = m_encoder.getInputMatrix();
    m_lookup.clear();
    return true;
}
///-------------------------------------------------------------------------------------------------

///-------------------------------------------------------------------------------------------------
bool CBoxAlgorithmFeaturesSelector::uninitialize()
{
    m_decoder.uninitialize();
    m_encoder.uninitialize();
    m_lookup.clear();
    return true;
}
///-------------------------------------------------------------------------------------------------

///-------------------------------------------------------------------------------------------------
bool CBoxAlgorithmFeaturesSelector::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}
///-------------------------------------------------------------------------------------------------

///-------------------------------------------------------------------------------------------------
bool CBoxAlgorithmFeaturesSelector::process()
{
    Kernel::IBoxIO& boxCtx = this->getDynamicBoxContext();

    for (size_t i = 0; i < boxCtx.getInputChunkCount(0); ++i)
    {
        m_decoder.decode(i);                                        // Decode the chunk
        if (m_decoder.isHeaderReceived())                           // Header Received
        {
            // Parse Setting
            const std::string setting = CString(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0)).toASCIIString();
            OV_ERROR_UNLESS_KRF(parseSetting(setting), "Parsing has failed", Kernel::ErrorType::BadSetting);
            OV_ERROR_UNLESS_KRF(!m_lookup.empty(), "No channel selected", Kernel::ErrorType::BadConfig);
            getLogManager() << Kernel::LogLevel_Debug << "Features selected :";
            for (const auto& r : m_lookup) { getLogManager() << " " << r; }
            getLogManager() << "\n";

            // Initialize Output Matrix
            m_oMatrix->resize(m_lookup.size());
            m_oMatrix->resetBuffer();
            for (size_t j = 0; j < m_lookup.size(); ++j) { m_oMatrix->setDimensionLabel(0, j, m_iMatrix->getDimensionLabel(0, m_lookup[j])); }
            m_encoder.encodeHeader();
        }
        if (m_decoder.isBufferReceived())                           // Buffer Received
        {
            const double* iBuffer = m_iMatrix->getBuffer();
            double* oBuffer       = m_oMatrix->getBuffer();
            for (size_t j = 0; j < m_lookup.size(); ++j) { memcpy(oBuffer + j, iBuffer + m_lookup[j], sizeof(double)); }
            m_encoder.encodeBuffer();
        }
        if (m_decoder.isEndReceived()) { m_encoder.encodeEnd(); }   // End Received

        boxCtx.markOutputAsReadyToSend(0, boxCtx.getInputChunkStartTime(0, i), boxCtx.getInputChunkEndTime(0, i));
    }
    return true;
}

bool CBoxAlgorithmFeaturesSelector::parseSetting(const std::string& setting)
{
    OV_ERROR_UNLESS_KRF(!setting.empty(), "Empty Setting is forbidden", Kernel::ErrorType::BadSetting);
    std::stringstream ss(setting);
    std::string token;
    std::vector<std::string> list;
    while (std::getline(ss, token, OV_Value_EnumeratedStringSeparator)) { list.push_back(token); }
    // Define Min Max
    const size_t startIdx = 0, endIdx = m_iMatrix->getDimensionSize(0); // never update in case of duplication or changing order
    size_t idx1, idx2;
    char sep;
    // For each token
    for (const auto& tmp : list)
    {
        ss.clear();
        ss.str(tmp);
        // Check if it's range
        const std::size_t pos = tmp.find(OV_Value_RangeStringSeparator);
        if (pos != std::string::npos)
        {
            if (tmp[0] == OV_Value_RangeStringSeparator) { idx1 = startIdx; }
            else { ss >> idx1; }
            ss >> sep;
            if (tmp[tmp.length() - 1] == OV_Value_RangeStringSeparator) { idx2 = endIdx; }
            else { ss >> idx2; }
            OV_ERROR_UNLESS_KRF((idx1 < idx2 && idx2 <= endIdx), "Invalid Range String.", Kernel::ErrorType::BadSetting);
            for (size_t i = idx1; i < idx2; ++i) { m_lookup.push_back(i); }
        }
        else
        {
            ss >> idx1;
            OV_ERROR_UNLESS_KRF(idx1 <= endIdx, "Invalid Index.", Kernel::ErrorType::BadSetting);
            m_lookup.push_back(idx1);
        }
    }

    return true;
}
///-------------------------------------------------------------------------------------------------
}  // namespace FeaturesSelection
}  // namespace Plugins
}  // namespace OpenViBE
