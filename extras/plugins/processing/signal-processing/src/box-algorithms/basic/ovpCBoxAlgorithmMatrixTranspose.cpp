#include "ovpCBoxAlgorithmMatrixTranspose.h"

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {

bool CBoxAlgorithmMatrixTranspose::initialize()
{
    m_decoder.initialize(*this, 0);
    m_encoder.initialize(*this, 0);
    return true;
}

bool CBoxAlgorithmMatrixTranspose::uninitialize()
{
    m_encoder.uninitialize();
    m_decoder.uninitialize();
    return true;
}

bool CBoxAlgorithmMatrixTranspose::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

bool CBoxAlgorithmMatrixTranspose::process()
{
    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();

    for (size_t i = 0; i < boxContext.getInputChunkCount(0); ++i)
    {
        m_decoder.decode(i);

        if (m_decoder.isHeaderReceived())
        {
            const size_t nDim = m_decoder.getOutputMatrix()->getDimensionCount();

            const CMatrix* input = m_decoder.getOutputMatrix();
            CMatrix* output      = m_encoder.getInputMatrix();

            if (nDim == 1)
            {
                this->getLogManager() << Kernel::LogLevel_Trace << "Upgrading your 1 dimensional matrix to 2 dimensions, [" << input->getDimensionSize(0) <<
                        "x 1]\n";

                output->resize(input->getDimensionSize(0), 1);

                for (size_t j = 0; j < input->getDimensionSize(0); ++j) { output->setDimensionLabel(0, j, input->getDimensionLabel(0, j)); }
                output->setDimensionLabel(1, 0, "Dimension 0");
            }
            else if (nDim == 2)
            {
                output->resize(input->getDimensionSize(1), input->getDimensionSize(0));

                for (size_t j = 0; j < output->getDimensionSize(0); ++j) { output->setDimensionLabel(0, j, input->getDimensionLabel(1, j)); }
                for (size_t j = 0; j < output->getDimensionSize(1); ++j) { output->setDimensionLabel(1, j, input->getDimensionLabel(0, j)); }
            }
            else
            {
                this->getLogManager() << Kernel::LogLevel_Error << "Only 1 and 2 dimensional matrices supported\n";
                return false;
            }

            this->getLogManager() << Kernel::LogLevel_Trace << "Output matrix will be [" << output->getDimensionSize(0) << "x" << output->getDimensionSize(1) << "]\n";

            m_encoder.encodeHeader();
            boxContext.markOutputAsReadyToSend(0, boxContext.getInputChunkStartTime(0, i), boxContext.getInputChunkEndTime(0, i));
        }

        if (m_decoder.isBufferReceived())
        {
            const CMatrix* input = m_decoder.getOutputMatrix();
            CMatrix* output      = m_encoder.getInputMatrix();

            if (input->getDimensionCount() == 1)
            {
                const double* iBuffer = input->getBuffer();
                double* oBuffer       = output->getBuffer();

                for (size_t j = 0; j < input->getBufferElementCount(); ++j) { oBuffer[j] = iBuffer[j]; }
            }
            else
            {
                // 2 dim
                const size_t nRows = input->getDimensionSize(0);
                const size_t nCols = input->getDimensionSize(1);

                const double* iBuffer = input->getBuffer();
                double* oBuffer       = output->getBuffer();

                for (size_t j = 0; j < nRows; ++j) { for (size_t k = 0; k < nCols; ++k) { oBuffer[k * nRows + j] = iBuffer[j * nCols + k]; } }
            }

            m_encoder.encodeBuffer();
            boxContext.markOutputAsReadyToSend(0, boxContext.getInputChunkStartTime(0, i), boxContext.getInputChunkEndTime(0, i));
        }


        if (m_decoder.isEndReceived())
        {
            m_encoder.encodeEnd();
            boxContext.markOutputAsReadyToSend(0, boxContext.getInputChunkStartTime(0, i), boxContext.getInputChunkEndTime(0, i));
        }
    }

    return true;
}
}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
