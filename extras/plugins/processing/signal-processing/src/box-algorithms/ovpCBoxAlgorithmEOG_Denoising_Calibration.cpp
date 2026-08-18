#include "ovpCBoxAlgorithmEOG_Denoising_Calibration.h"

#include <Eigen/Dense>
#include <cmath>
#include <cfloat>

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {

bool CBoxAlgorithmEOG_Denoising_Calibration::initialize()
{
    m_algo0SignalDecoder.initialize(*this, 0);
    m_algo1SignalDecoder.initialize(*this, 1);
    m_algo2StimulationDecoder.initialize(*this, 2);
    m_stimulationEncoder.initialize(*this, 0);

    m_calibrationFilename = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
    m_stimID              = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);

    m_startTime       = 0;
    m_endTime         = 0;
    m_nChunks         = 0;
    m_chunksVerify    = -1;
    m_endProcess      = false;
    m_time            = 0;
    m_startTimeChunks = 0;
    m_endTimeChunks   = 0;

    // Random id for tmp token, clash possible if multiple boxes run in parallel (but unlikely)
    const CString randomToken = CIdentifier::random().toString();
    m_eegTempFilename         = this->getConfigurationManager().expand("${Path_Tmp}/denoising_") + randomToken + "_EEG_tmp.dat";
    m_eogTempFilename         = this->getConfigurationManager().expand("${Path_Tmp}/denoising_") + randomToken + "_EOG_tmp.dat";

    m_eegFile.open(m_eegTempFilename, std::ios::out | std::ios::in | std::ios::trunc);
    if (m_eegFile.fail()) {
        this->getLogManager() << Kernel::LogLevel_Error << "Opening [" << m_eegTempFilename << "] for r/w failed\n";
        return false;
    }

    m_eogFile.open(m_eogTempFilename, std::ios::out | std::ios::in | std::ios::trunc);
    if (m_eogFile.fail()) {
        this->getLogManager() << Kernel::LogLevel_Error << "Opening [" << m_eogTempFilename << "] for r/w failed\n";
        return false;
    }

    m_matrixFile.open(m_calibrationFilename.toASCIIString(), std::ios::out | std::ios::trunc);
    if (m_matrixFile.fail()) {
        this->getLogManager() << Kernel::LogLevel_Error << "Opening [" << m_calibrationFilename << "] for writing failed\n";
        return false;
    }

    return true;
}


bool CBoxAlgorithmEOG_Denoising_Calibration::uninitialize()
{
    m_algo0SignalDecoder.uninitialize();
    m_algo1SignalDecoder.uninitialize();
    m_algo2StimulationDecoder.uninitialize();
    m_stimulationEncoder.uninitialize();

    // Clean up temporary files
    if (m_eegFile.is_open()) { m_eegFile.close(); }
    if (m_eegTempFilename != CString("")) { std::remove(m_eegTempFilename); }

    if (m_eogFile.is_open()) { m_eogFile.close(); }
    if (m_eogTempFilename != CString("")) { std::remove(m_eogTempFilename); }

    if (m_matrixFile.is_open()) { m_matrixFile.close(); }

    return true;
}


