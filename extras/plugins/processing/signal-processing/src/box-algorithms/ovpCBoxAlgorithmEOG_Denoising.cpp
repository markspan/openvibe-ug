#include "ovpCBoxAlgorithmEOG_Denoising.h"

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {

bool CBoxAlgorithmEOG_Denoising::initialize()
{
    // Signal stream decoder
    m_algo0SignalDecoder.initialize(*this, 0);
    m_algo1SignalDecoder.initialize(*this, 1);

    m_algo2SignalEncoder.getInputSamplingRate().setReferenceTarget(m_algo0SignalDecoder.getOutputSamplingRate());

    m_filename = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);

    m_fBMatrixFile.open(m_filename.toASCIIString(), std::ios::in);
    if (m_fBMatrixFile.fail()) {
        this->getLogManager() << Kernel::LogLevel_Error << "Failed to open [" << m_filename << "] for reading\n";
        return false;
    }

    m_fBMatrixFile >> m_nChannels0;
    m_fBMatrixFile >> m_nChannels1;
    m_fBMatrixFile >> m_nSamples0;

    if (m_fBMatrixFile.fail()) {
        this->getLogManager() << Kernel::LogLevel_Error << "Not able to successfully read dims from [" << m_filename << "]\n";
        m_fBMatrixFile.close();
        return false;
    }


    m_nSamples1 = m_nSamples0;

    m_noiseCoeff.resize(m_nChannels0, m_nChannels1);    //Noise Coefficients Matrix (Dim: Channels EEG x Channels EOG)
    m_noiseCoeff.setZero(m_nChannels0, m_nChannels1);

    for (Eigen::Index i = 0; i < m_nChannels0; ++i) {   //Number of channels
        for (Eigen::Index j = 0; j < m_nChannels1; ++j) { m_fBMatrixFile >> m_noiseCoeff(i, j); }   //Number of Samples per Chunk
    }

    if (m_fBMatrixFile.fail()) {
        this->getLogManager() << Kernel::LogLevel_Error << "Not able to successfully read coefficients from [" << m_filename << "]\n";
        m_fBMatrixFile.close();
        return false;
    }

    m_fBMatrixFile.close();

    // Signal stream encoder
    m_algo2SignalEncoder.initialize(*this, 0);

    return true;
}


bool CBoxAlgorithmEOG_Denoising::uninitialize()
{
    m_algo0SignalDecoder.uninitialize();
    m_algo1SignalDecoder.uninitialize();
    m_algo2SignalEncoder.uninitialize();

    return true;
}


bool CBoxAlgorithmEOG_Denoising::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}


bool CBoxAlgorithmEOG_Denoising::process()
{
    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();

    Eigen::MatrixXd data0(m_nChannels0, m_nSamples0);   //EEG data
    Eigen::MatrixXd data1(m_nChannels1, m_nSamples1);   //EOG data
    Eigen::MatrixXd eegC(m_nChannels0, m_nSamples0);    //Corrected Matrix

    if (boxContext.getInputChunkCount(0) != 0) {
        for (size_t i = 0; i < boxContext.getInputChunkCount(1); ++i) { //Don't know why getinputchunkcount(1)
            // Signal EEG
            // decode the chunk ii on input 0
            m_algo0SignalDecoder.decode(i);

            CMatrix* matrix0      = m_algo0SignalDecoder.getOutputMatrix();
            const double* buffer0 = matrix0->getBuffer();

            for (Eigen::Index c = 0; c < m_nChannels0; ++c) {   //Number of channels
                for (Eigen::Index s = 0; s < m_nSamples0; ++s) { data0(c, s) = buffer0[s + c * m_nSamples0]; }  //Number of Samples per Chunk
            }

            //Signal EOG
            m_algo1SignalDecoder.decode(i);

            CMatrix* matrix1      = m_algo1SignalDecoder.getOutputMatrix();
            const double* buffer1 = matrix1->getBuffer();

            for (Eigen::Index c = 0; c < m_nChannels1; ++c) {   //Number of channels
                for (Eigen::Index s = 0; s < m_nSamples1; ++s) { data1(c, s) = buffer1[s + c * m_nSamples1]; }  //Number of Samples per Chunk
            }


            //Set the output (corrected EEG) to the same structure as the EEG input
            m_algo2SignalEncoder.getInputMatrix()->resize(m_nChannels0, m_nSamples0);


            for (Eigen::Index c = 0; c < m_nChannels0; c++) {
                m_algo2SignalEncoder.getInputMatrix()->setDimensionLabel(0, c, m_algo0SignalDecoder.getOutputMatrix()->getDimensionLabel(0, c));
            }


            //Remove the noise
            eegC = data0 - (m_noiseCoeff * data1);


            for (Eigen::Index c = 0; c < m_nChannels0; ++c) {       //Number of EEG channels
                for (Eigen::Index s = 0; s < m_nSamples0; ++s) {    //Number of Samples per Chunk
                    m_algo2SignalEncoder.getInputMatrix()->getBuffer()[s + c * m_nSamples0] = eegC(c, s);
                }
            }

            m_algo2SignalEncoder.getInputSamplingRate().setReferenceTarget(m_algo1SignalDecoder.getOutputSamplingRate());

            if (m_algo1SignalDecoder.isHeaderReceived()) {
                m_algo2SignalEncoder.encodeHeader();
                // send the output chunk containing the header. The dates are the same as the input chunk:
                boxContext.markOutputAsReadyToSend(0, boxContext.getInputChunkStartTime(0, i), boxContext.getInputChunkEndTime(0, i));
            }

            if (m_algo1SignalDecoder.isBufferReceived()) {
                // Encode the output buffer :
                m_algo2SignalEncoder.encodeBuffer();
                // and send it to the next boxes :
                boxContext.markOutputAsReadyToSend(0, boxContext.getInputChunkStartTime(0, i), boxContext.getInputChunkEndTime(0, i));
            }
            if (m_algo1SignalDecoder.isEndReceived()) {
                // End of stream received. This happens only once when pressing "stop". Just pass it to the next boxes so they receive the message :
                m_algo2SignalEncoder.encodeEnd();
                boxContext.markOutputAsReadyToSend(0, boxContext.getInputChunkStartTime(0, i), boxContext.getInputChunkEndTime(0, i));
            }
        }
    }
    return true;
}
}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
