///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmSpatialFilter.cpp
/// \brief Classes implementation for the Box Spatial Filter.
/// \author Yann Renard (Inria) / Jussi T. Lindgren (Inria).
/// \version 1.1.
/// \copyright Copyright (C) 2022 Inria
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
/// 
///-------------------------------------------------------------------------------------------------

#include "CBoxAlgorithmSpatialFilter.hpp"

#include <sstream>
#include <string>

#include <Eigen/Dense>
typedef Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor> MatrixXdRowMajor;

namespace OpenViBE {
namespace Plugins {
namespace SignalProcessing {

size_t CBoxAlgorithmSpatialFilter::loadCoefs(const CString& coefs, const char c1, const char c2, const size_t nRows, const size_t nCols)
{
    // Count the number of entries
    // @Note To avoid doing a ton of subsequent memory allocations (very slow on Windows debug builds), we first count the number of entries in the vector. If the file format had specified the vector dimension, we wouldn't have to do this step.
    size_t count    = 0;
    const char* ptr = coefs.toASCIIString();
    while (*ptr != 0) {
        // Skip separator characters
        while (*ptr == c1 || *ptr == c2) { ptr++; }
        if (*ptr == 0) { break; }
        // Ok, we have reached something that is not NULL or separator, assume its a number
        count++;
        // Skip the normal characters
        while (*ptr != c1 && *ptr != c2 && *ptr != 0) { ptr++; }
    }

    OV_ERROR_UNLESS_KRZ(count == nRows*nCols, "Invalid computed coefficients count [" << count << "] (expected " << nRows * nCols << " coefficients)",
                        Kernel::ErrorType::BadProcessing);

    // Resize in one step for efficiency.
    m_filterBank.resize(nRows, nCols);

    double* filter = m_filterBank.getBuffer();

    // Ok, convert to floats
    ptr        = coefs.toASCIIString();
    size_t idx = 0;
    while (*ptr != 0) {
        const size_t size = 1024;
        char buffer[size];
        // Skip separator characters
        while (*ptr == c1 || *ptr == c2) { ptr++; }
        if (*ptr == 0) { break; }
        // Copy the normal characters, don't exceed buffer size
        size_t i = 0;
        while (*ptr != c1 && *ptr != c2 && *ptr != 0) {
            if (i < size - 1) { buffer[i++] = *ptr; }
            else { break; }
            ptr++;
        }
        buffer[i] = 0;

        OV_ERROR_UNLESS_KRZ(idx < count, "Invalid parsed coefficient number [" << idx << "] (expected maximium " << count << " coefficients)",
                            Kernel::ErrorType::BadProcessing);

        // Finally, convert
        try { filter[idx] = std::stod(buffer); }
        catch (const std::exception&) {
            const size_t row = idx / nRows + 1;
            const size_t col = idx % nRows + 1;

            OV_ERROR_KRZ("Failed to parse coefficient number [" << idx << "] at matrix positions [" << row << "," << col << "]",
                         Kernel::ErrorType::BadProcessing);
        }

        idx++;
    }

    return idx;
}

bool CBoxAlgorithmSpatialFilter::initialize()
{
    const Kernel::IBox& boxContext = this->getStaticBoxContext();

    m_decoder = nullptr;
    m_encoder = nullptr;

    CIdentifier id;
    boxContext.getInputType(0, id);

    if (id == OV_TypeId_StreamedMatrix) {
        m_decoder = new Toolkit::TStreamedMatrixDecoder<CBoxAlgorithmSpatialFilter>(*this, 0);
        m_encoder = new Toolkit::TStreamedMatrixEncoder<CBoxAlgorithmSpatialFilter>(*this, 0);
    }
    else if (id == OV_TypeId_Signal) {
        m_decoder = new Toolkit::TSignalDecoder<CBoxAlgorithmSpatialFilter>(*this, 0);
        m_encoder = new Toolkit::TSignalEncoder<CBoxAlgorithmSpatialFilter>(*this, 0);

        static_cast<Toolkit::TSignalEncoder<CBoxAlgorithmSpatialFilter>*>(m_encoder)->getInputSamplingRate().setReferenceTarget(
            static_cast<Toolkit::TSignalDecoder<CBoxAlgorithmSpatialFilter>*>(m_decoder)->getOutputSamplingRate());
    }
    else if (id == OV_TypeId_Spectrum) {
        m_decoder = new Toolkit::TSpectrumDecoder<CBoxAlgorithmSpatialFilter>(*this, 0);
        m_encoder = new Toolkit::TSpectrumEncoder<CBoxAlgorithmSpatialFilter>(*this, 0);

        static_cast<Toolkit::TSpectrumEncoder<CBoxAlgorithmSpatialFilter>*>(m_encoder)->getInputFrequencyAbscissa().setReferenceTarget(
            static_cast<Toolkit::TSpectrumDecoder<CBoxAlgorithmSpatialFilter>*>(m_decoder)->getOutputFrequencyAbscissa());
        static_cast<Toolkit::TSpectrumEncoder<CBoxAlgorithmSpatialFilter>*>(m_encoder)->getInputSamplingRate().setReferenceTarget(
            static_cast<Toolkit::TSpectrumDecoder<CBoxAlgorithmSpatialFilter>*>(m_decoder)->getOutputSamplingRate());
    }
    else { OV_ERROR_KRF("Invalid input stream type [" << id.str() << "]", Kernel::ErrorType::BadInput); }

    // If we have a filter file, use dimensions and coefficients from that. Otherwise, use box config params.
    const CString filterFile = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 3);
    if (filterFile != CString("")) {
        OV_ERROR_UNLESS_KRF(Toolkit::Matrix::loadFromTextFile(m_filterBank, filterFile),
                            "Failed to load filter parameters from file at location [" << filterFile << "]", Kernel::ErrorType::BadFileRead);

        OV_ERROR_UNLESS_KRF(m_filterBank.getDimensionCount() == 2,
                            "Invalid filter matrix in file " << filterFile << ": found [" << m_filterBank.getDimensionCount()
                            << "] dimensions (expected 2 dimension)", Kernel::ErrorType::BadConfig);

#if defined(DEBUG)
        Toolkit::Matrix::saveToTextFile(m_filterBank, this->getConfigurationManager().expand("${Path_UserData}/spatialfilter_debug.txt"));
#endif
    }
    else {
        const CString coefs = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
        // The double cast is needed until FSettingValueAutoCast supports size_t.
        const size_t nOChannels = size_t(uint64_t(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1)));
        const size_t nIChannels = size_t(uint64_t(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2)));
        const size_t nCoefs     = loadCoefs(coefs, ' ', OV_Value_EnumeratedStringSeparator, nOChannels, nIChannels);

