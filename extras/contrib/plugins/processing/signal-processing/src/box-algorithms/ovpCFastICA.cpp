#if defined TARGET_HAS_ThirdPartyITPP

#include "ovpCFastICA.h"

#include <iostream>

#include <itpp/itstat.h>
#include <itpp/itsignal.h>

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {

void CFastICA::computeICA()
{
    const size_t nChannel = m_decoder.getOutputMatrix()->getDimensionSize(0);
    const size_t nSample  = m_decoder.getOutputMatrix()->getDimensionSize(1);
    const double* iBuffer = m_decoder.getOutputMatrix()->getBuffer();

    const size_t nICs = m_encoder.getInputMatrix()->getDimensionSize(0);

    itpp::mat sources(nChannel, nSample);         // current block (for decomposing)
    itpp::mat bufferSources(nChannel, m_buffSize);  // accumulated blocks (for training)
    itpp::mat ICs(nICs, nSample);
    //mat Mix_mat(nChannel, nChannel);
    itpp::mat sepMat(nICs, nChannel);
    //mat Dewhite(nChannel, nChannel);

    // Append the data to a FIFO buffer
    for (size_t i = 0; i < nChannel; ++i)
    {
        for (size_t j = 0; j < m_buffSize; ++j)
        {
            if (j < m_buffSize - nSample)
            {
                m_fifoBuffer[i * m_buffSize + j + nSample] = m_fifoBuffer[i * m_buffSize + j]; // memory shift
                if (j < nSample)
                {
                    m_fifoBuffer[i * m_buffSize + j] = double(iBuffer[i * nSample + nSample - 1 - j]);
                    sources(int(i), int(j))          = double(iBuffer[i * nSample + j]);
                }
            }

            bufferSources(int(i), int(m_buffSize - 1 - j)) = m_fifoBuffer[i * m_buffSize + j];
        }
    }

    m_nSample += nSample;
    if ((m_nSample >= m_buffSize) && (m_trained == false))
    {
        this->getLogManager() << Kernel::LogLevel_Trace << "Instanciating the Fast_ICA object with " << m_nSample << " samples.\n";
        itpp::Fast_ICA fastica(bufferSources);
        this->getLogManager() << Kernel::LogLevel_Trace << "Setting the number of ICs to extract to " << nICs << " and configuring FastICA...\n";

        if (m_mode == EFastICAMode::PCA || m_mode == EFastICAMode::Whiten) { fastica.set_pca_only(true); }
        else
        {
            fastica.set_approach(int(m_type));
            fastica.set_non_linearity(int(m_nonLin));
            fastica.set_max_num_iterations(m_nRepMax);
            fastica.set_fine_tune(m_setFineTune);
            fastica.set_max_fine_tune(m_nTuneMax);
            fastica.set_mu(m_setMu);
            fastica.set_epsilon(m_epsilon);
        }

        fastica.set_nrof_independent_components(nICs);

        //if(m_nSample>nSample) fastica.set_init_guess((Dewhite * Dewhite.T()) * Sep_mat.T());  
        this->getLogManager() << Kernel::LogLevel_Trace << "Explicit launch of the Fast_ICA algorithm. Can occasionally take time.\n";
        fastica.separate();
        this->getLogManager() << Kernel::LogLevel_Trace << "Retrieving separating matrix from fastica .\n";
        if (m_mode == EFastICAMode::PCA) { sepMat = fastica.get_principal_eigenvectors().transpose(); }
        else if (m_mode == EFastICAMode::Whiten) { sepMat = fastica.get_whitening_matrix(); }
        else { sepMat = fastica.get_separating_matrix(); }

        m_trained       = true;
        double* demixer = m_demixer.getBuffer();
        for (size_t i = 0; i < nICs; ++i) { for (size_t j = 0; j < nChannel; ++j) { demixer[i * nChannel + j] = sepMat(int(i), int(j)); } }
    }
    else
    {
        // Use the previously stored matrix
        const double* demixer = m_demixer.getBuffer();
        for (size_t i = 0; i < nICs; ++i) { for (size_t j = 0; j < nChannel; ++j) { sepMat(int(i), int(j)) = demixer[i * nChannel + j]; } }
    }

    // Effective demixing (ICA after m_duration sec)
    ICs = sepMat * sources;

    double* buffer = m_encoder.getInputMatrix()->getBuffer();
    //this->getLogManager() << Kernel::LogLevel_Trace << "Filling output buffer with ICs .\n";
    for (size_t i = 0; i < nICs; ++i) { for (size_t j = 0; j < nSample; ++j) { buffer[i * nSample + j] = ICs(int(i), int(j)); } }
}


bool CFastICA::initialize()
{
    m_decoder.initialize(*this, 0);
    m_encoder.initialize(*this, 0);

    m_nICs                  = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
    m_mode                  = EFastICAMode(uint64_t(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1)));
    m_duration              = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2);
    m_type                  = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 3);
    m_nRepMax               = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 4);
    m_setFineTune           = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 5);
    m_nTuneMax              = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 6);
    m_nonLin                = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 7);
    m_setMu                 = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 8);
    m_epsilon               = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 9);
    m_spatialFilterFilename = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 10);
    m_saveAsFile            = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 11);

    m_fifoBuffer = nullptr;
    m_fileSaved  = false;

    if (m_saveAsFile && m_spatialFilterFilename == CString(""))
    {
        this->getLogManager() << "If save is enabled, filename must be provided\n";
        return false;
    }

    return true;
}

