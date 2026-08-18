//include OpenViBE
#include "ovpCBoxAlgorithmQuadraticForm.h"

//include C++ STL
#include <iostream>
#include <sstream>
//atoi

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {

bool CBoxAlgorithmQuadraticForm::initialize()
{
    //the algorithms that decode and encode the signals
    m_decoder = &getAlgorithmManager().getAlgorithm(getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_SignalDecoder));
    m_encoder = &getAlgorithmManager().getAlgorithm(getAlgorithmManager().createAlgorithm(OVP_GD_ClassId_Algorithm_SignalEncoder));
    m_decoder->initialize();
    m_encoder->initialize();
    m_decoder->getOutputParameter(OVP_GD_Algorithm_SignalDecoder_OutputParameterId_Sampling)->setReferenceTarget(
        m_encoder->getInputParameter(OVP_GD_Algorithm_SignalEncoder_InputParameterId_Sampling));

    //connecting the decoding and encoding the parameters
    m_iEBMLBufferHandle.initialize(m_decoder->getInputParameter(OVP_GD_Algorithm_SignalDecoder_InputParameterId_MemoryBufferToDecode));
    m_iMatrixHandle.initialize(m_decoder->getOutputParameter(OVP_GD_Algorithm_StreamedMatrixDecoder_OutputParameterId_Matrix));
    m_oMatrixHandle.initialize(m_encoder->getInputParameter(OVP_GD_Algorithm_StreamedMatrixEncoder_InputParameterId_Matrix));
    m_oEBMLBufferHandle.initialize(m_encoder->getOutputParameter(OVP_GD_Algorithm_SignalEncoder_OutputParameterId_EncodedMemoryBuffer));

    //end and start time
    m_startTime = 0;
    m_endTime   = 0;

    //reading the quadratic operator (matrix) values

    //the number of rows/columns
    const size_t nRow = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);

    //setting the size of the matrix
    m_quadraticOperator.resize(nRow, nRow);

    //the coefficients
    const CString coefs = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
    const char* str     = coefs.toASCIIString();
    double* buffer      = m_quadraticOperator.getBuffer();

    std::istringstream iss(str); //the stream for parsing the matrix coefficient
    double currentValue = 0.0; //the current coefficient being read

    for (size_t i = 0; i < nRow; ++i)
    {
        for (size_t j = 0; j < nRow; ++j)
        {
            //actual parsing, checking and storing value if everything is OK
            if (!(iss >> currentValue))
            {
                getBoxAlgorithmContext()->getPlayerContext()->getLogManager() << Kernel::LogLevel_Error <<
                        "Error reading quadratic operator coefficients\n The coefficients or the number of coefficient must be wrong\n";
                return false;
            }
            buffer[i * nRow + j] = currentValue;
        }
    }

    if (iss >> currentValue)
    {
        getBoxAlgorithmContext()->getPlayerContext()->getLogManager() << Kernel::LogLevel_Warning <<
                "There may be more coefficients specified in the setting 'Matrix values' than the number of rows/columns can allow\n";
    }

    return true;
}

bool CBoxAlgorithmQuadraticForm::uninitialize()
{
    //uninitializing algorithms and parameters handlers
    m_decoder->uninitialize();
    m_encoder->uninitialize();
    m_iEBMLBufferHandle.uninitialize();
    m_oEBMLBufferHandle.uninitialize();
    m_iMatrixHandle.uninitialize();
    m_oMatrixHandle.uninitialize();

    //releasing algorithms
    getAlgorithmManager().releaseAlgorithm(*m_decoder);
    getAlgorithmManager().releaseAlgorithm(*m_encoder);

    return true;
}

bool CBoxAlgorithmQuadraticForm::processInput(const size_t /*index*/)
{
    //if input is arrived, processing it, i.e., computing the corresponding quadratic forms
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

bool CBoxAlgorithmQuadraticForm::process()
{
    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();

    //prcessing the input buffers
    for (size_t i = 0; i < boxContext.getInputChunkCount(0); ++i)
    {
        //decoding the input signal
        m_iEBMLBufferHandle = boxContext.getInputChunk(0, i);
        m_decoder->process();
        //storing the start and end time of the chunk
        m_startTime = boxContext.getInputChunkStartTime(0, i);
        m_endTime   = boxContext.getInputChunkEndTime(0, i);

        //deal with the header if needed (initializations)
        if (m_decoder->isOutputTriggerActive(OVP_GD_Algorithm_SignalDecoder_OutputTriggerId_ReceivedHeader))
        {
            //getting some input matrix properties
            m_nChannels         = m_iMatrixHandle->getDimensionSize(0);
            m_nSamplesPerBuffer = m_iMatrixHandle->getDimensionSize(1);

            //checking that the number of channels is compatible with the quadratic operator size
            if (m_nChannels != m_quadraticOperator.getDimensionSize(0))
            {
                getBoxAlgorithmContext()->getPlayerContext()->getLogManager() << Kernel::LogLevel_Error <<
                        "The number of input channels is not compatible with the number of rows/columns of the quadratic operator matrix. This number of rows/columns must be equal to the number of input channels\n";
                return false;
            }

            //setting the size of the output buffer
            m_oMatrixHandle->resize(1, m_nSamplesPerBuffer);

            m_oEBMLBufferHandle = boxContext.getOutputChunk(0);
            //encoding the output
            m_encoder->process(OVP_GD_Algorithm_SignalEncoder_InputTriggerId_EncodeHeader);
            //sending the output
            boxContext.markOutputAsReadyToSend(0, m_startTime, m_endTime);
        }

        //applying the quadratic operator
        if (m_decoder->isOutputTriggerActive(OVP_GD_Algorithm_SignalDecoder_OutputTriggerId_ReceivedBuffer))
        {
            double* buffer  = m_quadraticOperator.getBuffer();
            double* iBuffer = m_iMatrixHandle->getBuffer();
            double* oBuffer = m_oMatrixHandle->getBuffer();

            //applying the quadratic operator for each sample: o = m^T * A * m
            for (size_t j = 0; j < m_nSamplesPerBuffer; ++j)
            {
                std::vector<double> prime(m_nChannels); //performing m' = A * m (intermediate step 1)
                for (size_t k = 0; k < prime.size(); ++k) { prime[k] = 0.0; }   //initializing with zeros

                //browsing the quadratic operator matrix (A) rows
                for (size_t k = 0; k < m_quadraticOperator.getDimensionSize(0); ++k)
                {
                    //browsing the quadratic operator matrix (A) columns
                    for (size_t l = 0; l < m_quadraticOperator.getDimensionSize(1); ++l)
                    {
                        prime[k] += buffer[k * m_quadraticOperator.getDimensionSize(0) + l] * iBuffer[l * m_nChannels + j];
                    }
                }

                //performing o = m^T * m' (intermediate step 2)
                double output = 0.0;
                for (size_t k = 0; k < prime.size(); ++k) { output += iBuffer[k * m_nChannels + j] * prime[k]; }

                oBuffer[j] = output;
            }

            boxContext.markInputAsDeprecated(0, i);
            m_oEBMLBufferHandle = boxContext.getOutputChunk(0);
            m_encoder->process(OVP_GD_Algorithm_SignalEncoder_InputTriggerId_EncodeBuffer);
            boxContext.markOutputAsReadyToSend(0, m_startTime, m_endTime);
        }
    }

    return true;
}

}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
