#include "CBoxAlgorithmNoiseGenerator.hpp"

namespace OpenViBE {
namespace Plugins {
namespace DataGeneration {

CNoiseGenerator::CNoiseGenerator()
{
    // Use the OV random seed (if specified) to get dictate the sequence here
    std::random_device device;
    m_engine = std::default_random_engine(device());
}

bool CNoiseGenerator::initialize()
{
    m_encoder.initialize(*this, 0);

    m_nChannel              = size_t(uint64_t(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0)));
    m_sampling              = size_t(uint64_t(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1)));
    m_nGeneratedEpochSample = size_t(uint64_t(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2)));
    m_noiseType             = size_t(uint64_t(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 3)));

    m_headerSent = false;

    if (m_nChannel == 0) {
        this->getLogManager() << Kernel::LogLevel_Error << "Channel count is 0. At least 1 channel is required. Check box settings.\n";
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

    // Set parameters of the encoder

    m_encoder.getInputSamplingRate() = m_sampling;

    CMatrix* sample = m_encoder.getInputMatrix();
    sample->resize(m_nChannel, m_nGeneratedEpochSample);
    for (size_t i = 0; i < m_nChannel; ++i) { sample->setDimensionLabel(0, i, ("Noise " + std::to_string(i + 1)).c_str()); }

    return true;
}

bool CNoiseGenerator::uninitialize()
{
    m_encoder.uninitialize();
    return true;
}

bool CNoiseGenerator::processClock(Kernel::CMessageClock& /*msg*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

bool CNoiseGenerator::process()
{
    Kernel::IBoxIO* boxContext = getBoxAlgorithmContext()->getDynamicBoxContext();

    // Send header?
    if (!m_headerSent) {
        m_encoder.encodeHeader();
        boxContext->markOutputAsReadyToSend(0, 0, 0);
        m_headerSent = true;
    }
    else {
        // Send buffer
        double* sample = m_encoder.getInputMatrix()->getBuffer();
        const size_t n = m_encoder.getInputMatrix()->getBufferElementCount();
        if (m_noiseType == TypeId_NoiseType_Uniform) { for (size_t i = 0; i < n; ++i) { sample[i] = double(m_uniformDistrib(m_engine)); } }
        else { for (size_t i = 0; i < n; ++i) { sample[i] = double(m_normalDistrib(m_engine)); } }

        const size_t prevNSamples = m_nSentSample;
        m_nSentSample             = prevNSamples + m_nGeneratedEpochSample;

        const uint64_t start = CTime(m_sampling, prevNSamples).time();
        const uint64_t end   = CTime(m_sampling, m_nSentSample).time();

        m_encoder.encodeBuffer();

        boxContext->markOutputAsReadyToSend(0, start, end);
    }

    return true;
}

uint64_t CNoiseGenerator::getClockFrequency()
{
    // Intentional parameter swap to get the frequency
    return CTime(m_nGeneratedEpochSample, m_sampling).time();
}
}  // namespace DataGeneration
}  // namespace Plugins
}  // namespace OpenViBE
