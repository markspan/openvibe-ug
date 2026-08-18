/*
 * Linearly superposes a simple phase-locked template around 300ms after OVTK_StimulationId_Target
 *
 * Fiddler can be used to debug P300 scenarios. Note that the same effect could be 
 * achieved with a box that tampers the signal after specific stimulation markers.
 * It can also be used as an example of how to manipulate the signal on the 
 * acquisition server side using a plugin.
 *
 * Known limitations: Overlapping patterns not handled
 *
 */

#include "ovasCPluginFiddler.h"
#include "ovasParameterHelper.h"

#include <vector>

#include <system/ovCMath.h>

#include <cmath>

namespace OpenViBE {
namespace AcquisitionServer {
namespace Plugins {

CPluginFiddler::CPluginFiddler(const Kernel::IKernelContext& ctx)
    : IAcquisitionServerPlugin(ctx, "AcquisitionServer_Plugin_Fiddler"),
    m_fiddlerStrength("Fiddler Strength","The amplitude of the pattern, 0 = disable", 0.0, 0.0, 1.0)
{
    m_kernelCtx.getLogManager() << Kernel::LogLevel_Info << "Loading plugin: Fiddler\n";
    m_parameters.push_back(&m_fiddlerStrength);
    
    loadParametersFromConfig(m_kernelCtx.getConfigurationManager(),
                             {{"Fiddler_Strength", &m_fiddlerStrength}},
                             this->m_name);
}

// Hooks


bool CPluginFiddler::startHook(const std::vector<CString>& /*selectedChannelNames*/, const size_t sampling, const size_t /*nChannel*/,
                               const size_t nSamplePerSentBlock)
{
    if (m_fiddlerStrength > 10e-06F) {
        m_nSamplePerSentBlock = nSamplePerSentBlock;

        m_StartSample      = std::numeric_limits<uint64_t>::max();
        m_EndSample        = 0;
        m_NProcessedSample = 0;
        m_Counter          = 0;

        m_Sampling = sampling;

        m_LastBufferSize = 0;

        m_kernelCtx.getLogManager() << Kernel::LogLevel_Warning << "Fiddler is enabled! Only use for debug purposes. Set strength=0 to disable.\n";
    }

    return true;
}

void CPluginFiddler::loopHook(std::deque<std::vector<float>>& buffers, CStimulationSet& stimSet, const uint64_t /*start*/, const uint64_t /*end*/,
                              const uint64_t /* sampleTime */)
{
    if (m_fiddlerStrength > 10e-06F) {
        // We need to make sure we don't process the same samples twice
        uint64_t nProcessed = 0;
        if (m_LastBufferSize > m_nSamplePerSentBlock) { nProcessed = m_LastBufferSize - m_nSamplePerSentBlock; }

        // Loop over the stimulations
        for (size_t i = 0; i < stimSet.size(); ++i) {
            const uint64_t id      = stimSet.getId(i);
            const uint64_t time    = stimSet.getDate(i);
            const uint64_t nSample = CTime(time).toSampleCount(m_Sampling);

            if (id == OVTK_StimulationId_Target && nSample > m_NProcessedSample) {
                m_StartSample = nSample + CTime(0.0).toSampleCount(m_Sampling);
                m_EndSample   = nSample + CTime(0.5).toSampleCount(m_Sampling);
                m_Counter     = 0;
            }
        }

        for (size_t i = size_t(nProcessed); i < buffers.size(); ++i) {
            // std::cout << "Sample " << CTime(l_SampleTime).toSeconds() << " range " << CTime(m_startTime).toSeconds() << " stop " <<  CTime(m_endTime).toSeconds();

            if (m_NProcessedSample > m_StartSample && m_NProcessedSample <= m_EndSample) {
                const float lobe1   = 0.25F;    // Position, in seconds
                const float lobe2   = 0.30F;
                const float spread1 = 0.008F;   // Width
                const float spread2 = 0.004F;

                // Two beta functions weighted by gaussians; the beta parameters are ^1 and ^4
                // n.b. not terribly efficient as the same pattern is created every time anew. In practice this is of little importance.
                const float st    = float(CTime(m_Sampling, m_Counter).toSeconds());
                const float bump1 = std::exp(-std::pow(st - lobe1, 2) / spread1) * (st * std::pow(1 - st, 4));
                const float bump2 = std::exp(-std::pow(st - lobe2, 2) / spread2) * (st * std::pow(1 - st, 4));
                const float value = (-0.5F * bump1 + 0.9F * bump2) * 40.0F;

                std::vector<float>& sample = buffers[i];
                float* tmp                 = &sample[0];
                for (size_t j = 0; j < sample.size(); ++j) { tmp[j] += value * m_fiddlerStrength; }
                m_Counter++;
            }

            m_NProcessedSample++;
        }

        m_LastBufferSize = buffers.size();
    }
}

}  // namespace Plugins
}  // namespace AcquisitionServer
}  // namespace OpenViBE