bool CBoxAlgorithmEOG_Denoising_Calibration::processClock(Kernel::CMessageClock& /*msg*/)
{
    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();

    if (m_nChunks != m_chunksVerify && m_endProcess == false) {
        m_chunksVerify = m_nChunks;
        if (std::fabs(m_time - m_startTime) <= DBL_EPSILON) { m_startTimeChunks = m_nChunks; }
        if (std::fabs(m_time - m_endTime) <= DBL_EPSILON) { m_endTimeChunks = m_nChunks; }
    }
    else if (m_nChunks == m_chunksVerify && m_endProcess == false) {
        if ((m_startTime >= m_endTime) || (m_endTime >= m_time)) {
            this->getLogManager() << Kernel::LogLevel_Warning << "Verify time interval of sampling" << "\n";
            this->getLogManager() << Kernel::LogLevel_Warning << "Total time of your sample: " << m_time << "\n";
            this->getLogManager() << Kernel::LogLevel_Warning << "b Matrix was NOT successfully calculated" << "\n";

            m_stimulationEncoder.getInputStimulationSet()->clear();
            m_stimulationEncoder.getInputStimulationSet()->push_back(OVTK_StimulationId_TrainCompleted, 0, 0);
            m_stimulationEncoder.encodeBuffer();

            boxContext.markOutputAsReadyToSend(0, boxContext.getInputChunkStartTime(0, 0), boxContext.getInputChunkEndTime(0, 0));

            //this->getLogManager() << Kernel::LogLevel_Warning << "You can stop this scenario " <<"\n";
            m_chunksVerify = -1;
            m_endProcess   = true;
        }
        else {
            this->getLogManager() << Kernel::LogLevel_Info << "End of data gathering...calculating b matrix" << "\n";

            m_eegFile.close();
            m_eogFile.close();

            m_eegFile.open(m_eegTempFilename, std::ios::in | std::ios::app);
            if (m_eegFile.fail()) {
                this->getLogManager() << Kernel::LogLevel_Error << "Opening [" << m_eegTempFilename << "] for reading failed\n";
                return false;
            }

            m_eogFile.open(m_eogTempFilename, std::ios::in | std::ios::app);
            if (m_eogFile.fail()) {
                this->getLogManager() << Kernel::LogLevel_Error << "Opening [" << m_eogTempFilename << "] for reading failed\n";
                return false;
            }

            //Process to extract the Matrix B
            double aux;

            Eigen::MatrixXd data0(m_nChannels0, m_nSamples0 * m_nChunks);   //EEG data
            Eigen::MatrixXd data1(m_nChannels1, m_nSamples1 * m_nChunks);   //EOG data

            for (Eigen::Index k = 0; k < m_nChunks; ++k) {
                for (Eigen::Index i = 0; i < m_nChannels0; ++i) {       //Number of channels
                    for (Eigen::Index j = 0; j < m_nSamples0; ++j) {    //Number of Samples per Chunk
                        m_eegFile >> aux;
                        data0(i, j + k * m_nSamples0) = aux;
                    }
                }
            }

            for (Eigen::Index k = 0; k < m_nChunks; ++k) {
                for (Eigen::Index i = 0; i < m_nChannels1; ++i) {       //Number of channels
                    for (Eigen::Index j = 0; j < m_nSamples1; ++j) {    //Number of Samples per Chunk
                        m_eogFile >> aux;
                        data1(i, j + k * m_nSamples1) = aux;
                    }
                }
            }

            // We will eliminate the firsts and lasts chunks of each channel

            Eigen::MatrixXd data0N(m_nChannels0, m_nSamples0 * m_nChunks);  //EEG data
            Eigen::MatrixXd data1N(m_nChannels1, m_nSamples1 * m_nChunks);  //EOG data

            Eigen::Index validChunks = m_endTimeChunks - m_startTimeChunks;

            Eigen::Index iblockeeg = 0;
            Eigen::Index jblockeeg = m_startTimeChunks * m_nSamples0 - 1;
            Eigen::Index pblockeeg = m_nChannels0;
            Eigen::Index qblockeeg = m_nSamples0 * validChunks;
            Eigen::Index iblockeog = 0;
            Eigen::Index jblockeog = m_startTimeChunks * m_nSamples1 - 1;
            Eigen::Index pblockeog = m_nChannels1;
            Eigen::Index qblockeog = m_nSamples1 * validChunks;


            data0N = data0.block(iblockeeg, jblockeeg, pblockeeg, qblockeeg);
            data1N = data1.block(iblockeog, jblockeog, pblockeog, qblockeog);


            size_t nVal = 0;
            double min  = 1e-6;
            double max  = 1e6;

            Eigen::VectorXd meanRowEEG(m_nChannels0);
            Eigen::VectorXd meanRowEog(m_nChannels1);

            meanRowEEG.setZero(m_nChannels0, 1);
            meanRowEog.setZero(m_nChannels1, 1);


            for (Eigen::Index i = 0; i < m_nChannels0; ++i) {                   //Number of channels
                nVal = 0;
                for (Eigen::Index j = 0; j < m_nSamples0 * validChunks; ++j) {  //Number of Samples per Chunk
                    if ((data0N(i, j) > -max && data0N(i, j) < -min) || (data0N(i, j) > min && data0N(i, j) < max)) {
                        //Valid Interval
                        meanRowEEG(i) = meanRowEEG(i) + data0N(i, j);
                        nVal++;
                    }
                }
                if (nVal != 0) { meanRowEEG(i) = meanRowEEG(i) / double(nVal); }
                else { meanRowEEG(i) = 0; }
            }


            for (Eigen::Index i = 0; i < m_nChannels1; ++i) {                   //Number of channels
                nVal = 0;
                for (Eigen::Index j = 0; j < m_nSamples1 * validChunks; ++j) {  //Number of Samples per Chunk
                    if ((data1N(i, j) > -max && data1N(i, j) < -min) || (data1N(i, j) > min && data1N(i, j) < max)) {
                        //Valid Interval
                        meanRowEog(i) = meanRowEog(i) + data1N(i, j);
                        nVal++;
                    }
                }
                if (nVal != 0) { meanRowEog(i) = meanRowEog(i) / double(nVal); }
                else { meanRowEog(i) = 0; }
            }


            // The values which are not valid (very large or very small) will be set to the mean value
            // So these values will not influence to the covariance calcul because the covariance is centered (value - mean)


            for (Eigen::Index i = 0; i < m_nChannels0; ++i) {                   //Number of channels
                for (Eigen::Index j = 0; j < m_nSamples0 * validChunks; ++j) {  //Number of total samples
                    if ((data0N(i, j) > -max && data0N(i, j) < -min) || (data0N(i, j) > min && data0N(i, j) < max)) {   //Valid Interval
                        data0N(i, j) = data0N(i, j) - meanRowEEG(i);
                    }
                    else { data0N(i, j) = 0; }  //Invalid
                }
            }


            for (Eigen::Index i = 0; i < m_nChannels1; ++i) {                   //Number of channels
                for (Eigen::Index j = 0; j < m_nSamples1 * validChunks; ++j) {  //Number of total samples
                    if ((data1N(i, j) > -max && data1N(i, j) < -min) || (data1N(i, j) > min && data1N(i, j) < max)) {   //Valid Interval
                        data1N(i, j) = data1N(i, j) - meanRowEog(i);
                    }
                    else { data1N(i, j) = 0; }  //Invalid
                }
            }

            //Now we need to calculate the matrix b (which tells us the correct weights to be stored in b matrix)
            Eigen::MatrixXd noiseCoeff(m_nChannels0, m_nChannels1);   //Noise Coefficients Matrix (Dim: Channels EEG x Channels EOG)
            Eigen::MatrixXd covEog(m_nChannels1, m_nChannels1);
            Eigen::MatrixXd covEogInv(m_nChannels1, m_nChannels1);
            Eigen::MatrixXd covEegAndEog(m_nChannels0, m_nChannels1);

            covEog       = (data1N * data1N.transpose());
            covEogInv    = covEog.inverse();
            covEegAndEog = data0N * (data1N.transpose());
            noiseCoeff   = covEegAndEog * covEogInv;

            // Save Matrix b to the file specified in the parameters
            m_matrixFile << m_nChannels0 << " " << m_nChannels1 << " " << m_nSamples0 << "\n";

            for (Eigen::Index i = 0; i < m_nChannels0; ++i) {   //Number of channels EEG
                for (Eigen::Index j = 0; j < m_nChannels1; ++j) { m_matrixFile << noiseCoeff(i, j) << "\n"; }   //Number of channels EOG
            }

            m_eegFile.close();
            m_eogFile.close();
            m_matrixFile.close();

            m_chunksVerify = -1;
            m_endProcess   = true;

            this->getLogManager() << Kernel::LogLevel_Info << "b Matrix was successfully calculated" << "\n";
            this->getLogManager() << Kernel::LogLevel_Info << "Wrote the matrix to [" << m_calibrationFilename << "]\n";

            m_stimulationEncoder.getInputStimulationSet()->clear();
            m_stimulationEncoder.getInputStimulationSet()->push_back(OVTK_StimulationId_TrainCompleted, 0, 0);
            m_stimulationEncoder.encodeBuffer();

            boxContext.markOutputAsReadyToSend(0, boxContext.getInputChunkStartTime(0, 0), boxContext.getInputChunkEndTime(0, 0));
        }
    }

    m_time++;
    return true;
}

