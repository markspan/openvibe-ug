// UG fork: the errors about settings and files here say what is wrong and what to do, politely. See UG.md, 2.23.
#include "ovpCBoxAlgorithmCSVFileWriter.h"

#include <string>
#include <iostream>

namespace OpenViBE {
namespace Plugins {
namespace FileIO {

bool CBoxAlgorithmCSVFileWriter::initialize()
{
    this->getStaticBoxContext().getInputType(0, m_typeID);

    m_separator = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);

    if (this->getTypeManager().isDerivedFromStream(m_typeID, OV_TypeId_StreamedMatrix)) {
        if (m_typeID == OV_TypeId_Signal) {
            m_decoder = new Toolkit::TSignalDecoder<CBoxAlgorithmCSVFileWriter>();
            m_decoder->initialize(*this, 0);
        }
            //else if(m_typeID==OV_TypeId_Spectrum)
            //{
            // m_decoder=new Toolkit::TSpectrumDecoder < CBoxAlgorithmCSVFileWriter >();
            // m_decoder->initialize(*this,0);
            //}
        else if (m_typeID == OV_TypeId_FeatureVector) {
            m_decoder = new Toolkit::TFeatureVectorDecoder<CBoxAlgorithmCSVFileWriter>();
            m_decoder->initialize(*this, 0);
        }
        else {
            if (m_typeID != OV_TypeId_StreamedMatrix) {
                this->getLogManager() << Kernel::LogLevel_Info <<
                        "Input is a type derived from matrix that the box doesn't recognize, decoding as Streamed Matrix\n";
            }
            m_decoder = new Toolkit::TStreamedMatrixDecoder<CBoxAlgorithmCSVFileWriter>();
            m_decoder->initialize(*this, 0);
        }
        m_realProcess = &CBoxAlgorithmCSVFileWriter::processStreamedMatrix;
    }
    else if (m_typeID == OV_TypeId_Stimulations) {
        m_decoder = new Toolkit::TStimulationDecoder<CBoxAlgorithmCSVFileWriter>();
        m_decoder->initialize(*this, 0);
        m_realProcess = &CBoxAlgorithmCSVFileWriter::processStimulation;
    }
    else { OV_ERROR_KRF("Invalid input type identifier " << this->getTypeManager().getTypeName(m_typeID), Kernel::ErrorType::BadInput); }

    m_nSample = 0;

    m_firstBuffer    = true;
    m_headerReceived = false;

    return true;
}

bool CBoxAlgorithmCSVFileWriter::uninitialize()
{
    if (m_fileStream.is_open()) { m_fileStream.close(); }

    if (m_decoder) {
        m_decoder->uninitialize();
        delete m_decoder;
    }

    return true;
}

bool CBoxAlgorithmCSVFileWriter::initializeFile()
{
    const CString filename   = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
    const uint64_t precision = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2);

    m_fileStream.open(filename.toASCIIString(), std::ios::trunc);

    OV_ERROR_UNLESS_KRF(m_fileStream.is_open(), "the file [" << filename << "] from this box's setting \"Filename\" could not be opened for writing. Paths need forward slashes, as a backslash is lost on the way to a box. Perhaps its folder does not exist, or the file is open in another program. Might we trouble you to check? Thank you most kindly.", Kernel::ErrorType::BadFileWrite);

    m_fileStream << std::scientific;
    m_fileStream.precision(std::streamsize(precision));

