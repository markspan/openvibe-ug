#include "ovasCDriverGenericOscillator.h"

#include "toolkit/ovtk_stimulations.h"

#include <cmath>


CDriverGenericOscillator::CDriverGenericOscillator()
    : m_paramSendPeriodicStimulations("SendPeriodicStimulations", "Send periodic stimulations", false),
      m_paramStimulationInterval("StimulationInterval", "Stimulation interval", 1.0, 0.001, DBL_MAX, 3)
{
    m_samplingRate = 256;
    m_channelCount = 4;

    m_parameters.push_back(&m_samplingRate);
    m_parameters.push_back(&m_channelCount);
    m_parameters.push_back(&m_paramSendPeriodicStimulations);
    m_parameters.push_back(&m_paramStimulationInterval);
}

void CDriverGenericOscillator::release()
{
    delete this;
}

//___________________________________________________________________//
//                                                                   //

bool CDriverGenericOscillator::init()
{
    for (size_t i = 0; i < m_channelNames.value().size(); ++i) {
        if (m_channelNames.getAt(i).empty()) {
            m_channelNames.setAt(i, "Oscillator " + std::to_string(i + 1));
        }
    }
    return true;
}

bool CDriverGenericOscillator::start()
{
    m_nTotalSample = 0;
    m_startTime    = timestamp();
    m_nTotalStim   = 0;

    return true;
}

int CDriverGenericOscillator::read(sampleType* data, uint64_t size)
{
    size_t remainingSamples = 0;

    // Generate the contents we want to send next
    const uint64_t elapsed            = timestamp() - m_startTime;  // nanoseconds
    const uint64_t samplesNeededSoFar = elapsed * uint64_t(m_samplingRate.value()) / 1'000'000'000ULL;

    if (samplesNeededSoFar > m_nTotalSample) {
        remainingSamples = std::min(size_t(samplesNeededSoFar - m_nTotalSample), size);

        for (size_t i = 0; i < remainingSamples; ++i) {
            for (size_t j = 0; j < m_channelCount.value(); ++j) {
                const double value = sin((m_nTotalSample * (j + 1) * 12.3) / m_samplingRate.value())
                                    + sin((m_nTotalSample * (j + 1) * 4.5) / m_samplingRate.value())
                                    + sin((m_nTotalSample * (j + 1) * 67.8) / m_samplingRate.value());
                data[i * m_channelCount.value() + j] = float(value);
            }
            m_nTotalSample++;
        }
    }

    // We process stimulations here to avoid calculating elapsed time twice.
    // They are pulled by the server in readStimulations method, just after this call.
    m_stimQueue.clear();
    const uint64_t intervalNs = uint64_t(m_paramStimulationInterval.value() * 1e9);
    if (m_paramSendPeriodicStimulations.value() && elapsed >= (m_nTotalStim + 1) * intervalNs) {
        const uint64_t stimDate = m_startTime + (m_nTotalStim + 1) * intervalNs;
        m_stimQueue.push_back(ovStimulation{OVTK_StimulationId_Label_00, stimDate, 0});
        m_nTotalStim++;
    }

    return remainingSamples;
}

int CDriverGenericOscillator::readStimulations(ovStimulation* buff, uint32_t maxCount)
{
    const int stimCount = m_stimQueue.size();
    int stimAdded = 0;
    for (size_t i = 0; i < stimCount && i < maxCount; ++i) {
        buff[i] = m_stimQueue.front();
        m_stimQueue.pop_front();
        stimAdded++;
    }
    return stimAdded;
}

bool CDriverGenericOscillator::stop()
{
    return true; //(m_driverCtx.isConnected() && m_driverCtx.isStarted());
}

bool CDriverGenericOscillator::unInit()
{
    return true;
}

//___________________________________________________________________//
//                                                                   //
/* @todo: how to save parameters to acquisition server settings ?
bool CDriverGenericOscillator::configure()
{
    m_driverCtx.getLogManager() << Kernel::LogLevel_Debug << "CDriverGenericOscillator::configure\n";

    return saveParametersToConfig(m_driverCtx.getConfigurationManager(),
                                  {&m_paramSendPeriodicStimulations, &m_paramStimulationInterval},
                                   "AcquisitionServer_Driver_GenericOscillator",
                                   &m_header);
}*/

bool CDriverGenericOscillator::startImpedanceCheck()
{
    m_impedanceStart = timestamp();
    return true;
}

int CDriverGenericOscillator::readImpedances(double* values, size_t count)
{
    const double elapsed = double(timestamp() - m_impedanceStart) / 1e9;
    for (size_t i = 0; i < count; ++i) {
        values[i] = 8000.0 + 4000.0 * sin(elapsed + double(i) * 1.3);
    }
    return int(count);
}

void CDriverGenericOscillator::setChunkSize(uint32_t chunkSize) {
    m_nSamplePerSentBlock = chunkSize;
}

