#include "CSinusSignalGenerator.hpp"

#include <cmath>

namespace OpenViBE {
namespace Plugins {
namespace DataGeneration {

bool CSinusSignalGenerator::initialize()
{
    m_encoder.initialize(*this, 0);

    // Parses box settings to try connecting to server
    m_nChannel              = size_t(uint64_t(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0)));
    m_sampling              = size_t(uint64_t(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1)));
    m_nGeneratedEpochSample = size_t(uint64_t(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2)));
    m_headerSent            = false;

    if (m_nChannel == 0) {
        this->getLogManager() << Kernel::LogLevel_Error << "Channel count is 0. At least 1 channel required. Check box settings.\n";
        return false;
    }

    if (m_sampling == 0) {
        this->getLogManager() << Kernel::LogLevel_Error << "Sampling rate of 0 is not supported. Check box settings.\n";
        return false;
    }

    if (m_nGeneratedEpochSample == 0) {
        this->getLogManager() << Kernel::LogLevel_Error << "Epoch sample count is 0. An epoch must have at least 1 sample. Check box settings.\n";
        return false;
    }

    return true;
}

bool CSinusSignalGenerator::uninitialize()
{
    m_encoder.uninitialize();
    return true;
}

bool CSinusSignalGenerator::processClock(Kernel::CMessageClock& /*msg*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

bool CSinusSignalGenerator::process()
{
    Kernel::IBoxIO* boxContext = getBoxAlgorithmContext()->getDynamicBoxContext();

    if (!m_headerSent) {
        m_encoder.getInputSamplingRate() = m_sampling;

        CMatrix* matrix = m_encoder.getInputMatrix();
        matrix->resize(m_nChannel, m_nGeneratedEpochSample);

        // Convention: channel shown as users go as 1,2,...
        for (size_t i = 0; i < m_nChannel; ++i) {
            matrix->setDimensionLabel(0, i, ("sinusOsc " + std::to_string(i + 1)).c_str());
        }

        m_encoder.encodeHeader();

        m_headerSent = true;

        const uint64_t time = CTime(m_sampling, m_nSentSample).time();
        boxContext->markOutputAsReadyToSend(0, time, time);
    }
    else {
        double* buffer = m_encoder.getInputMatrix()->getBuffer();

        const size_t nSentSample = m_nSentSample;
        for (size_t i = 0; i < m_nChannel; ++i) {
            for (size_t j = 0; j < m_nGeneratedEpochSample; ++j) {
                const double coef                       = (double(j + m_nSentSample) * double(i + 1)) / double(m_sampling);
                buffer[i * m_nGeneratedEpochSample + j] = sin(coef * 12.3) + sin(coef * 4.5) + sin(coef * 67.8);
            }
        }

        m_encoder.encodeBuffer();

        m_nSentSample += m_nGeneratedEpochSample;

        const uint64_t start = CTime(m_sampling, nSentSample).time();
        const uint64_t end   = CTime(m_sampling, m_nSentSample).time();

        boxContext->markOutputAsReadyToSend(0, start, end);
    }

    return true;
}

}  // namespace DataGeneration
}  // namespace Plugins
}  // namespace OpenViBE