bool CBoxAlgorithmEOG_Denoising_Calibration::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}


bool CBoxAlgorithmEOG_Denoising_Calibration::process()
{
    const Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();

    if (m_endProcess) {
        // We have done our stuff and have sent out a stimuli that we're done. However, if we're called again, we just do nothing,
        // but do not return false (==error) as this state is normal after training.
        return true;
    }

    // Signal EEG
    for (size_t i = 0; i < boxContext.getInputChunkCount(0); ++i) {
        m_algo0SignalDecoder.decode(i);

        m_nChannels0 = Eigen::Index(m_algo0SignalDecoder.getOutputMatrix()->getDimensionSize(0));
        m_nSamples0  = Eigen::Index(m_algo0SignalDecoder.getOutputMatrix()->getDimensionSize(1));

        CMatrix* matrix0      = m_algo0SignalDecoder.getOutputMatrix();
        const double* buffer0 = matrix0->getBuffer();

        for (size_t j = 0; j < matrix0->getBufferElementCount(); ++j) { m_eegFile << buffer0[j] << "\n"; }
    }
    //Signal EOG
    for (size_t i = 0; i < boxContext.getInputChunkCount(1); ++i) {
        m_algo1SignalDecoder.decode(i);

        m_nChannels1 = Eigen::Index(m_algo1SignalDecoder.getOutputMatrix()->getDimensionSize(0));
        m_nSamples1  = Eigen::Index(m_algo1SignalDecoder.getOutputMatrix()->getDimensionSize(1));

        CMatrix* matrix1 = m_algo1SignalDecoder.getOutputMatrix();
        double* buffer1  = matrix1->getBuffer();

        for (size_t j = 0; j < matrix1->getBufferElementCount(); ++j) { m_eogFile << buffer1[j] << "\n"; }

        m_nChunks++;
    }


    for (size_t chunk = 0; chunk < boxContext.getInputChunkCount(2); ++chunk) {
        m_algo2StimulationDecoder.decode(chunk);
        for (size_t j = 0; j < m_algo2StimulationDecoder.getOutputStimulationSet()->size(); ++j) {
            if (m_algo2StimulationDecoder.getOutputStimulationSet()->getId(j) == 33025) {
                m_startTime = m_time;
                this->getLogManager() << Kernel::LogLevel_Info << "Start time: " << m_startTime << "\n";
            }

            if (m_algo2StimulationDecoder.getOutputStimulationSet()->getId(j) == 33031) {
                m_endTime = m_time;
                this->getLogManager() << Kernel::LogLevel_Info << "End time: " << m_endTime << "\n";
            }

            // m_trainDate = m_algo2StimulationDecoder.getOutputStimulationSet()->getDate(m_algo2StimulationDecoder.getOutputStimulationSet()->size());
            m_trainDate = m_algo2StimulationDecoder.getOutputStimulationSet()->getDate(j);
        }
    }
    return true;
}
}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
