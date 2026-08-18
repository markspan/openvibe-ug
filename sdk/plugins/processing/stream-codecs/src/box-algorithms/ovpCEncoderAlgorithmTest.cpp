#include "ovpCEncoderAlgorithmTest.h"

#include "../algorithms/encoders/ovpCExperimentInfoEncoder.h"
#include "../algorithms/encoders/ovpCFeatureVectorEncoder.h"
#include "../algorithms/encoders/ovpCSignalEncoder.h"
#include "../algorithms/encoders/ovpCSpectrumEncoder.h"
#include "../algorithms/encoders/ovpCStimulationEncoder.h"
#include "../algorithms/encoders/ovpCChannelLocalisationEncoder.h"

namespace OpenViBE {
namespace Plugins {
namespace StreamCodecs {

bool CEncoderAlgorithmTest::initialize()
{
    m_encoders[0] = &getAlgorithmManager().getAlgorithm(getAlgorithmManager().createAlgorithm(OVP_ClassId_Algorithm_ExperimentInfoEncoder));
    m_encoders[1] = &getAlgorithmManager().getAlgorithm(getAlgorithmManager().createAlgorithm(OVP_ClassId_Algorithm_FeatureVectorEncoder));
    m_encoders[2] = &getAlgorithmManager().getAlgorithm(getAlgorithmManager().createAlgorithm(OVP_ClassId_Algorithm_SignalEncoder));
    m_encoders[3] = &getAlgorithmManager().getAlgorithm(getAlgorithmManager().createAlgorithm(OVP_ClassId_Algorithm_SpectrumEncoder));
    m_encoders[4] = &getAlgorithmManager().getAlgorithm(getAlgorithmManager().createAlgorithm(OVP_ClassId_Algorithm_StimulationEncoder));
    m_encoders[5] = &getAlgorithmManager().getAlgorithm(getAlgorithmManager().createAlgorithm(OVP_ClassId_Algorithm_StreamedMatrixEncoder));
    m_encoders[6] = &getAlgorithmManager().getAlgorithm(getAlgorithmManager().createAlgorithm(OVP_ClassId_Algorithm_ChannelLocalisationEncoder));

    for (size_t i = 0; i < 7; ++i)
    {
        m_encoders[i]->initialize();
        op_buffer[i].initialize(m_encoders[i]->getOutputParameter(OVP_Algorithm_EBMLEncoder_OutputParameterId_EncodedMemoryBuffer));
    }

    m_matrix1 = new CMatrix();
    m_matrix1->resize(16, 16);

    m_matrix2 = new CMatrix();
    m_matrix2->resize(16);

    m_matrix3 = new CMatrix();
    m_matrix3->resize(4, 3);
    m_matrix3->setDimensionLabel(0, 0, "C3");
    m_matrix3->setDimensionLabel(0, 1, "Cz");
    m_matrix3->setDimensionLabel(0, 2, "C4");
    m_matrix3->setDimensionLabel(0, 3, "Pz");
    m_matrix3->setDimensionLabel(1, 0, "x");
    m_matrix3->setDimensionLabel(1, 1, "y");
    m_matrix3->setDimensionLabel(1, 2, "z");

    m_stimSet = new CStimulationSet();

    size_t frequency = 16;

    m_encoders[1]->getInputParameter(OVP_Algorithm_StreamedMatrixEncoder_InputParameterId_Matrix)->setValue(&m_matrix1);
    m_encoders[2]->getInputParameter(OVP_Algorithm_StreamedMatrixEncoder_InputParameterId_Matrix)->setValue(&m_matrix1);
    m_encoders[2]->getInputParameter(OVP_Algorithm_SignalEncoder_InputParameterId_Sampling)->setValue(&frequency);
    m_encoders[3]->getInputParameter(OVP_Algorithm_StreamedMatrixEncoder_InputParameterId_Matrix)->setValue(&m_matrix1);
    m_encoders[3]->getInputParameter(OVP_Algorithm_SpectrumEncoder_InputParameterId_FrequencyAbscissa)->setValue(&m_matrix2);
    m_encoders[4]->getInputParameter(OVP_Algorithm_StimulationEncoder_InputParameterId_StimulationSet)->setValue(&m_stimSet);
    m_encoders[5]->getInputParameter(OVP_Algorithm_StreamedMatrixEncoder_InputParameterId_Matrix)->setValue(&m_matrix2);
    m_encoders[6]->getInputParameter(OVP_Algorithm_StreamedMatrixEncoder_InputParameterId_Matrix)->setValue(&m_matrix3);

    m_hasSentHeader = false;
    m_startTime     = 0;
    m_endTime       = 0;

    return true;
}

bool CEncoderAlgorithmTest::uninitialize()
{
    delete m_stimSet;
    delete m_matrix3;
    delete m_matrix2;
    delete m_matrix1;

    for (size_t i = 0; i < 7; ++i)
    {
        op_buffer[i].uninitialize();
        m_encoders[i]->uninitialize();
        getAlgorithmManager().releaseAlgorithm(*m_encoders[i]);
        m_encoders[i] = nullptr;
    }

    return true;
}

bool CEncoderAlgorithmTest::processClock(Kernel::CMessageClock& /*msg*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

bool CEncoderAlgorithmTest::process()
{
    Kernel::IBoxIO& boxContext            = getDynamicBoxContext();
    Kernel::IPlayerContext& playerContext = getPlayerContext();
    const size_t nInput                   = getStaticBoxContext().getOutputCount();

    if (!m_hasSentHeader)
    {
        m_startTime = 0;
        m_endTime   = 0;
        for (size_t i = 0; i < nInput; ++i)
        {
            op_buffer[i] = boxContext.getOutputChunk(i);
            m_encoders[i]->process(OVP_Algorithm_EBMLEncoder_InputTriggerId_EncodeHeader);
        }
        m_hasSentHeader = true;
    }
    else
    {
        for (size_t i = 0; i < nInput; ++i)
        {
            op_buffer[i] = boxContext.getOutputChunk(i);
            m_encoders[i]->process(OVP_Algorithm_EBMLEncoder_InputTriggerId_EncodeBuffer);
        }
    }

    for (size_t i = 0; i < nInput; ++i) { boxContext.markOutputAsReadyToSend(i, m_startTime, m_endTime); }

    m_startTime = m_endTime;
    m_endTime   = playerContext.getCurrentTime();

    return true;
}

}  // namespace StreamCodecs
}  // namespace Plugins
}  // namespace OpenViBE