bool CFastICA::uninitialize()
{
    m_encoder.uninitialize();
    m_decoder.uninitialize();
    if (m_fifoBuffer)
    {
        delete[] m_fifoBuffer;
        m_fifoBuffer = nullptr;
    }

    return true;
}

bool CFastICA::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

bool CFastICA::process()
{
    IDynamicBoxContext* boxContext = getBoxAlgorithmContext()->getDynamicBoxContext();

    // Process input data
    for (size_t i = 0; i < boxContext->getInputChunkCount(0); ++i)
    {
        m_decoder.decode(i);

        if (m_decoder.isHeaderReceived())
        {
            // Set the output (encoder) matrix prorperties from the input (decoder)
            if (m_decoder.getOutputMatrix()->getDimensionCount() != 2)
            {
                this->getLogManager() << Kernel::LogLevel_Error << "Needs a 2 dimensional (rows x cols) matrix as input\n";
                return false;
            }
            m_buffSize            = size_t(m_decoder.getOutputSamplingRate()) * m_duration;
            const size_t nChannel = m_decoder.getOutputMatrix()->getDimensionSize(0);
            const size_t nSample  = m_decoder.getOutputMatrix()->getDimensionSize(1);

            delete[] m_fifoBuffer;
            m_fifoBuffer = new double[nChannel * m_buffSize];
            this->getLogManager() << Kernel::LogLevel_Trace << "FIFO buffer initialized with " << nChannel * m_buffSize << ".\n";

            if (m_nICs > nChannel)
            {
                this->getLogManager() << Kernel::LogLevel_Warning << "Trying to estimate more components than channels, truncating\n";
                m_nICs = nChannel;
            }

            for (size_t j = 0; j < nChannel * m_buffSize; ++j) { m_fifoBuffer[j] = 0.0; }
            m_nSample = 0;
            m_trained = false;

            CMatrix* matrix = m_encoder.getInputMatrix();
            matrix->resize(m_nICs, nSample);

            m_encoder.getInputSamplingRate().setReferenceTarget(m_decoder.getOutputSamplingRate());

            std::string prefix;
            if (m_mode == EFastICAMode::PCA) { prefix = "PC"; }
            else if (m_mode == EFastICAMode::Whiten) { prefix = "Wh"; }
            else { prefix = "IC"; }

            for (size_t c = 0; c < m_nICs; ++c) { matrix->setDimensionLabel(0, c, (prefix + std::to_string(c + 1)).c_str()); }

            m_encoder.encodeHeader();

            m_demixer.resize(m_nICs, nChannel);
            // Set the demixer to (partial) identity matrix to start with
            m_demixer.resetBuffer();
            double* demixer = m_demixer.getBuffer();
            for (size_t c = 0; c < m_nICs; ++c) { demixer[c * nChannel + c] = 1.0; }

            getBoxAlgorithmContext()->getDynamicBoxContext()->markOutputAsReadyToSend(0, 0, 0);
        }

        if (m_decoder.isBufferReceived())
        {
            const uint64_t startTime = boxContext->getInputChunkStartTime(0, i);
            const uint64_t endTime   = boxContext->getInputChunkEndTime(0, i);

            computeICA();

            if ((m_saveAsFile) && (m_trained) && (m_fileSaved == false))
            {
                if (!Toolkit::Matrix::saveToTextFile(m_demixer, m_spatialFilterFilename))
                {
                    this->getLogManager() << Kernel::LogLevel_Warning << "Unable to save to [" << m_spatialFilterFilename << "\n";
                }
                m_fileSaved = true;
            }

            m_encoder.encodeBuffer();

            getBoxAlgorithmContext()->getDynamicBoxContext()->markOutputAsReadyToSend(0, startTime, endTime);
        }

        // if (m_decoder.isEndReceived()) { }   // NOP
    }

    return true;
}

}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
#endif // TARGET_HAS_ThirdPartyITPP
