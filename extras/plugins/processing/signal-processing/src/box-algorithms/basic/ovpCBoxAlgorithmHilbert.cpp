#include "ovpCBoxAlgorithmHilbert.h"

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {

bool CBoxAlgorithmHilbert::initialize()
{
    // Signal stream decoder
    m_decoder.initialize(*this, 0);
    // Signal stream encoder
    m_algo1Encoder.initialize(*this, 0);
    m_algo2Encoder.initialize(*this, 1);
    m_algo3Encoder.initialize(*this, 2);

    m_hilbertAlgo = &this->getAlgorithmManager().getAlgorithm(this->getAlgorithmManager().createAlgorithm(OVP_ClassId_Algorithm_HilbertTransform));
    m_hilbertAlgo->initialize();

    ip_signalMatrix.initialize(m_hilbertAlgo->getInputParameter(OVP_Algorithm_HilbertTransform_InputParameterId_Matrix));
    op_hilbertMatrix.initialize(m_hilbertAlgo->getOutputParameter(OVP_Algorithm_HilbertTransform_OutputParameterId_HilbertMatrix));
    op_envelopeMatrix.initialize(m_hilbertAlgo->getOutputParameter(OVP_Algorithm_HilbertTransform_OutputParameterId_EnvelopeMatrix));
    op_phaseMatrix.initialize(m_hilbertAlgo->getOutputParameter(OVP_Algorithm_HilbertTransform_OutputParameterId_PhaseMatrix));

    ip_signalMatrix.setReferenceTarget(m_decoder.getOutputMatrix());

    m_algo1Encoder.getInputSamplingRate().setReferenceTarget(m_decoder.getOutputSamplingRate());
    m_algo2Encoder.getInputSamplingRate().setReferenceTarget(m_decoder.getOutputSamplingRate());
    m_algo3Encoder.getInputSamplingRate().setReferenceTarget(m_decoder.getOutputSamplingRate());

    m_algo1Encoder.getInputMatrix().setReferenceTarget(op_hilbertMatrix);
    m_algo2Encoder.getInputMatrix().setReferenceTarget(op_envelopeMatrix);
    m_algo3Encoder.getInputMatrix().setReferenceTarget(op_phaseMatrix);

    return true;
}
/*******************************************************************************/

bool CBoxAlgorithmHilbert::uninitialize()
{
    m_decoder.uninitialize();
    m_algo1Encoder.uninitialize();
    m_algo2Encoder.uninitialize();
    m_algo3Encoder.uninitialize();

    ip_signalMatrix.uninitialize();
    op_hilbertMatrix.uninitialize();
    op_envelopeMatrix.uninitialize();
    op_phaseMatrix.uninitialize();

    m_hilbertAlgo->uninitialize();
    this->getAlgorithmManager().releaseAlgorithm(*m_hilbertAlgo);

    return true;
}
/*******************************************************************************/


bool CBoxAlgorithmHilbert::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}
/*******************************************************************************/

bool CBoxAlgorithmHilbert::process()
{
    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();

    //iterate over all chunk on input 0
    for (size_t i = 0; i < boxContext.getInputChunkCount(0); ++i)
    {
        // decode the chunk i on input 0
        m_decoder.decode(i);
        // the decoder may have decoded 3 different parts : the header, a buffer or the end of stream.
        if (m_decoder.isHeaderReceived())
        {
            // Header received
            m_hilbertAlgo->process(OVP_Algorithm_HilbertTransform_InputTriggerId_Initialize);

            // Pass the header to the next boxes, by encoding a header on the output 0:
            m_algo1Encoder.encodeHeader();
            m_algo2Encoder.encodeHeader();
            m_algo3Encoder.encodeHeader();

            // send the output chunk containing the header. The dates are the same as the input chunk:
            boxContext.markOutputAsReadyToSend(0, boxContext.getInputChunkStartTime(0, i), boxContext.getInputChunkEndTime(0, i));
            boxContext.markOutputAsReadyToSend(1, boxContext.getInputChunkStartTime(0, i), boxContext.getInputChunkEndTime(0, i));
            boxContext.markOutputAsReadyToSend(2, boxContext.getInputChunkStartTime(0, i), boxContext.getInputChunkEndTime(0, i));
        }
        if (m_decoder.isBufferReceived())
        {
            m_hilbertAlgo->process(OVP_Algorithm_HilbertTransform_InputTriggerId_Process);

            // Encode the output buffer :
            m_algo1Encoder.encodeBuffer();
            m_algo2Encoder.encodeBuffer();
            m_algo3Encoder.encodeBuffer();

            // and send it to the next boxes :
            boxContext.markOutputAsReadyToSend(0, boxContext.getInputChunkStartTime(0, i), boxContext.getInputChunkEndTime(0, i));
            boxContext.markOutputAsReadyToSend(1, boxContext.getInputChunkStartTime(0, i), boxContext.getInputChunkEndTime(0, i));
            boxContext.markOutputAsReadyToSend(2, boxContext.getInputChunkStartTime(0, i), boxContext.getInputChunkEndTime(0, i));
        }
        if (m_decoder.isEndReceived())
        {
            // End of stream received. This happens only once when pressing "stop". Just pass it to the next boxes so they receive the message :
            m_algo1Encoder.encodeEnd();
            m_algo2Encoder.encodeEnd();
            m_algo3Encoder.encodeEnd();

            boxContext.markOutputAsReadyToSend(0, boxContext.getInputChunkStartTime(0, i), boxContext.getInputChunkEndTime(0, i));
            boxContext.markOutputAsReadyToSend(1, boxContext.getInputChunkStartTime(0, i), boxContext.getInputChunkEndTime(0, i));
            boxContext.markOutputAsReadyToSend(2, boxContext.getInputChunkStartTime(0, i), boxContext.getInputChunkEndTime(0, i));
        }

        // The current input chunk has been processed, and automaticcaly discarded.
        // you don't need to call "boxContext.markInputAsDeprecated(0, i);"
    }


    return true;
}
}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