    return true;
}

bool CBoxAlgorithmCSVFileWriter::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

bool CBoxAlgorithmCSVFileWriter::process()
{
    if (!m_fileStream.is_open()) { if (!initializeFile()) { return false; } }
    return (this->*m_realProcess)();
}

bool CBoxAlgorithmCSVFileWriter::processStreamedMatrix()
{
    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();
    for (size_t i = 0; i < boxContext.getInputChunkCount(0); ++i) {
        const uint64_t tStart = boxContext.getInputChunkStartTime(0, i);
        const uint64_t tEnd   = boxContext.getInputChunkEndTime(0, i);

        m_decoder->decode(i);

        if (m_decoder->isHeaderReceived()) {
            if (!m_headerReceived) {
                m_headerReceived = true;

                const CMatrix* matrix = static_cast<Toolkit::TStreamedMatrixDecoder<CBoxAlgorithmCSVFileWriter>*>(m_decoder)->getOutputMatrix();

                OV_ERROR_UNLESS_KRF(matrix->getDimensionCount() == 1 || matrix->getDimensionCount() == 2,
                                    "Invalid input matrix: must have 1 or 2 dimensions", Kernel::ErrorType::BadInput);

                if (matrix->getDimensionCount() == 1 || m_typeID == OV_TypeId_FeatureVector) {
                    // The matrix is a vector, make a matrix to represent it. This [n X 1] will get written as a single row due to transpose later
                    m_oMatrix.resize(matrix->getDimensionSize(0), 1);
                    for (size_t j = 0; j < matrix->getDimensionSize(0); ++j) { m_oMatrix.setDimensionLabel(0, j, matrix->getDimensionLabel(0, j)); }
                }
                else {
                    // As-is
                    m_oMatrix.copyDescription(*matrix);
                }
                // std::cout<<&m_Matrix<<" "<<&op_pMatrix<<"\n";
                m_fileStream << "Time (s)";
                for (size_t c = 0; c < m_oMatrix.getDimensionSize(0); ++c) {
                    std::string label(m_oMatrix.getDimensionLabel(0, c));
                    while (label.length() > 0 && label[label.length() - 1] == ' ') { label.erase(label.length() - 1); }
                    m_fileStream << m_separator.toASCIIString() << label;
                }

                if (m_typeID == OV_TypeId_Signal) { m_fileStream << m_separator.toASCIIString() << "Sampling Rate"; }
                else if (m_typeID == OV_TypeId_Spectrum) {
                    m_fileStream << m_separator << "Min frequency band";
                    m_fileStream << m_separator << "Max frequency band";
                }
                else { }

                m_fileStream << "\n";
            }
            else { OV_ERROR_KRF("Multiple streamed matrix headers received", Kernel::ErrorType::BadInput); }
        }
        if (m_decoder->isBufferReceived()) {
            const CMatrix* matrix = static_cast<Toolkit::TStreamedMatrixDecoder<CBoxAlgorithmCSVFileWriter>*>(m_decoder)->getOutputMatrix();

            const size_t nChannel = m_oMatrix.getDimensionSize(0);
            const size_t nSample  = m_oMatrix.getDimensionSize(1);

            //this->getLogManager() << Kernel::LogLevel_Info << " dimsIn " << matrix->getDimensionSize(0) << "," << matrix->getDimensionSize(1) << "\n";
            //this->getLogManager() << Kernel::LogLevel_Info << " dimsBuf " << m_oMatrix.getDimensionSize(0) << "," << m_oMatrix.getDimensionSize(1) << "\n";

            for (size_t s = 0; s < nSample; ++s) {
                if (m_typeID == OV_TypeId_StreamedMatrix || m_typeID == OV_TypeId_FeatureVector) { m_fileStream << CTime(tStart).toSeconds(); }
                else if (m_typeID == OV_TypeId_Signal) {
                    const uint64_t frequency = static_cast<Toolkit::TSignalDecoder<CBoxAlgorithmCSVFileWriter>*>(m_decoder)->
                            getOutputSamplingRate();
                    const uint64_t timeOfNthSample = CTime(frequency, s).time(); // assuming chunk start is 0
                    const uint64_t sampleTime      = tStart + timeOfNthSample;

                    m_fileStream << CTime(sampleTime).toSeconds();
                }
                else if (m_typeID == OV_TypeId_Spectrum) { m_fileStream << CTime(tEnd).toSeconds(); }
                for (size_t c = 0; c < nChannel; ++c) { m_fileStream << m_separator.toASCIIString() << matrix->getBuffer()[c * nSample + s]; }

                if (m_firstBuffer) {
                    if (m_typeID == OV_TypeId_Signal) {
                        const uint64_t frequency = static_cast<Toolkit::TSignalDecoder<CBoxAlgorithmCSVFileWriter>*>(m_decoder)->
                                getOutputSamplingRate();

                        m_fileStream << m_separator.toASCIIString() << uint64_t(frequency);

                        m_firstBuffer = false;
                    }
                    else if (m_typeID == OV_TypeId_Spectrum) {
                        // This should not be supported anymore
                        // This is not the correct formula
                        const CMatrix* freq = static_cast<Toolkit::TSpectrumDecoder<CBoxAlgorithmCSVFileWriter>*>(m_decoder)->
                                getOutputFrequencyAbscissa();
                        const double half = s > 0 ? (freq->getBuffer()[s] - freq->getBuffer()[s - 1]) / 2.0
                                                : (freq->getBuffer()[s + 1] - freq->getBuffer()[s]) / 2.0;
                        m_fileStream << m_separator.toASCIIString() << (freq->getBuffer()[s] - half);
                        m_fileStream << m_separator.toASCIIString() << (freq->getBuffer()[s] + half);
                    }
                    else { }
                }
                else {
                    if (m_typeID == OV_TypeId_Signal) { m_fileStream << m_separator.toASCIIString(); }
                    else if (m_typeID == OV_TypeId_Spectrum) { m_fileStream << m_separator.toASCIIString() << m_separator.toASCIIString(); }
                    else { }
                }

                m_fileStream << "\n";
            }
            m_nSample += nSample;

            m_firstBuffer = false;
        }
        if (m_decoder->isEndReceived()) { }
        boxContext.markInputAsDeprecated(0, i);
    }

    return true;
}

bool CBoxAlgorithmCSVFileWriter::processStimulation()
{
    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();

    for (size_t i = 0; i < boxContext.getInputChunkCount(0); ++i) {
        m_decoder->decode(i);
        if (m_decoder->isHeaderReceived()) {
            if (!m_headerReceived) {
                m_headerReceived = true;
                m_fileStream << "Time (s)" << m_separator.toASCIIString() << "Identifier" << m_separator.toASCIIString() << "Duration\n";
            }
            else { OV_ERROR_KRF("Multiple stimulation headers received", Kernel::ErrorType::BadInput); }
        }
        if (m_decoder->isBufferReceived()) {
            const CStimulationSet* stimSet = static_cast<Toolkit::TStimulationDecoder<CBoxAlgorithmCSVFileWriter>*>(m_decoder)->
                    getOutputStimulationSet();
            for (size_t j = 0; j < stimSet->size(); ++j) {
                m_fileStream << CTime(stimSet->getDate(j)).toSeconds() << m_separator.toASCIIString()
                        << stimSet->getId(j) << m_separator.toASCIIString()
                        << CTime(stimSet->getDuration(j)).toSeconds() << "\n";
            }
        }
        if (m_decoder->isEndReceived()) { }
        boxContext.markInputAsDeprecated(0, i);
    }

    return true;
}

}  // namespace FileIO
}  // namespace Plugins
}  // namespace OpenViBE
