#include "ovpCDecoderAlgorithmTest.h"

#include "../algorithms/decoders/ovpCExperimentInfoDecoder.h"
#include "../algorithms/decoders/ovpCFeatureVectorDecoder.h"
#include "../algorithms/decoders/ovpCSignalDecoder.h"
#include "../algorithms/decoders/ovpCSpectrumDecoder.h"
#include "../algorithms/decoders/ovpCStimulationDecoder.h"
#include "../algorithms/decoders/ovpCChannelLocalisationDecoder.h"

namespace OpenViBE {
namespace Plugins {
namespace StreamCodecs {

static std::stringstream print(CMatrix& matrix)
{
    std::stringstream ss;
    ss << "Matrix :\n";
    ss << " | Dimension count : " << matrix.getDimensionCount() << "\n";
    for (size_t i = 0; i < matrix.getDimensionCount(); ++i)
    {
        ss << " |   Dimension size " << i << " : " << matrix.getDimensionSize(i) << "\n";
        for (size_t j = 0; j < matrix.getDimensionSize(i); ++j)
        {
            ss << " |     Dimension label " << i << " " << j << " : " << matrix.getDimensionLabel(i, j) << "\n";
        }
    }
    return ss;
}

static std::stringstream print(CStimulationSet& stimSet)
{
    std::stringstream ss;
    ss << "Stimulation set :\n";
    ss << " | Number of elements : " << stimSet.size() << "\n";
    for (size_t i = 0; i < stimSet.size(); ++i)
    {
        ss << " |   Stimulation " << i << " : "
                << "id = " << stimSet.getId(i) << " "
                << "date = " << stimSet.getDate(i) << " "
                << "duration = " << stimSet.getDuration(i) << "\n";
    }
    return ss;
}

bool CDecoderAlgorithmTest::initialize()
{
    m_decoder[0] = &getAlgorithmManager().getAlgorithm(getAlgorithmManager().createAlgorithm(OVP_ClassId_Algorithm_ExperimentInfoDecoder));
    m_decoder[1] = &getAlgorithmManager().getAlgorithm(getAlgorithmManager().createAlgorithm(OVP_ClassId_Algorithm_FeatureVectorDecoder));
    m_decoder[2] = &getAlgorithmManager().getAlgorithm(getAlgorithmManager().createAlgorithm(OVP_ClassId_Algorithm_SignalDecoder));
    m_decoder[3] = &getAlgorithmManager().getAlgorithm(getAlgorithmManager().createAlgorithm(OVP_ClassId_Algorithm_SpectrumDecoder));
    m_decoder[4] = &getAlgorithmManager().getAlgorithm(getAlgorithmManager().createAlgorithm(OVP_ClassId_Algorithm_StimulationDecoder));
    m_decoder[5] = &getAlgorithmManager().getAlgorithm(getAlgorithmManager().createAlgorithm(OVP_ClassId_Algorithm_StreamedMatrixDecoder));
    m_decoder[6] = &getAlgorithmManager().getAlgorithm(getAlgorithmManager().createAlgorithm(OVP_ClassId_Algorithm_ChannelLocalisationDecoder));

    for (size_t i = 0; i < 7; ++i)
    {
        m_decoder[i]->initialize();
        ip_buffer[i].initialize(m_decoder[i]->getInputParameter(OVP_Algorithm_EBMLDecoder_InputParameterId_MemoryBufferToDecode));
    }

    return true;
}

bool CDecoderAlgorithmTest::uninitialize()
{
    for (size_t i = 0; i < 7; ++i)
    {
        ip_buffer[i].uninitialize();
        m_decoder[i]->uninitialize();
        getAlgorithmManager().releaseAlgorithm(*m_decoder[i]);
        m_decoder[i] = nullptr;
    }

    return true;
}

bool CDecoderAlgorithmTest::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

bool CDecoderAlgorithmTest::process()
{
    Kernel::IBoxIO& boxContext = getDynamicBoxContext();
    const size_t nInput        = getStaticBoxContext().getInputCount();

    for (size_t i = 0; i < nInput; ++i)
    {
        for (size_t j = 0; j < boxContext.getInputChunkCount(i); ++j)
        {
            ip_buffer[i] = boxContext.getInputChunk(i, j);
            m_decoder[i]->process();

            if (m_decoder[i]->isOutputTriggerActive(OVP_Algorithm_EBMLDecoder_OutputTriggerId_ReceivedHeader))
            {
                {
                    Kernel::TParameterHandler<CMatrix*> handler(m_decoder[i]->getOutputParameter(OVP_Algorithm_StreamedMatrixDecoder_OutputParameterId_Matrix));
                    if (handler.exists()) { OV_WARNING_K(print(*handler).str()); }
                }

                {
                    Kernel::TParameterHandler<CMatrix*> handler(m_decoder[i]->getOutputParameter(OVP_Algorithm_SpectrumDecoder_OutputParameterId_FrequencyAbscissa));
                    if (handler.exists()) { OV_WARNING_K(print(*handler).str()); }
                }

                {
                    Kernel::TParameterHandler<uint64_t> handler(m_decoder[i]->getOutputParameter(OVP_Algorithm_SignalDecoder_OutputParameterId_Sampling));
                    if (handler.exists()) { OV_WARNING_K(handler); }
                }
            }

            if (m_decoder[i]->isOutputTriggerActive(OVP_Algorithm_EBMLDecoder_OutputTriggerId_ReceivedBuffer))
            {
                {
                    Kernel::TParameterHandler<CStimulationSet*>
                            handler(m_decoder[i]->getOutputParameter(OVP_Algorithm_StimulationDecoder_OutputParameterId_StimulationSet));
                    if (handler.exists()) { getLogManager() << Kernel::LogLevel_Warning << print(*handler).str() << "\n"; }
                }
            }

            boxContext.markInputAsDeprecated(i, j);
        }
    }

    return true;
}

}  // namespace StreamCodecs
}  // namespace Plugins
}  // namespace OpenViBE
