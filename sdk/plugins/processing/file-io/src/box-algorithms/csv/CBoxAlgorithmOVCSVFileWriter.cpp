///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmOVCSVFileWriter.cpp
/// \brief Classes of the box CSV File Writer.
/// \author Victor Herlin (Mensia), Thomas Prampart (Inria).
/// \version 1.2.0
/// \date 07/05/2021
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
/// along with this program. If not, see <https://www.gnu.org/licenses/>.
/// 
///-------------------------------------------------------------------------------------------------

#include "CBoxAlgorithmOVCSVFileWriter.hpp"

#include <fs/Files.h>

#include <string>
#include <iostream>
#include <regex>

namespace OpenViBE {
namespace Plugins {
namespace FileIO {

///-------------------------------------------------------------------------------------------------
bool CBoxAlgorithmOVCSVFileWriter::initialize()
{
    m_isFileOpen = false;
    m_epoch      = 0;

    // If there is only 1 input, it is stimulation only
    if (this->getStaticBoxContext().getInputCount() > 1) {
        OV_ERROR_UNLESS_KRF(this->getStaticBoxContext().getInputType(0, m_typeID), "Error while getting input type", Kernel::ErrorType::Internal);
        if (m_typeID == OV_TypeId_Signal) {
            m_writerLib->setFormatType(CSV::EStreamType::Signal);
            m_streamDecoder = new Toolkit::TSignalDecoder<CBoxAlgorithmOVCSVFileWriter>(*this, 0);
        }
        else if (m_typeID == OV_TypeId_StreamedMatrix || m_typeID == OV_TypeId_CovarianceMatrix) {
            m_writerLib->setFormatType(CSV::EStreamType::StreamedMatrix);
            m_streamDecoder = new Toolkit::TStreamedMatrixDecoder<CBoxAlgorithmOVCSVFileWriter>(*this, 0);
        }
        else if (m_typeID == OV_TypeId_FeatureVector) {
            m_writerLib->setFormatType(CSV::EStreamType::FeatureVector);
            m_streamDecoder = new Toolkit::TFeatureVectorDecoder<CBoxAlgorithmOVCSVFileWriter>(*this, 0);
        }
        else if (m_typeID == OV_TypeId_Spectrum) {
            m_writerLib->setFormatType(CSV::EStreamType::Spectrum);
            m_streamDecoder = new Toolkit::TSpectrumDecoder<CBoxAlgorithmOVCSVFileWriter>(*this, 0);
        }
        else { OV_ERROR_KRF("Input is a type derived from matrix that the box doesn't recognize", Kernel::ErrorType::BadInput); }
    }
    else {
        m_stimIdx = 0;
        m_writerLib->setFormatType(CSV::EStreamType::Stimulations);
    }

    OV_ERROR_UNLESS_KRF(m_stimDecoder.initialize(*this, m_stimIdx),
                        "Error while stimulation decoder initialization", Kernel::ErrorType::Internal);


    const CString filename = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
    m_writerLib->setOutputFloatPrecision(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1));
    m_appendData     = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2);
    m_lastMatrixOnly = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 3);
    m_writerLib->setLastMatrixOnlyMode(m_lastMatrixOnly);

    if (!m_appendData) {
        OV_ERROR_UNLESS_KRF(m_writerLib->openFile(filename.toASCIIString(), CSV::EFileAccessMode::Write),
                            (CSV::ICSVHandler::getLogError(m_writerLib->getLastLogError()) + (m_writerLib->getLastErrorString().empty()
                                ? "" : "Details: " + m_writerLib->getLastErrorString())).c_str(), Kernel::ErrorType::Internal);
        m_writeHeader = true;
    }
    else {
        FILE* file = FS::Files::open(filename, "r");

        if (file) {
            fseek(file, 0, SEEK_END);
            m_writeHeader = ftell(file) == 0;
            fclose(file);
        }

        OV_ERROR_UNLESS_KRF(m_writerLib->openFile(filename.toASCIIString(), CSV::EFileAccessMode::Append),
                            (CSV::ICSVHandler::getLogError(m_writerLib->getLastLogError()) + (m_writerLib->getLastErrorString().empty()
                                ? "" : "Details: " + m_writerLib->getLastErrorString())).c_str(), Kernel::ErrorType::Internal);
    }

    return true;
}

