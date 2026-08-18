#if defined TARGET_HAS_ThirdPartyMatlab

#include "ovpCMatlabHelper.h"
#include <iostream>
#include <sstream>
#include <mex.h>
//#include <engine.h>
#include <string>

#if defined TARGET_OS_Windows
#include <windows.h>
#endif

namespace OpenViBE {
namespace Plugins {
namespace Matlab {

//---------------------------------------------------------------------------------------------------------------
static std::string escapeMatlabString(const char* sStringToEscape)
{
    std::string str = std::string(sStringToEscape);
    auto it         = str.begin();
    while (it != str.end())
    {
        if (*it == '\'')
        {
            str.insert(it, '\'');
            ++it;
        }
        ++it;
    }
    return str;
}

static std::string genLabelsList(CMatrix* matrix, const size_t axis = 0)
{
    std::string labelsList;
    for (size_t i = 0; i < matrix->getDimensionSize(axis); ++i)
    {
        labelsList += std::string("'") + escapeMatlabString(matrix->getDimensionLabel(axis, i)) + "' ";
    }
    return labelsList;
}


static uint64_t convertFromMArray(mxArray* array, const size_t index)
{
    const double* ptr      = ::mxGetPr(array);
    const double timeValue = double(ptr[index]);
    return CTime(timeValue).time();
}

static uint64_t castFromMArray(mxArray* array, const size_t index)
{
    const double* ptr  = ::mxGetPr(array);
    const double value = double(ptr[index]);
    return uint64_t(value);
}

static CString getNameFromCell(mxArray* names, const size_t index)
{
    mxArray* cell = mxGetCell(names, index);
    if (!cell) { return ""; }
    const mwSize* cellSizes = mxGetDimensions(cell);
    char* name              = new char[cellSizes[1] + 1];
    name[cellSizes[1]]      = '\0';
    for (size_t cellsize = 0; cellsize < size_t(cellSizes[1]); ++cellsize) { name[cellsize] = static_cast<char*>(mxGetData(cell))[cellsize * 2]; }
    CString res = name;
    delete name;
    return res;
}

std::vector<CString> CMatlabHelper::getNamelist(const char* name) const
{
    std::vector<CString> res;

    mxArray* marray = engGetVariable(m_matlabEngine, name);
    if (!marray)
    {
        this->getLogManager() << Kernel::LogLevel_Error << "Nonexisting variable [" << name << "]\n";
        return res;
    }

    const mwSize nbCells = mxGetNumberOfElements(marray);
    for (size_t cell = 0; cell < nbCells; ++cell) { res.push_back(getNameFromCell(marray, cell)); }

    mxDestroyArray(marray);
    return res;
}

uint32_t CMatlabHelper::getUi32FromEnv(const char* name) const
{
    mxArray* marray = engGetVariable(m_matlabEngine, name);
    if (!marray)
    {
        this->getLogManager() << Kernel::LogLevel_Error << "Nonexisting variable [" << name << "]\n";
        return 0;
    }
    const uint32_t res = uint32_t(*mxGetPr(marray));
    mxDestroyArray(marray);
    return res;
}

uint64_t CMatlabHelper::getUi64FromEnv(const char* name) const
{
    mxArray* marray = engGetVariable(m_matlabEngine, name);
    if (!marray)
    {
        this->getLogManager() << Kernel::LogLevel_Error << "Nonexisting variable [" << name << "]\n";
        return 0;
    }
    const uint64_t res = uint64_t(*mxGetPr(marray));
    mxDestroyArray(marray);
    return res;
}

uint64_t CMatlabHelper::genUi64FromEnvConverted(const char* name) const
{
    mxArray* marray = engGetVariable(m_matlabEngine, name);
    if (!marray)
    {
        this->getLogManager() << Kernel::LogLevel_Error << "Nonexisting variable [" << name << "]\n";
        return 0;
    }

    const double value = double(*mxGetPr(marray));
    const uint64_t res = CTime(value).time();
    mxDestroyArray(marray);
    return res;
}
//---------------------------------------------------------------------------------------------------------------

bool CMatlabHelper::setStreamedMatrixInputHeader(const size_t index, CMatrix* matrix) const
{
    std::string labelList;
    std::string dimensionSizes;
    for (size_t dim = 0; dim < matrix->getDimensionCount(); ++dim)
    {
        dimensionSizes += std::to_string(matrix->getDimensionSize(dim)) + " ";
        labelList += std::string("{") + genLabelsList(matrix, dim) + "} ";
    }
    //function box_out = OV_setStreamedMatrixInputInputHeader(box_in, input_index, dimension_count, dimension_sizes, dimension_labels)

    const std::string cmd = std::string(m_boxInstanceVariableName) + " = OV_setStreamedMatrixInputHeader(" + m_boxInstanceVariableName.toASCIIString() + ","
                            + std::to_string(index + 1) + "," + std::to_string(matrix->getDimensionCount()) + "," + "[" + dimensionSizes + "],"
                            + "{" + labelList + "});";

    return engEvalString(m_matlabEngine, cmd.c_str()) == 0;
}

bool CMatlabHelper::setFeatureVectorInputHeader(const size_t index, CMatrix* matrix) const
{
    const std::string labelList = genLabelsList(matrix, 0);

    //function box_out = OV_setStreamedMatrixInputInputHeader(box_in, input_index, dimension_count, dimension_sizes, dimension_labels)

    //box_out = OV_setFeatureVectorInputHeader(box_in, input_index, nb_features, labels)
    const std::string cmd = std::string(m_boxInstanceVariableName.toASCIIString()) + " = OV_setFeatureVectorInputHeader("
                            + m_boxInstanceVariableName.toASCIIString() + "," + std::to_string(index + 1) + ","
                            + std::to_string(matrix->getDimensionSize(0)) + "," + "{" + labelList + "});";

    return engEvalString(m_matlabEngine, cmd.c_str()) == 0;
}

bool CMatlabHelper::setSignalInputHeader(const size_t index, CMatrix* matrix, const uint64_t frequency) const
{
    const std::string labelList = genLabelsList(matrix, 0);
    //function box_out = ov_set_signal_input_header(box_in,  input_index, nb_channel, nb_samples_per_buffer, channel_names, sampling_rate)

    const std::string cmd = std::string(m_boxInstanceVariableName) + " = OV_setSignalInputHeader(" + m_boxInstanceVariableName.toASCIIString() + ","
                            + std::to_string(index + 1) + "," + std::to_string(matrix->getDimensionSize(0)) + ","
                            + std::to_string(matrix->getDimensionSize(1)) + "," + "{" + labelList + "}," + std::to_string(frequency) + ");";

    return engEvalString(m_matlabEngine, cmd.c_str()) == 0;
}

bool CMatlabHelper::setChannelLocalisationInputHeader(const size_t index, CMatrix* matrix, const bool dynamic) const
{
    const std::string labelList = genLabelsList(matrix, 0);
    //function  box_out = OV_setChannelLocalisationInputHeader(box_in, input_index, nb_channels, channel_names, dynamic)

    const std::string cmd = std::string(m_boxInstanceVariableName) + " = OV_setChannelLocalisationInputHeader(" + m_boxInstanceVariableName.toASCIIString()
                            + "," + std::to_string(index + 1) + "," + std::to_string(matrix->getDimensionSize(0)) + "," + "{" + labelList + "},"
                            + (dynamic ? "true" : "false") + ");";

    return engEvalString(m_matlabEngine, cmd.c_str()) == 0;
}

bool CMatlabHelper::setSpectrumInputHeader(const size_t index, CMatrix* matrix, CMatrix* frequencyAbscissa, const uint64_t frequency) const
{
    const std::string labelList    = genLabelsList(matrix, 0);
    const std::string abscissaList = genLabelsList(matrix, 1);

    // @FIXME CERT this is now one dim array
    mxArray* matlabMatrix = ::mxCreateDoubleMatrix(frequencyAbscissa->getDimensionSize(0), 1, mxREAL);

    memcpy(mxGetPr(matlabMatrix), frequencyAbscissa->getBuffer(), frequencyAbscissa->getBufferElementCount() * sizeof(double));
    engPutVariable(m_matlabEngine, "OV_MATRIX_TMP", matlabMatrix);

    mxDestroyArray(matlabMatrix);

    //box_out = OV_setSpectrumInputHeader(box_in, input_index, nb_channels, channel_names, nb_bands, band_names, bands, sampling_rate)
    const std::string cmd = std::string(m_boxInstanceVariableName.toASCIIString()) + " = OV_setSpectrumInputHeader("
                            + std::string(m_boxInstanceVariableName.toASCIIString()) + ","
                            + std::to_string(index + 1) + "," // input_index
                            + std::to_string(matrix->getDimensionSize(0)) + "," // nb_channels
                            + "{" + labelList + "}," // channel_names
                            + std::to_string(matrix->getDimensionSize(1)) + "," // nb_freq abscissa
                            + "{" + abscissaList + "}," //freq abscissa names
                            + "OV_MATRIX_TMP," //bands
                            + std::to_string(frequency) + ");"; //sampling rate

    return engEvalString(m_matlabEngine, cmd.c_str()) == 0;
}

bool CMatlabHelper::setStimulationsInputHeader(const size_t index) const
{
    //box_out = OV_setStimulationInputHeader(box_in, input_index)
    const std::string cmd = std::string(m_boxInstanceVariableName) + " = OV_setStimulationsInputHeader(" + m_boxInstanceVariableName.toASCIIString() + "," +
                            std::to_string(index + 1) + ");";

    return engEvalString(m_matlabEngine, cmd.c_str()) == 0;
}

//bool setExperimentInfoInputHeader(CMatrix * pMatrix);

//---------------------------------------------------------------------------------------------------------------

bool CMatlabHelper::addStreamedMatrixInputBuffer(const size_t index, CMatrix* matrix, const uint64_t startTime, const uint64_t endTime) const
{
    mwSize* dims = new mwSize[matrix->getDimensionCount()];
    size_t j     = matrix->getDimensionCount() - 1;
    for (size_t i = 0; i < matrix->getDimensionCount(); ++i)
    {
        dims[i] = matrix->getDimensionSize(j);
        j--;
    }

    mxArray* matlabMatrix = ::mxCreateNumericArray(matrix->getDimensionCount(), dims, mxDOUBLE_CLASS, mxREAL);

    //test : channel 1 samples to '10'
    //for (size_t i = 0; i < 32;i++) matrix->getBuffer()[i] = 10;

    memcpy(mxGetPr(matlabMatrix), matrix->getBuffer(), matrix->getBufferElementCount() * sizeof(double));
    engPutVariable(m_matlabEngine, "OV_MATRIX_TMP", matlabMatrix);

    mxDestroyArray(matlabMatrix);

    const std::string cmd = std::string(m_boxInstanceVariableName) + " = OV_addInputBuffer(" + m_boxInstanceVariableName.toASCIIString() + ","
                            + std::to_string(index + 1) + ","
                            + std::to_string(CTime(startTime).toSeconds()) + ","
                            + std::to_string(CTime(endTime).toSeconds()) + ",OV_MATRIX_TMP');";
    // please note the transpose operator ' to put the matrix with  1 channel per line

    delete[] dims;

    return engEvalString(m_matlabEngine, cmd.c_str()) == 0;
}

bool CMatlabHelper::addStimulationsInputBuffer(const size_t index, CStimulationSet* stimSet, const uint64_t startTime, const uint64_t endTime) const
{
    if (stimSet->size() == 0 && engEvalString(m_matlabEngine, "OV_MATRIX_TMP = 0") != 0) { return false; }
    // we create a 3xN matrix for N stims (access is easier in that order)
    mxArray* matrix = ::mxCreateDoubleMatrix(3, size_t(stimSet->size()), mxREAL);

    for (size_t i = 0; i < stimSet->size(); ++i)
    {
        ::mxGetPr(matrix)[i * 3]     = double(stimSet->getId(i));
        ::mxGetPr(matrix)[i * 3 + 1] = CTime(stimSet->getDate(i)).toSeconds();
        ::mxGetPr(matrix)[i * 3 + 2] = CTime(stimSet->getDuration(i)).toSeconds();
    }

    engPutVariable(m_matlabEngine, "OV_MATRIX_TMP", matrix);

    mxDestroyArray(matrix);

    const std::string cmd = std::string(m_boxInstanceVariableName) + " = OV_addInputBuffer(" + m_boxInstanceVariableName.toASCIIString() + ","
                            + std::to_string(index + 1) + ","
                            + std::to_string(CTime(startTime).toSeconds()) + ","
                            + std::to_string(CTime(endTime).toSeconds()) + ",OV_MATRIX_TMP');";

    return (engEvalString(m_matlabEngine, cmd.c_str()) == 0);
}

//---------------------------------------------------------------------------------------------------------------

bool CMatlabHelper::getStreamedMatrixOutputHeader(const size_t index, CMatrix* matrix) const
{
    const std::string cmd = std::string("[OV_ERRNO, OV_NB_DIMENSIONS, OV_DIMENSION_SIZES, OV_DIMENSION_LABELS] = OV_getStreamedMatrixOutputHeader(")
                            + m_boxInstanceVariableName.toASCIIString() + "," + std::to_string(index + 1) + ");";
    OV_ERROR_UNLESS_KRF(engEvalString(m_matlabEngine, cmd.c_str()) == 0 && getUi32FromEnv("OV_ERRNO") == 0,
                        "Could not get Streamed matrix output header", Kernel::ErrorType::BadProcessing);

    const size_t nDimension = getUi32FromEnv("OV_NB_DIMENSIONS");
    mxArray* dimensionSizes = engGetVariable(m_matlabEngine, "OV_DIMENSION_SIZES");
    if (!dimensionSizes)
    {
        this->getLogManager() << Kernel::LogLevel_Error << "Nonexisting variable [OV_DIMENSION_SIZES]\n";
        return false;
    }

    std::vector<CString> nameList = getNamelist("OV_DIMENSION_LABELS");

    matrix->setDimensionCount(nDimension);
    size_t idx = 0;
    for (size_t i = 0; i < nDimension; ++i)
    {
        matrix->setDimensionSize(i, size_t(mxGetPr(dimensionSizes)[i]));
        for (size_t x = 0; x < matrix->getDimensionSize(i) && idx < nameList.size(); ++x)
        {
            matrix->setDimensionLabel(i, x, escapeMatlabString(nameList[idx].toASCIIString()).c_str());
            idx++;
        }
    }

    mxDestroyArray(dimensionSizes);

    return true;
}

bool CMatlabHelper::getFeatureVectorOutputHeader(const size_t index, CMatrix* matrix) const
{
    const std::string cmd = std::string("[OV_ERRNO, OV_NB_FEATURES, OV_LABELS] = OV_getFeatureVectorOutputHeader(")
                            + m_boxInstanceVariableName.toASCIIString() + "," + std::to_string(index + 1) + ");";
    OV_ERROR_UNLESS_KRF(engEvalString(m_matlabEngine, cmd.c_str()) == 0 && getUi32FromEnv("OV_ERRNO") == 0,
                        "Could not get Feature Vector output header",
                        Kernel::ErrorType::BadProcessing);

    const size_t nbFeatures       = getUi32FromEnv("OV_NB_FEATURES");
    std::vector<CString> nameList = getNamelist("OV_LABELS");
    // Check nb features == nb names ?

    matrix->resize(nbFeatures);
    for (size_t x = 0; x < nbFeatures; ++x) { matrix->setDimensionLabel(0, x, escapeMatlabString(nameList[x].toASCIIString()).c_str()); }

    return true;
}

bool CMatlabHelper::getSignalOutputHeader(const size_t index, CMatrix* matrix, uint64_t& frequency) const
{
    const std::string cmd = std::string("[OV_ERRNO, OV_NB_CHANNELS, OV_NB_SAMPLES_PER_BUFFER, OV_CHANNEL_NAMES, OV_SAMPLING_RATE] = OV_getSignalOutputHeader(")
                            + m_boxInstanceVariableName.toASCIIString() + "," + std::to_string(index + 1) + ");";
    OV_ERROR_UNLESS_KRF(engEvalString(m_matlabEngine, cmd.c_str()) == 0 && getUi32FromEnv("OV_ERRNO") == 0, "Could not get Signal output header",
                        Kernel::ErrorType::BadProcessing);

    const size_t nbChannels       = getUi32FromEnv("OV_NB_CHANNELS");
    const size_t nbSamples        = getUi32FromEnv("OV_NB_SAMPLES_PER_BUFFER");
    std::vector<CString> nameList = getNamelist("OV_CHANNEL_NAMES");
    const size_t rate             = getUi32FromEnv("OV_SAMPLING_RATE");

    if (nameList.size() != nbChannels) { return false; }

    matrix->resize(nbChannels, nbSamples);
    frequency = rate;

    for (size_t x = 0; x < nbChannels; ++x) { matrix->setDimensionLabel(0, x, escapeMatlabString(nameList[x].toASCIIString()).c_str()); }

    return true;
}

bool CMatlabHelper::getChannelLocalisationOutputHeader(const size_t index, CMatrix* matrix, bool& /*dynamic*/) const
{
    const std::string cmd = std::string("[OV_ERRNO, OV_NB_CHANNELS, OV_CHANNEL_NAMES, OV_DYNAMIC] = OV_getChannelLocalisationOutputHeader(")
                            + m_boxInstanceVariableName.toASCIIString() + "," + std::to_string(index + 1) + ");";
    OV_ERROR_UNLESS_KRF(engEvalString(m_matlabEngine, cmd.c_str()) == 0 && getUi32FromEnv("OV_ERRNO") == 0,
                        "Could not get Channel Localisation output header", Kernel::ErrorType::BadProcessing);

    const size_t nChannel         = getUi32FromEnv("OV_NB_CHANNELS");
    std::vector<CString> nameList = getNamelist("OV_CHANNEL_NAMES");
    mxArray* dynamic              = engGetVariable(m_matlabEngine, "OV_DYNAMIC");
    if (!dynamic)
    {
        this->getLogManager() << Kernel::LogLevel_Error << "Nonexisting variable [OV_DYNAMIC]\n";
        return false;
    }

    if (nameList.size() != nChannel) { return false; }

    matrix->resize(nChannel, 3);

    for (size_t x = 0; x < nChannel; ++x) { matrix->setDimensionLabel(0, x, escapeMatlabString(nameList[x].toASCIIString()).c_str()); }

    mxDestroyArray(dynamic);

    return true;
}

bool CMatlabHelper::getSpectrumOutputHeader(const size_t index, CMatrix* matrix, CMatrix* frequencyAbscissa, uint64_t& frequency) const
{
    const std::string command = std::string(
                                    "[OV_ERRNO, OV_NB_CHANNELS, OV_CHANNEL_NAMES, OV_NB_BANDS, OV_BANDS_NAME, OV_BANDS_LINEAR, OV_SAMPLING_RATE] = OV_getSpectrumOutputHeader(")
                                + m_boxInstanceVariableName.toASCIIString() + "," + std::to_string(index + 1) + ");";
    OV_ERROR_UNLESS_KRF(engEvalString(m_matlabEngine, command.c_str()) == 0 && getUi32FromEnv("OV_ERRNO") == 0,
                        "Could not get Spectrum output header", Kernel::ErrorType::BadProcessing);

    const size_t nChannel                  = getUi32FromEnv("OV_NB_CHANNELS");
    std::vector<CString> nameList          = getNamelist("OV_CHANNEL_NAMES");
    const size_t nAbscissa                 = getUi32FromEnv("OV_NB_ABSCISSAS");
    std::vector<CString> freqAbscissaNames = getNamelist("OV_ABSCISSAS_NAME");
    mxArray* freqAbscissa                  = engGetVariable(m_matlabEngine, "OV_ABSCISSAS_LINEAR");
    if (!freqAbscissa)
    {
        this->getLogManager() << Kernel::LogLevel_Error << "Nonexisting variable [OV_ABSCISSAS_LINEAR]\n";
        return false;
    }

    frequency = getUi64FromEnv("OV_SAMPLING_RATE");

    //The Frequency abscissa list has dimensions nb_bands
    frequencyAbscissa->resize(nAbscissa);
    for (size_t x = 0; x < nAbscissa; ++x) { frequencyAbscissa->setDimensionLabel(0, x, escapeMatlabString(freqAbscissaNames[x].toASCIIString()).c_str()); }

    // Adding the bands:
    memcpy(frequencyAbscissa->getBuffer(), ::mxGetPr(freqAbscissa), nAbscissa * sizeof(double));

    matrix->resize(nChannel, nAbscissa);
    for (size_t x = 0; x < nChannel; ++x) { matrix->setDimensionLabel(0, x, escapeMatlabString(nameList[x].toASCIIString()).c_str()); }
    for (size_t x = 0; x < nAbscissa; ++x) { matrix->setDimensionLabel(1, x, escapeMatlabString(freqAbscissaNames[x].toASCIIString()).c_str()); }
    // @FIXME CERT is it me or it never copy data to matrix ?

    mxDestroyArray(freqAbscissa);

    return true;
}

bool CMatlabHelper::getStimulationsOutputHeader(size_t /*index*/, CStimulationSet* /*stimulationSet*/)
{
    // Nothing to do, the stimulation header is empty.
    return true;
}

//---------------------------------------------------------------------------------------------------------------

bool CMatlabHelper::popStreamedMatrixOutputBuffer(const size_t index, CMatrix* matrix, uint64_t& startTime, uint64_t& endTime) const
{
    const std::string buf = std::string("[") + m_boxInstanceVariableName.toASCIIString()
                            + ", OV_START_TIME, OV_END_TIME, OV_LINEAR_DATA_SIZE, OV_LINEAR_DATA] = OV_popOutputBufferReshape("
                            + m_boxInstanceVariableName.toASCIIString() + ", " + std::to_string(index + 1) + ");";
    const size_t res = engEvalString(m_matlabEngine, buf.c_str());
    if (res != 0) { return false; }

    startTime           = genUi64FromEnvConverted("OV_START_TIME");
    endTime             = genUi64FromEnvConverted("OV_END_TIME");
    const uint64_t size = getUi64FromEnv("OV_LINEAR_DATA_SIZE");
    mxArray* data       = engGetVariable(m_matlabEngine, "OV_LINEAR_DATA");
    if (!data)
    {
        this->getLogManager() << Kernel::LogLevel_Error << "Nonexisting variable [OV_LINEAR_DATA]\n";
        return false;
    }

    // ti be copied direclty in openvibe buffer, the linear matrix must be ordered line by line
    memcpy(matrix->getBuffer(), ::mxGetPr(data), size * sizeof(double));
    mxDestroyArray(data);

    return true;
}

bool CMatlabHelper::popStimulationsOutputBuffer(const size_t index, CStimulationSet* stimSet, uint64_t& startTime, uint64_t& endTime) const
{
    const std::string buf = std::string("[") + m_boxInstanceVariableName.toASCIIString()
                            + ", OV_START_TIME, OV_END_TIME, OV_LINEAR_MATRIX_SIZE, OV_LINEAR_DATA] = OV_popOutputBuffer("
                            + m_boxInstanceVariableName.toASCIIString() + ", " + std::to_string(index + 1) + ");";
    const size_t res = engEvalString(m_matlabEngine, buf.c_str());
    if (res != 0) { return false; }
    startTime         = genUi64FromEnvConverted("OV_START_TIME");
    endTime           = genUi64FromEnvConverted("OV_END_TIME");
    const size_t size = getUi32FromEnv("OV_LINEAR_MATRIX_SIZE");
    mxArray* data     = engGetVariable(m_matlabEngine, "OV_LINEAR_DATA");
    if (!data)
    {
        this->getLogManager() << Kernel::LogLevel_Error << "Nonexisting variable [OV_LINEAR_DATA]\n";
        return false;
    }

    for (size_t i = 0; i < size; i += 3)
    {
        const uint64_t id       = castFromMArray(data, i + 0);
        const uint64_t date     = convertFromMArray(data, i + 1);
        const uint64_t duration = convertFromMArray(data, i + 2);
        stimSet->push_back(id, date, duration);
    }

    mxDestroyArray(data);

    return true;
}
//--------------------------------------------------------------------------------------------------------------

}  // namespace Matlab
}  // namespace Plugins
}  // namespace OpenViBE
#endif // TARGET_HAS_ThirdPartyMatlab
