///-------------------------------------------------------------------------------------------------
///
/// \file CBoxAlgorithmConnectivityMeasure.cpp
/// \brief Implementation of the Box Connectivity Measure.
/// \author Arthur DESBOIS (Inria).
/// \version 0.0.1.
/// \date Fri Oct 30 16:18:49 2020.
///
/// \copyright Copyright(C) 2020-2022 Inria
///
/// This program is free software: you can redistribute it and/or modify
/// it under the terms of the GNU Affero General Public License as published
/// by the Free Software Foundation, either version 3 of the License, or
/// (at your option) any later version.
///
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU Affero General Public License for more details.
///
/// You should have received a copy of the GNU Affero General Public License
/// along with this program.  If not, see <https://www.gnu.org/licenses/>.
///-------------------------------------------------------------------------------------------------

#include "CBoxAlgorithmConnectivityMeasure.hpp"
#include "eigen/convert.hpp"
#include <cmath>
#include <chrono>

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {

bool CBoxAlgorithmConnectivityMeasure::initialize()
{
    m_signalDecoder.initialize(*this, 0);
    m_matrixEncoder.initialize(*this, 0);

    m_iMatrix = m_signalDecoder.getOutputMatrix();
    m_oMatrix = m_matrixEncoder.getInputMatrix();

    // Settings
    m_selectedChannels     = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
    m_metric               = EConnectMetric(uint64_t(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1)));
    m_connectLengthSeconds = double(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2));
    m_connectOverlap       = int(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 3));
    m_dcRemoval            = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 4);
    m_fftSize              = size_t(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 5));
    m_psdMode              = EPsdMode(uint64_t(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 6)));
    
    OV_ERROR_UNLESS_KRF(m_psdMode == EPsdMode::Welch || m_psdMode == EPsdMode::Burg, "Invalid PSD mode", Kernel::ErrorType::BadSetting);

    switch(m_psdMode) {
        case EPsdMode::Welch:
            m_windowMethod         = EConnectWindowMethod(uint64_t(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 7)));
            m_windowLengthSeconds  = double(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 8));
            m_windowOverlap        = int(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 9));
            break;

        case EPsdMode::Burg:
            m_autoRegOrder  = int(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 7));
            break;
    }

    return true;
}

/*******************************************************************************/
bool CBoxAlgorithmConnectivityMeasure::uninitialize()
{
    m_signalDecoder.uninitialize();
    m_matrixEncoder.uninitialize();
    return true;
}

bool CBoxAlgorithmConnectivityMeasure::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