///-------------------------------------------------------------------------------------------------
bool CBoxAlgorithmOVCSVFileWriter::uninitialize()
{
    m_streamDecoder.uninitialize();
    m_stimDecoder.uninitialize();

    OV_ERROR_UNLESS_KRF(m_writerLib->noEventsUntilDate(std::numeric_limits<double>::max()),
                        (CSV::ICSVHandler::getLogError(m_writerLib->getLastLogError()) + (m_writerLib->getLastErrorString().empty()
                            ? "" : "Details: " + m_writerLib->getLastErrorString())).c_str(), Kernel::ErrorType::Internal);

    OV_ERROR_UNLESS_KRF(m_writerLib->writeAllDataToFile(),
                        (CSV::ICSVHandler::getLogError(m_writerLib->getLastLogError()) + (m_writerLib->getLastErrorString().empty()
                            ? "" : "Details: " + m_writerLib->getLastErrorString())).c_str(), Kernel::ErrorType::Internal);

    OV_ERROR_UNLESS_KRF(m_writerLib->closeFile(),
                        (CSV::ICSVHandler::getLogError(m_writerLib->getLastLogError()) + (m_writerLib->getLastErrorString().empty()
                            ? "" : "Details: " + m_writerLib->getLastErrorString())).c_str(), Kernel::ErrorType::Internal);

    return true;
}

///-------------------------------------------------------------------------------------------------
bool CBoxAlgorithmOVCSVFileWriter::processInput(const size_t /*index*/)
{
    OV_ERROR_UNLESS_KRF(getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess(), "Error while marking algorithm as ready to process",
                        Kernel::ErrorType::Internal);
    return true;
}

///-------------------------------------------------------------------------------------------------
bool CBoxAlgorithmOVCSVFileWriter::process()
{
    if (this->getStaticBoxContext().getInputCount() > 1) {
        OV_ERROR_UNLESS_KRF(this->processStreamedMatrix(), "Error have been thrown during streamed matrix process", Kernel::ErrorType::Internal);
    }
    OV_ERROR_UNLESS_KRF(this->processStimulation(), "Error have been thrown during stimulation process", Kernel::ErrorType::Internal);

    // write into the library
    if (!m_lastMatrixOnly) {
        OV_ERROR_UNLESS_KRF(m_writerLib->writeDataToFile(),
                            (CSV::ICSVHandler::getLogError(m_writerLib->getLastLogError()) + (m_writerLib->getLastErrorString().empty()
                                ? "" : "Details: " + m_writerLib->getLastErrorString())).c_str(), Kernel::ErrorType::Internal);
    }

    return true;
}

