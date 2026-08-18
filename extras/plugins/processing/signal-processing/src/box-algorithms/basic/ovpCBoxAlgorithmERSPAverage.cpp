#include "ovpCBoxAlgorithmERSPAverage.h"

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {

bool CBoxAlgorithmERSPAverage::initialize()
{
    m_epochingStim = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
    m_computeStim  = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);

    m_decoderSpectrum.initialize(*this, 0);
    m_decoderStimulations.initialize(*this, 1);

    m_encoder.initialize(*this, 0);


    return true;
}

bool CBoxAlgorithmERSPAverage::uninitialize()
{
    m_encoder.uninitialize();
    m_decoderSpectrum.uninitialize();
    m_decoderStimulations.uninitialize();

    for (auto& v : m_cachedSpectra) { for (auto m : v) { delete m; } }
    m_cachedSpectra.clear();

    return true;
}

bool CBoxAlgorithmERSPAverage::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

bool CBoxAlgorithmERSPAverage::process()
{
    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();

    for (size_t i = 0; i < boxContext.getInputChunkCount(1); ++i) {
        m_decoderStimulations.decode(i);
        if (m_decoderStimulations.isBufferReceived()) {
            const auto stims = m_decoderStimulations.getOutputStimulationSet();
            for (size_t j = 0; j < stims->size(); ++j) {
                if (stims->getId(j) == m_epochingStim) {
                    m_currentChunk = 0;
                    m_numTrials++;
                }
                if (stims->getId(j) == m_computeStim) { computeAndSend(); }
            }
        }
    }

    for (size_t i = 0; i < boxContext.getInputChunkCount(0); ++i) {
        m_decoderSpectrum.decode(i);

        if (m_decoderSpectrum.isHeaderReceived()) {
            const uint64_t samplingRate      = m_decoderSpectrum.getOutputSamplingRate();
            m_encoder.getInputSamplingRate() = samplingRate;
            m_encoder.getInputFrequencyAbscissa()->copy(*m_decoderSpectrum.getOutputFrequencyAbscissa());
            m_encoder.getInputMatrix()->copyDescription(*m_decoderSpectrum.getOutputMatrix());
            m_encoder.encodeHeader();
            boxContext.markOutputAsReadyToSend(0, boxContext.getInputChunkStartTime(0, i), boxContext.getInputChunkEndTime(0, i));
        }

        if (m_decoderSpectrum.isBufferReceived()) {
            const CMatrix* input = m_decoderSpectrum.getOutputMatrix();
            appendChunk(*input, boxContext.getInputChunkStartTime(0, i), boxContext.getInputChunkEndTime(0, i));
        }

        if (m_decoderSpectrum.isEndReceived()) {
            m_encoder.encodeEnd();
            boxContext.markOutputAsReadyToSend(0, boxContext.getInputChunkStartTime(0, i), boxContext.getInputChunkEndTime(0, i));
        }
    }

    return true;
}

bool CBoxAlgorithmERSPAverage::appendChunk(const CMatrix& chunk, const uint64_t startTime, const uint64_t endTime)
{
    if (m_cachedSpectra.size() <= m_currentChunk) {
        m_cachedSpectra.resize(m_currentChunk + 1);
        m_timestamps.resize(m_currentChunk + 1);
    }

    CMatrix* matrixCopy = new CMatrix();
    matrixCopy->copy(chunk);
    m_cachedSpectra[m_currentChunk].push_back(matrixCopy);
    m_timestamps[m_currentChunk].start = startTime;
    m_timestamps[m_currentChunk].end   = endTime;

    m_currentChunk++;

    return true;
}

bool CBoxAlgorithmERSPAverage::computeAndSend()
{
    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();

    double* outptr = m_encoder.getInputMatrix()->getBuffer();

    this->getLogManager() << Kernel::LogLevel_Info << "Counted " << m_numTrials << " trials and " << m_cachedSpectra.size() << " spectra per trial.\n";

    for (size_t i = 0; i < m_cachedSpectra.size(); ++i) {
        // Compute average for each slice
        const double divider = 1.0 / double(m_cachedSpectra[i].size());
        m_encoder.getInputMatrix()->resetBuffer();

        for (auto mat : m_cachedSpectra[i]) {
            const double* inptr = mat->getBuffer();
            for (size_t p = 0; p < mat->getBufferElementCount(); ++p) { outptr[p] += divider * inptr[p]; }
            delete mat;
        }

        m_encoder.encodeBuffer();
        boxContext.markOutputAsReadyToSend(0, m_timestamps[i].start - m_timestamps[0].start, m_timestamps[i].end - m_timestamps[0].start);
    }

    m_numTrials    = 0;
    m_currentChunk = 0;
    m_cachedSpectra.clear();

    return true;
}
}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