/*******************************************************************************/
bool CBoxAlgorithmConnectivityMeasure::process()
{
    // the dynamic box context describes the current state of the box inputs and outputs (i.e. the chunks)
    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();

    //iterate over all chunk on input 0
    for (size_t i = 0; i < boxContext.getInputChunkCount(0); ++i) {
        m_signalDecoder.decode(i);

        const uint64_t tStart = boxContext.getInputChunkStartTime(0, i);    // Time Code Chunk Start
        const uint64_t tEnd   = boxContext.getInputChunkEndTime(0, i);        // Time Code Chunk End
        
        if (m_signalDecoder.isHeaderReceived()) {
            // Header received. This happens only once when pressing "play".

            const CMatrix* matrix = m_signalDecoder.getOutputMatrix();          // the StreamedMatrix of samples.
            uint64_t sampling     = m_signalDecoder.getOutputSamplingRate();    // the sampling rate of the signal

            m_windowLength          = int(std::floor(double(sampling) * m_windowLengthSeconds));
            m_connectLength         = int(std::floor(double(sampling) * m_connectLengthSeconds));
            m_connectOverlapSamples = int(std::floor(double(m_connectLength * m_connectOverlap / 100.0 )));
            m_windowOverlapSamples  = int(std::floor(double(m_windowLength * m_windowOverlap / 100.0)));
            m_connectShiftSamples = m_connectLength - m_connectOverlapSamples;

            if (m_selectedChannels == CString("")) {
                // Keep all channels
                for (size_t j = 0; j < m_iMatrix->getDimensionSize(0); ++j) {
                    m_vLookup.push_back(j);
                }
            } else {
                std::vector<CString> token;
                const size_t nToken = split(m_selectedChannels, Toolkit::String::TSplitCallback<std::vector<CString>>(token), OV_Value_EnumeratedStringSeparator);

                for (size_t j = 0; j < m_iMatrix->getDimensionSize(0); ++j) {
                    for (size_t k = 0; k < nToken; ++k) {
                        if (Toolkit::String::isAlmostEqual(m_iMatrix->getDimensionLabel(0, j), token[k], false)) {m_vLookup.push_back(j); }
                    }
                }

                if (nToken != m_vLookup.size()) {
                    this->getLogManager() << Kernel::LogLevel_Warning << "Some channels in the subselection were not found in the signal stream.\n";
                    for (size_t k = 0; k < nToken; ++k) {
                        bool found = false;
                        for (size_t j = 0; j < m_vLookup.size(); j++) {
                            if (Toolkit::String::isAlmostEqual(m_iMatrix->getDimensionLabel(0, m_vLookup[j]), token[k], false)) { 
                                found = true; break;
                            }
                        }

                        if (!found) {
                            this->getLogManager() << Kernel::LogLevel_Warning << token[k] << " was not found in the signal stream.\n";
                        }
                    }
                }

            }

            m_nbChanOutput = m_vLookup.size();
            OV_ERROR_UNLESS_KRF(m_nbChanOutput > 0, "Connectivity channel subselection error. Mismatch between the subselection and the channel names? ",
                Kernel::ErrorType::BadArgument);


                        
            this->getLogManager() << Kernel::LogLevel_Info << "NB CHANNELS FOR CONNECTIVITY OUTPUT: " << m_nbChanOutput << ")\n";

            m_timeIncrement = uint64_t(m_connectLengthSeconds * std::pow(2, 32));
            this->getLogManager() << Kernel::LogLevel_Debug << "TIME INCREMENT: " << m_timeIncrement << " (" << (double)(m_timeIncrement)*std::pow(2,-32) << ")\n";

            m_nbChannels           = matrix->getDimensionSize(0);
            const auto sampPerChan = matrix->getDimensionSize(1);

            this->getLogManager() << Kernel::LogLevel_Info << "HEADER : nChannels " << m_nbChannels << ", " << sampPerChan << " samples, sampling rate " << sampling << "\n";
            this->getLogManager() << Kernel::LogLevel_Debug << "Connectivity segments overlap percent/samples : " << m_connectOverlap << " " << m_connectOverlapSamples << "\n";
            this->getLogManager() << Kernel::LogLevel_Debug << "Connectivity segment shift (samples): " << m_connectShiftSamples << "\n";
            this->getLogManager() << Kernel::LogLevel_Debug << "Welch Windows overlap percent/samples : " << m_windowOverlap << " " << m_windowOverlapSamples << "\n";
                    
            // Vectors buffers init
            m_vectorXdBuffer.resize(m_nbChannels);
            m_signalChannelBuffers.resize(m_nbChannels);

            this->getLogManager() << Kernel::LogLevel_Debug << "m_signalChannelBuffers.size() " << m_signalChannelBuffers.size() << "\n";

            // Connectivity algo class inits
            if(m_psdMode == EPsdMode::Welch) {
                connectivityMeasure.initializeWelch(m_metric, sampling, m_nbChannels, m_fftSize, m_dcRemoval, m_windowMethod, m_windowLength, m_windowOverlap);
            } else if(m_psdMode == EPsdMode::Burg) {
                connectivityMeasure.initializeBurg(m_metric, sampling, m_nbChannels, m_fftSize, m_dcRemoval, m_autoRegOrder);
            } else {

            }

            m_oMatrix->resize({ m_fftSize, m_nbChannels, m_nbChanOutput }); // nbFreqs x nbChan x nbChanOutput
            this->getLogManager() << Kernel::LogLevel_Debug << "output Matrix dimensions: " << m_oMatrix->getDimensionSize(0) << " x " <<  m_oMatrix->getDimensionSize(1) << "\n";
            for (size_t aa = 0; aa < m_fftSize; aa++) {
                m_oMatrix->setDimensionLabel(0, aa, std::to_string(aa));
            }
            for (size_t aa = 0; aa < m_nbChannels; aa++) {
                m_oMatrix->setDimensionLabel(1, aa, matrix->getDimensionLabel(0, aa));
                //this->getLogManager() << Kernel::LogLevel_Debug << "Channel " << aa << " : " << m_oMatrix->getDimensionLabel(1, aa) << "/" << m_oMatrix->getDimensionLabel(2, aa) << "\n";
            }
            for (size_t aa = 0; aa < m_nbChanOutput; aa++) {
                m_oMatrix->setDimensionLabel(2, aa, matrix->getDimensionLabel(0, m_vLookup[aa]));
                this->getLogManager() << Kernel::LogLevel_Info << "Output matrix: Channel " << aa << " : " << m_oMatrix->getDimensionLabel(2, aa) << "\n";
            }

            m_matrixEncoder.encodeHeader(); // Pass the header to the next boxes
            boxContext.markOutputAsReadyToSend(0, tStart, tEnd);

        }

        if (m_signalDecoder.isBufferReceived()) {
            CMatrix* matrix = m_signalDecoder.getOutputMatrix(); // the StreamedMatrix of samples.
            uint64_t sampRate = m_signalDecoder.getOutputSamplingRate(); // the sampling rate of the signal
            const auto sampPerChan = matrix->getDimensionSize(1);

            // Accumulate buffers here and send a whole chunk to the connectivity algorithm         
            const double* buffer = matrix->getBuffer();
            size_t idx = 0;
            std::vector<double> temp;
            for (size_t row = 0; row < m_nbChannels; ++row) {
                for (size_t col = 0; col < sampPerChan; ++col) { // parse all columns in the buffer
                    temp.push_back(buffer[idx++]);
                }
                m_signalChannelBuffers[row].insert(m_signalChannelBuffers[row].end(), temp.begin(), temp.end());
                temp.clear();
            }
            
            int nbConnectWindows = std::floor(m_signalChannelBuffers[0].size() / m_connectShiftSamples);
            while(m_startTimes.size() < nbConnectWindows) {
                this->getLogManager() << Kernel::LogLevel_Debug << "=== START TIMES: NB CONNECT WINDOWS: " << nbConnectWindows << " (size of vector: " << m_startTimes.size() << ")\n";
                m_startTimes.push_back(tStart);
                this->getLogManager() << Kernel::LogLevel_Debug << "=== START TIMES: ADDING " << (double)(tStart)*std::pow(2, -32) << " (size of vector: " << m_startTimes.size() << ")\n";
            }

            this->getLogManager() << Kernel::LogLevel_Debug << "BUFFER : " << sampPerChan << " samples per " << m_nbChannels << " channels, sampling rate " <<
                sampRate << " // Signal buffers size : " << m_signalChannelBuffers[0].size() << "\n";
            
            // If enough data was accumulated, process it.
            while (m_signalChannelBuffers[0].size() >= size_t(m_connectLength)) {

                // Convert to Eigen container for easier use in algo
                for (size_t aa = 0; aa < m_nbChannels; ++aa) {
                    m_vectorXdBuffer[aa] = Eigen::VectorXd::Map(m_signalChannelBuffers[aa].data(), Eigen::Index(m_connectLength));
                }
                this->getLogManager() << Kernel::LogLevel_Debug << "Signal buffers : FULL (" << m_signalChannelBuffers[0].size() << ") / "
                    << " size of data sent to connectivity algo : " << (size_t)m_vectorXdBuffer[0].size() << "\n";

                // Connectivity chunk overlap for next processing loop: Keep the overlapping part in the vectors, discard the rest
                for (size_t aa = 0; aa < m_nbChannels; ++aa) {
                    m_signalChannelBuffers[aa].erase(m_signalChannelBuffers[aa].begin(), m_signalChannelBuffers[aa].begin() + m_connectShiftSamples);
                }

                // 3D Matrix init, vector of size (m_nbChanOutput) of Matrices (chan x fftsize)
                std::vector<Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic>> connectivityMatrix(m_nbChanOutput, Eigen::MatrixXd::Zero(m_nbChannels, m_fftSize));

                OV_ERROR_UNLESS_KRF(connectivityMeasure.process(m_vectorXdBuffer, connectivityMatrix, m_psdMode, m_vLookup), "Connectivity measurement error",
                                    Kernel::ErrorType::BadProcessing);
                
                this->getLogManager() << Kernel::LogLevel_Debug << "Exited connectivityMeasure.process() : connect size "
                        << (size_t)connectivityMatrix.size() << " x " << (size_t)connectivityMatrix[0].rows() << " x " << (size_t)connectivityMatrix[0].cols() << "\n";

                // Convert output matrix (nchanoutput x nchan x fftsize) to matrix (fftsize x nchan x nchanoutput)
                MatrixConvert(connectivityMatrix, *m_oMatrix, 5);

                m_matrixEncoder.encodeBuffer();
                boxContext.markOutputAsReadyToSend(0, m_startTimes[0], m_startTimes[0] + m_timeIncrement);
                
                this->getLogManager() << Kernel::LogLevel_Debug << "Output chunk times " << m_startTimes[0] << " / " << m_startTimes[0] + m_timeIncrement << "\n";
                this->getLogManager() << Kernel::LogLevel_Debug << "Output chunk times (human readable) " << ((double)(m_startTimes[0])* std::pow(2, -32)) << " / " << ((double)(m_startTimes[0] + m_timeIncrement)* std::pow(2, -32)) << "\n";

                m_startTimes.erase(m_startTimes.begin());
            }
            
        }

        if (m_signalDecoder.isEndReceived()) { 
            this->getLogManager() << Kernel::LogLevel_Debug << "END RECEIVED " << ((double)(tStart)* std::pow(2, -32)) << " / " << ((double)(tEnd)* std::pow(2, -32)) << "\n";
            m_matrixEncoder.encodeEnd(); 
            boxContext.markOutputAsReadyToSend(0, tStart, tEnd);
        }

    }

    return true;
}
}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