///-------------------------------------------------------------------------------------------------
bool CBoxAlgorithmOVCSVFileWriter::processStreamedMatrix()
{
    Kernel::IBoxIO& boxCtx = this->getDynamicBoxContext();

    for (size_t i = 0; i < boxCtx.getInputChunkCount(0); ++i) {
        OV_ERROR_UNLESS_KRF(m_streamDecoder.decode(i), "Failed to decode chunk", Kernel::ErrorType::Internal);

        // represents the properties of the input, no data
        const CMatrix* matrix = m_streamDecoder.getOutputMatrix();

        if (m_streamDecoder.isHeaderReceived()) {
            OV_ERROR_UNLESS_KRF(!m_isStreamedMatrixHeaderReceived, "Multiple streamed matrix headers received", Kernel::ErrorType::BadInput);

            m_isStreamedMatrixHeaderReceived = true;

            if (m_typeID == OV_TypeId_Signal) {
                OV_ERROR_UNLESS_KRF(m_streamDecoder.getOutputSamplingRate() != 0, "Sampling rate can not be 0", Kernel::ErrorType::BadInput);
                std::vector<std::string> labels = get1DLabels(*matrix);

                OV_ERROR_UNLESS_KRF(
                    m_writerLib->setSignalInformation(labels, m_streamDecoder.getOutputSamplingRate(), matrix->getDimensionSize(1)),
                    (CSV::ICSVHandler::getLogError(m_writerLib->getLastLogError()) + (m_writerLib->getLastErrorString().empty()
                        ? "" : "Details: " + m_writerLib->getLastErrorString())).c_str(), Kernel::ErrorType::Internal);

                if (m_writeHeader) {
                    OV_ERROR_UNLESS_KRF(m_writerLib->writeHeaderToFile(),
                                        (CSV::ICSVHandler::getLogError(m_writerLib->getLastLogError()) + (m_writerLib->getLastErrorString().empty()
                                            ? "" : "Details: " + m_writerLib->getLastErrorString())).c_str(), Kernel::ErrorType::Internal);
                }
            }
            else if (m_typeID == OV_TypeId_StreamedMatrix || m_typeID == OV_TypeId_CovarianceMatrix) {
                std::vector<std::string> labels = get2DLabels(*matrix);
                std::vector<size_t> sizes;
                for (size_t d1 = 0; d1 < matrix->getDimensionCount(); ++d1) { sizes.push_back(matrix->getDimensionSize(d1)); }

                OV_ERROR_UNLESS_KRF(m_writerLib->setStreamedMatrixInformation(sizes, labels),
                                    (CSV::ICSVHandler::getLogError(m_writerLib->getLastLogError()) + (m_writerLib->getLastErrorString().empty()
                                        ? "" : "Details: " + m_writerLib->getLastErrorString())).c_str(), Kernel::ErrorType::Internal);

                if (m_writeHeader) {
                    OV_ERROR_UNLESS_KRF(m_writerLib->writeHeaderToFile(),
                                        (CSV::ICSVHandler::getLogError(m_writerLib->getLastLogError()) + (m_writerLib->getLastErrorString().empty()
                                            ? "" : "Details: " + m_writerLib->getLastErrorString())).c_str(), Kernel::ErrorType::Internal);
                }
            }
            else if (m_typeID == OV_TypeId_FeatureVector) {
                std::vector<std::string> labels = get1DLabels(*matrix);

                OV_ERROR_UNLESS_KRF(m_writerLib->setFeatureVectorInformation(labels),
                                    (CSV::ICSVHandler::getLogError(m_writerLib->getLastLogError()) + (m_writerLib->getLastErrorString().empty()
                                        ? "" : "Details: " + m_writerLib->getLastErrorString())).c_str(), Kernel::ErrorType::Internal);

                if (m_writeHeader) {
                    OV_ERROR_UNLESS_KRF(m_writerLib->writeHeaderToFile(),
                                        (CSV::ICSVHandler::getLogError(m_writerLib->getLastLogError()) + (m_writerLib->getLastErrorString().empty()
                                            ? "" : "Details: " + m_writerLib->getLastErrorString())).c_str(), Kernel::ErrorType::Internal);
                }
            }
            else if (m_typeID == OV_TypeId_Spectrum) {
                const CMatrix* frequencyAbscissaMatrix = m_streamDecoder.getOutputFrequencyAbcissa();
                std::vector<std::string> labels        = get1DLabels(*matrix);
                std::vector<double> frequencyAbscissa;

                for (size_t j = 0; j < frequencyAbscissaMatrix->getDimensionSize(0); ++j) {
                    frequencyAbscissa.push_back(frequencyAbscissaMatrix->getBuffer()[j]);
                }

                OV_ERROR_UNLESS_KRF(m_writerLib->setSpectrumInformation(labels, frequencyAbscissa, m_streamDecoder.getOutputSamplingRate()),
                                    (CSV::ICSVHandler::getLogError(m_writerLib->getLastLogError()) + (m_writerLib->getLastErrorString().empty()
                                        ? "" : "Details: " + m_writerLib->getLastErrorString())).c_str(), Kernel::ErrorType::Internal);

                if (m_writeHeader) {
                    OV_ERROR_UNLESS_KRF(m_writerLib->writeHeaderToFile(),
                                        (CSV::ICSVHandler::getLogError(m_writerLib->getLastLogError()) + (m_writerLib->getLastErrorString().empty()
                                            ? "" : "Details: " + m_writerLib->getLastErrorString())).c_str(), Kernel::ErrorType::Internal);
                }
            }
        }

        if (m_streamDecoder.isBufferReceived()) {
            const CMatrix* imatrix = m_streamDecoder.getOutputMatrix();

            if (m_typeID == OV_TypeId_Signal) {
                const uint64_t sampling       = m_streamDecoder.getOutputSamplingRate();
                const uint64_t chunkStartTime = boxCtx.getInputChunkStartTime(0, i);
                const size_t nChannel         = matrix->getDimensionSize(0);
                const size_t nSample          = matrix->getDimensionSize(1);

                for (size_t sampleIndex = 0; sampleIndex < nSample; ++sampleIndex) {
                    std::vector<double> matrixValues;
                    // get starting and ending time

                    const uint64_t timeOfNthSample       = CTime(sampling, sampleIndex).time(); // assuming chunk start is 0
                    const uint64_t sampleTime            = chunkStartTime + timeOfNthSample;
                    const double startTime               = CTime(sampleTime).toSeconds();
                    const uint64_t timeOfNthAndOneSample = CTime(sampling, sampleIndex + 1).time();
                    const double endTime                 = CTime(chunkStartTime + timeOfNthAndOneSample).toSeconds();

                    // get matrix values
                    for (size_t channelIndex = 0; channelIndex < nChannel; ++channelIndex) {
                        matrixValues.push_back(imatrix->getBuffer()[channelIndex * nSample + sampleIndex]);
                    }

                    // add sample to the library
                    OV_ERROR_UNLESS_KRF(m_writerLib->addSample({ startTime, endTime, matrixValues, m_epoch }),
                                        (CSV::ICSVHandler::getLogError(m_writerLib->getLastLogError()) + (m_writerLib->getLastErrorString().empty()
                                            ? "" : "Details: " + m_writerLib->getLastErrorString())).c_str(), Kernel::ErrorType::Internal);
                }
            }
            else if (m_typeID == OV_TypeId_StreamedMatrix || m_typeID == OV_TypeId_CovarianceMatrix) {
                const double startTime = CTime(boxCtx.getInputChunkStartTime(0, i)).toSeconds();
                const double endTime   = CTime(boxCtx.getInputChunkEndTime(0, i)).toSeconds();
                const std::vector<double> streamedMatrixValues(matrix->getBuffer(), matrix->getBuffer() + matrix->getBufferElementCount());

                OV_ERROR_UNLESS_KRF(m_writerLib->addSample({ startTime, endTime, streamedMatrixValues, m_epoch }),
                                    (CSV::ICSVHandler::getLogError(m_writerLib->getLastLogError()) + (m_writerLib->getLastErrorString().empty()
                                        ? "" : "Details: " + m_writerLib->getLastErrorString())).c_str(), Kernel::ErrorType::Internal);
            }
            else if (m_typeID == OV_TypeId_FeatureVector) {
                const double startTime = CTime(boxCtx.getInputChunkStartTime(0, i)).toSeconds();
                const double endTime   = CTime(boxCtx.getInputChunkEndTime(0, i)).toSeconds();
                const CMatrix* zmatrix = m_streamDecoder.getOutputMatrix();

                const std::vector<double> streamedMatrixValues(zmatrix->getBuffer(), zmatrix->getBuffer() + zmatrix->getBufferElementCount());

                OV_ERROR_UNLESS_KRF(m_writerLib->addSample({ startTime, endTime, streamedMatrixValues, m_epoch }),
                                    (CSV::ICSVHandler::getLogError(m_writerLib->getLastLogError()) + (m_writerLib->getLastErrorString().empty()
                                        ? "" : "Details: " + m_writerLib->getLastErrorString())).c_str(), Kernel::ErrorType::Internal);
            }
            else if (m_typeID == OV_TypeId_Spectrum) {
                const double startTime = CTime(boxCtx.getInputChunkStartTime(0, i)).toSeconds();
                const double endTime   = CTime(boxCtx.getInputChunkEndTime(0, i)).toSeconds();
                const std::vector<double> streamedMatrixValues(matrix->getBuffer(), matrix->getBuffer() + matrix->getBufferElementCount());

                OV_ERROR_UNLESS_KRF(m_writerLib->addSample({ startTime, endTime, streamedMatrixValues, m_epoch }),
                                    (CSV::ICSVHandler::getLogError(m_writerLib->getLastLogError()) + (m_writerLib->getLastErrorString().empty()
                                        ? "" : "Details: " + m_writerLib->getLastErrorString())).c_str(), Kernel::ErrorType::Internal);
            }

            m_epoch++;
        }

        OV_ERROR_UNLESS_KRF(boxCtx.markInputAsDeprecated(0, i), "Fail to mark input as deprecated", Kernel::ErrorType::Internal);
    }

    return true;
}