        OV_ERROR_UNLESS_KRF(nCoefs == nOChannels * nIChannels,
                            "Invalid number of coefficients [" << nCoefs << "] (expected "<< nOChannels * nIChannels << " coefficients)",
                            Kernel::ErrorType::BadConfig);

#if defined(DEBUG)
        Toolkit::Matrix::saveToTextFile(m_filterBank, this->getConfigurationManager().expand("${Path_UserData}/spatialfilter_debug.txt"));
#endif
    }

    return true;
}

bool CBoxAlgorithmSpatialFilter::uninitialize()
{
    if (m_decoder) {
        m_decoder->uninitialize();
        delete m_decoder;
        m_decoder = nullptr;
    }

    if (m_encoder) {
        m_encoder->uninitialize();
        delete m_encoder;
        m_encoder = nullptr;
    }

    return true;
}

bool CBoxAlgorithmSpatialFilter::processInput(const size_t /*index*/)
{
    this->getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

bool CBoxAlgorithmSpatialFilter::process()
{
    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();

    for (size_t i = 0; i < boxContext.getInputChunkCount(0); ++i) {
        m_decoder->decode(i);
        if (m_decoder->isHeaderReceived()) {
            // we can treat them all as matrix decoders as they all inherit from it
            const CMatrix* iMatrix = (static_cast<Toolkit::TStreamedMatrixDecoder<CBoxAlgorithmSpatialFilter>*>(m_decoder))->getOutputMatrix();

            const size_t nChannelIn = iMatrix->getDimensionSize(0);
            const size_t nSampleIn  = iMatrix->getDimensionSize(1);

            OV_ERROR_UNLESS_KRF(nChannelIn != 0 && nSampleIn != 0,
                                "Invalid matrix size with zero dimension on input [" << nChannelIn << " x " << nSampleIn << "]", Kernel::ErrorType::BadConfig);

            const size_t nChannelFilterIn  = m_filterBank.getDimensionSize(1);
            const size_t nChannelFilterOut = m_filterBank.getDimensionSize(0);

            OV_ERROR_UNLESS_KRF(nChannelIn == nChannelFilterIn,
                                "Invalid input channel count  [" << nChannelIn << "] (expected " << nChannelFilterIn << " channel count)",
                                Kernel::ErrorType::BadConfig);

            CMatrix* oMatrix = static_cast<Toolkit::TStreamedMatrixEncoder<CBoxAlgorithmSpatialFilter>*>(m_encoder)->getInputMatrix();
            oMatrix->resize(nChannelFilterOut, nSampleIn);

            // Name channels
            for (size_t j = 0; j < oMatrix->getDimensionSize(0); ++j) { oMatrix->setDimensionLabel(0, j, ("sFiltered " + std::to_string(j)).c_str()); }

            m_encoder->encodeHeader();
        }
        if (m_decoder->isBufferReceived()) {
            const CMatrix* iMatrix = static_cast<Toolkit::TStreamedMatrixDecoder<CBoxAlgorithmSpatialFilter>*>(m_decoder)->getOutputMatrix();
            CMatrix* oMatrix       = static_cast<Toolkit::TStreamedMatrixEncoder<CBoxAlgorithmSpatialFilter>*>(m_encoder)->getInputMatrix();

            const double* in         = iMatrix->getBuffer();
            double* out              = oMatrix->getBuffer();
            const size_t nChannelIn  = iMatrix->getDimensionSize(0);
            const size_t nChannelOut = oMatrix->getDimensionSize(0);
            const size_t nSample     = iMatrix->getDimensionSize(1);

            //@TODO check this part we only create matrix ?
            const Eigen::Map<MatrixXdRowMajor> inMapper(const_cast<double*>(in), nChannelIn, nSample);
            const Eigen::Map<MatrixXdRowMajor> filterMapper(m_filterBank.getBuffer(), m_filterBank.getDimensionSize(0), m_filterBank.getDimensionSize(1));
            Eigen::Map<MatrixXdRowMajor> outMapper(out, nChannelOut, nSample);
            outMapper = filterMapper * inMapper;
            m_encoder->encodeBuffer();
        }
        if (m_decoder->isEndReceived()) { m_encoder->encodeEnd(); }

        boxContext.markOutputAsReadyToSend(0, boxContext.getInputChunkStartTime(0, i), boxContext.getInputChunkEndTime(0, i));
    }

    return true;
}

}  // namespace SignalProcessing
}  // namespace Plugins
}  // namespace OpenViBE
