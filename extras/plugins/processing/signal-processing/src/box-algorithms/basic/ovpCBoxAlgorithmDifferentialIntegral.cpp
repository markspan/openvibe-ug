#include "ovpCBoxAlgorithmDifferentialIntegral.h"

#include <iostream>

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {

bool CBoxAlgorithmDifferentialIntegral::initialize()
{
    // Signal stream decoder
    m_decoder.initialize(*this, 0);
    // Signal stream encoder
    m_encoder.initialize(*this, 0);

    // If you need to, you can manually set the reference targets to link the codecs input and output. To do so, you can use :
    m_encoder.getInputMatrix().setReferenceTarget(m_decoder.getOutputMatrix());
    m_encoder.getInputSamplingRate().setReferenceTarget(m_decoder.getOutputSamplingRate());

    m_operation   = EDifferentialIntegralOperation(uint64_t(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0)));
    m_filterOrder = uint64_t(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1));

    return true;
}
/*******************************************************************************/

bool CBoxAlgorithmDifferentialIntegral::uninitialize()
{
    m_decoder.uninitialize();
    m_encoder.uninitialize();

    return true;
}
/*******************************************************************************/

bool CBoxAlgorithmDifferentialIntegral::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}
/*******************************************************************************/

double CBoxAlgorithmDifferentialIntegral::operation(const double a, const double b) const
{
    if (m_operation == EDifferentialIntegralOperation::Differential) { return a - b; }
    if (m_operation == EDifferentialIntegralOperation::Integral) { return a + b; }
    return 0;
}


bool CBoxAlgorithmDifferentialIntegral::process()
{
    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();

    //iterate over all chunk on input 0
    for (size_t i = 0; i < boxContext.getInputChunkCount(0); ++i)
    {
        // decode the chunk i on input 0
        m_decoder.decode(i);

        if (m_decoder.isHeaderReceived())
        {
            // initialize the past data array
            CMatrix* matrix = m_decoder.getOutputMatrix(); // the StreamedMatrix of samples.


            // initialize all of the tables according to the number of channels
            m_pastData   = new double*[matrix->getDimensionSize(0)];
            m_tmpData    = new double*[matrix->getDimensionSize(0)];
            m_stabilized = new bool[matrix->getDimensionSize(0)];
            m_step       = new size_t[matrix->getDimensionSize(0)];

            for (size_t k = 0; k < matrix->getDimensionSize(0); ++k)
            {
                m_stabilized[k] = false;
                m_step[k]       = 0;
                m_pastData[k]   = new double[m_filterOrder];
                m_tmpData[k]    = new double[m_filterOrder];
                m_tmpData[k][0] = 0;
            }

            // Encode the output header
            m_encoder.encodeHeader();
            boxContext.markOutputAsReadyToSend(0, boxContext.getInputChunkStartTime(0, i), boxContext.getInputChunkEndTime(0, i));
        }

        if (m_decoder.isBufferReceived())
        {
            CMatrix* matrix = m_decoder.getOutputMatrix(); // the StreamedMatrix of samples.

            const size_t nChannel          = matrix->getDimensionSize(0);
            const size_t samplesPerChannel = matrix->getDimensionSize(1);

            // ... do some process on the matrix ...

            double* buffer = matrix->getBuffer();

            for (size_t c = 0; c < nChannel; ++c)
            {
                for (size_t s = 0; s < samplesPerChannel; ++s)
                {
                    // save the results of the previous step in a temporary array
                    for (size_t step = 0; step < m_step[c]; ++step) { m_tmpData[c][step] = m_pastData[c][step]; }

                    // save the current sample as f^0(x)
                    m_pastData[c][0] = buffer[s + c * samplesPerChannel];

                    // save all of the f^n(x)
                    for (size_t step = 1; step < m_step[c]; ++step) { m_pastData[c][step] = operation(m_pastData[c][step - 1], m_tmpData[c][step - 1]); }

                    // if the filter is not yet stabilized we increase the step and use 0 as a return value
                    if (!m_stabilized[c])
                    {
                        if (m_step[c] == m_filterOrder) { m_stabilized[c] = true; }
                        else { m_step[c]++; }

                        buffer[s + c * samplesPerChannel] = 0;
                    }
                        // otherwise use f^order(x)
                    else { buffer[s + c * samplesPerChannel] = operation(m_pastData[c][m_filterOrder - 1], m_tmpData[c][m_filterOrder - 1]); }
                }
            }

            // Encode the output buffer
            m_encoder.encodeBuffer();
            boxContext.markOutputAsReadyToSend(0, boxContext.getInputChunkStartTime(0, i), boxContext.getInputChunkEndTime(0, i));
        }

        if (m_decoder.isEndReceived())
        {
            // End of stream received. This happens only once when pressing "stop". Just pass it to the next boxes so they receive the message :
            m_encoder.encodeEnd();
            boxContext.markOutputAsReadyToSend(0, boxContext.getInputChunkStartTime(0, i), boxContext.getInputChunkEndTime(0, i));
        }
    }

    return true;
}
}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