///-------------------------------------------------------------------------------------------------
bool CBoxAlgorithmOVCSVFileWriter::processStimulation()
{
    Kernel::IBoxIO& boxCtx = this->getDynamicBoxContext();

    // add every stimulation received
    for (size_t i = 0; i < boxCtx.getInputChunkCount(m_stimIdx); ++i) {
        OV_ERROR_UNLESS_KRF(m_stimDecoder.decode(i), "Failed to decode stimulation chunk", Kernel::ErrorType::Internal);
        if (m_stimDecoder.isHeaderReceived()) {
            OV_ERROR_UNLESS_KRF(!m_isStimulationsHeaderReceived, "Multiple Stimulations headers received", Kernel::ErrorType::BadInput);

            m_isStimulationsHeaderReceived = true;

            if (this->getStaticBoxContext().getInputCount() == 1) {
                if (m_writeHeader) {
                    OV_ERROR_UNLESS_KRF(m_writerLib->writeHeaderToFile(),
                                        (CSV::ICSVHandler::getLogError(m_writerLib->getLastLogError()) + (m_writerLib->getLastErrorString().empty()
                                            ? "" : "Details: " + m_writerLib->getLastErrorString())).c_str(), Kernel::ErrorType::Internal);
                }
            }
        }
        else if (m_stimDecoder.isBufferReceived()) {
            const CStimulationSet* stimSet = m_stimDecoder.getOutputStimulationSet();
            // for each stimulation, get its informations

            for (size_t j = 0; j < stimSet->size(); ++j) {
                OV_ERROR_UNLESS_KRF(
                    m_writerLib->addEvent({ stimSet->getId(j), CTime(stimSet->getDate(j)).toSeconds(), CTime(stimSet->getDuration(j)).
                        toSeconds() }), (CSV::ICSVHandler::getLogError(m_writerLib->getLastLogError()) + (m_writerLib->getLastErrorString().empty()
                        ? "" : "Details: " + m_writerLib->getLastErrorString())).c_str(), Kernel::ErrorType::Internal);
            }

            // set NoEventUntilDate to prevent time that will be empty of stimulations until the end of the last chunk
            OV_ERROR_UNLESS_KRF(
                m_writerLib->noEventsUntilDate(CTime(boxCtx.getInputChunkEndTime(m_stimIdx, (boxCtx.getInputChunkCount(m_stimIdx) -
                    1))).toSeconds()),
                (CSV::ICSVHandler::getLogError(m_writerLib->getLastLogError()) + (m_writerLib->getLastErrorString().empty()
                    ? "" : "Details: " + m_writerLib-> getLastErrorString())).c_str(), Kernel::ErrorType::Internal);
        }

        OV_ERROR_UNLESS_KRF(boxCtx.markInputAsDeprecated(m_stimIdx, i),
                            "Failed to mark stimulations input as deprecated", Kernel::ErrorType::Internal);
    }

    return true;
}

///-------------------------------------------------------------------------------------------------
std::vector<std::string> CBoxAlgorithmOVCSVFileWriter::get1DLabels(const CMatrix& matrix)
{
    std::vector<std::string> labels;
    for (size_t j = 0; j < matrix.getDimensionSize(0); ++j) {
        labels.push_back(transformLabel(matrix.getDimensionLabel(0, j)));
        std::cout << "Label " << j << " : " << labels[j] << std::endl;
    }
    return labels;
}

///-------------------------------------------------------------------------------------------------
std::vector<std::string> CBoxAlgorithmOVCSVFileWriter::get2DLabels(const CMatrix& matrix)
{
    std::vector<std::string> labels;
    for (size_t d1 = 0; d1 < matrix.getDimensionCount(); ++d1) {
        for (size_t d2 = 0; d2 < matrix.getDimensionSize(d1); ++d2) { labels.push_back(transformLabel(matrix.getDimensionLabel(d1, d2))); }
    }
    return labels;
}

///-------------------------------------------------------------------------------------------------
std::string CBoxAlgorithmOVCSVFileWriter::transformLabel(const std::string& str)
{
    return std::regex_replace(str, std::regex("\r*\n"), "_newLine_");   // Windows Line Break with \r before \n
}

}  // namespace FileIO
}  // namespace Plugins
}  // namespace OpenViBE
