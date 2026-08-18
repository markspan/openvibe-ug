#include <iomanip>
#if defined TARGET_HAS_ThirdPartyMatlab

#include "ovpCBoxAlgorithmMatlabScripting.h"

#include <system/ovCMath.h>

#include <iostream>
#include <stdio.h>
#include <sstream>
#include <string>
#include <ctime>
#include <algorithm>

#include <mex.h>
#include <engine.h>

#include <fs/Files.h>

#if defined TARGET_OS_Windows
#include <windows.h>
#include <direct.h>
#define getCurrentDir _getcwd
#else
#include <unistd.h>
#define getCurrentDir getcwd
#endif


// Size of the internal buffer storing matlab messages, in char
#define MATLAB_BUFFER 2048
#define MATLAB_ENGINE ((Engine*)m_engineHandle)

namespace OpenViBE {
namespace Plugins {
namespace Matlab {

// Sanitizes a path so that it only has / or \ characters and has a / or \ in the end.
// @fixme should move to plugins-FS possibly
void CBoxAlgorithmMatlabScripting::sanitizePath(CString& pathToModify)
{
    std::string tmpPath(pathToModify);
    // Append / to end of path if its not there already
    if (tmpPath.length() > 0)
    {
        const char lastChar = tmpPath.at(tmpPath.length() - 1);
        if (lastChar != '\\' && lastChar != '/') { tmpPath += "/"; }
    }

#if defined TARGET_OS_Windows
    std::replace(tmpPath.begin(), tmpPath.end(), '/', '\\');    // Convert '/' to '\'
#endif
    pathToModify = tmpPath.c_str();
}

// The checkFailureRoutine() verifies the result of a matlab call (via engine or helper functions) given as argument.
// If the result is false (the matlab call failed), the message msg is printed in the Error Log Level, and the macro returns false.
// The Matlab output buffer is then printed. If an error message is detected in the buffer, the same error message is printed and
// the macro returns false. 
bool CBoxAlgorithmMatlabScripting::checkFailureRoutine(const bool result, const CString& msg)
{
    OV_ERROR_UNLESS_KRF(result, msg, Kernel::ErrorType::BadProcessing);

    m_errorDetected = false;
    this->printOutputBufferWithFormat();
    OV_ERROR_UNLESS_KRF(!m_errorDetected, msg, Kernel::ErrorType::BadProcessing);

    return true;
}

bool CBoxAlgorithmMatlabScripting::openMatlabEngineSafely()
{
    this->getLogManager() << Kernel::LogLevel_Trace << "Trying to open the Matlab engine\n";
#if defined TARGET_OS_Linux
    m_engineHandle = ::engOpen(m_matlabPath.toASCIIString());
    OV_ERROR_UNLESS_KRF(m_matlabEngine, "Could not open the Matlab engine.\nThe configured path to the matlab executable was expanded as '" << m_matlabPath << "'.", Kernel::ErrorType::BadProcessing);
}
#elif defined TARGET_OS_Windows
    __try { m_engineHandle = engOpen(nullptr); }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        this->getLogManager() << Kernel::LogLevel_Error << "First call to the MATLAB engine failed.\n";
        this->getLogManager() << Kernel::LogLevel_Error << "To use this box you must have MATLAB installed on your computer.\n";
#if defined(TARGET_ARCHITECTURE_x64)
        this->getLogManager() << Kernel::LogLevel_Error << "For this build of OpenViBE, MATLAB needs to be a 64bit version.\n";
#else
        this->getLogManager() << Kernel::LogLevel_Error << "For this build of OpenViBE, MATLAB needs to be a 32bit version.\n";
#endif
        m_engineHandle = nullptr;
    }
    if (!MATLAB_ENGINE)
    {
        this->getLogManager() << Kernel::LogLevel_Error << "Could not open the Matlab engine.\nThe matlab binary path was reasoned to be '" << m_matlabPath << "'.\n";
        return false;
    }
#else
    OV_ERROR_KRF("Only Linux and Windows are supported", Kernel::ErrorType::BadVersion);
#endif
    return true;
}

bool CBoxAlgorithmMatlabScripting::initialize()
{
    m_engineHandle = nullptr;
    m_matlabBuffer = nullptr;
    m_helper       = new CMatlabHelper(this->getLogManager(), this->getErrorManager());

    m_clockFrequency = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);

    for (size_t i = 0; i < getStaticBoxContext().getInputCount(); ++i)
    {
        CIdentifier type;
        getStaticBoxContext().getInputType(i, type);
        Toolkit::TDecoder<CBoxAlgorithmMatlabScripting>* decoder = nullptr;
        if (type == OV_TypeId_StreamedMatrix) { decoder = new Toolkit::TStreamedMatrixDecoder<CBoxAlgorithmMatlabScripting>(*this, i); }
        else if (type == OV_TypeId_ChannelLocalisation) { decoder = new Toolkit::TChannelLocalisationDecoder<CBoxAlgorithmMatlabScripting>(*this, i); }
        else if (type == OV_TypeId_FeatureVector) { decoder = new Toolkit::TFeatureVectorDecoder<CBoxAlgorithmMatlabScripting>(*this, i); }
        else if (type == OV_TypeId_Spectrum) { decoder = new Toolkit::TSpectrumDecoder<CBoxAlgorithmMatlabScripting>(*this, i); }
        else if (type == OV_TypeId_Signal) { decoder = new Toolkit::TSignalDecoder<CBoxAlgorithmMatlabScripting>(*this, i); }
        else if (type == OV_TypeId_Stimulations) { decoder = new Toolkit::TStimulationDecoder<CBoxAlgorithmMatlabScripting>(*this, i); }
        else if (type == OV_TypeId_ExperimentInfo) { decoder = new Toolkit::TExperimentInfoDecoder<CBoxAlgorithmMatlabScripting>(*this, i); }
        else { this->getLogManager() << Kernel::LogLevel_Warning << "Undefined type on input [" << i << "].\n"; }
        if (decoder != nullptr)
        {
            m_decoders.insert(std::make_pair(i, decoder));
            m_nInputHeaderSent = 0;
        }
    }

    for (size_t i = 0; i < getStaticBoxContext().getOutputCount(); ++i)
    {
        CIdentifier type;
        getStaticBoxContext().getOutputType(i, type);
        Toolkit::TEncoder<CBoxAlgorithmMatlabScripting>* encoder = nullptr;
        if (type == OV_TypeId_StreamedMatrix) { encoder = new Toolkit::TStreamedMatrixEncoder<CBoxAlgorithmMatlabScripting>(*this, i); }
        else if (type == OV_TypeId_ChannelLocalisation) { encoder = new Toolkit::TChannelLocalisationEncoder<CBoxAlgorithmMatlabScripting>(*this, i); }
        else if (type == OV_TypeId_FeatureVector) { encoder = new Toolkit::TFeatureVectorEncoder<CBoxAlgorithmMatlabScripting>(*this, i); }
        else if (type == OV_TypeId_Spectrum) { encoder = new Toolkit::TSpectrumEncoder<CBoxAlgorithmMatlabScripting>(*this, i); }
        else if (type == OV_TypeId_Signal) { encoder = new Toolkit::TSignalEncoder<CBoxAlgorithmMatlabScripting>(*this, i); }
        else if (type == OV_TypeId_Stimulations) { encoder = new Toolkit::TStimulationEncoder<CBoxAlgorithmMatlabScripting>(*this, i); }
        else if (type == OV_TypeId_ExperimentInfo) { encoder = new Toolkit::TExperimentInfoEncoder<CBoxAlgorithmMatlabScripting>(*this, i); }
        else { this->getLogManager() << Kernel::LogLevel_Warning << "Undefined type on input [" << i << "].\n"; }
        if (encoder != nullptr)
        {
            m_encoders.insert(std::make_pair(i, encoder));
            m_oHeaderStates.insert(std::make_pair(i, false));
        }
    }

    m_matlabPath = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);

#if defined TARGET_OS_Windows
    if (!FS::Files::directoryExists(m_matlabPath) && FS::Files::fileExists(m_matlabPath))
    {
        // The path might be pointing to the executable, try to extract the directory
        char parentPath[MAX_PATH];
        FS::Files::getParentPath(m_matlabPath, parentPath);
        m_matlabPath = CString(parentPath);
    }

    sanitizePath(m_matlabPath);

    this->getLogManager() << Kernel::LogLevel_Trace << "Interpreting Matlab path as '" << m_matlabPath << "'\n";

    OV_ERROR_UNLESS_KRF(FS::Files::directoryExists(m_matlabPath), "Configured Matlab path '" << m_matlabPath << "' does not seem to be a directory",
                        Kernel::ErrorType::BadConfig);

    char* path = getenv("PATH");
    OV_ERROR_UNLESS_KRF(path, "Could not access the environment variable PATH to add Matlab path to it.", Kernel::ErrorType::BadSetting);

    std::string strPath = std::string(path);
    size_t found        = strPath.find(m_matlabPath.toASCIIString());
    if (found == std::string::npos)
    {
        CString pathModif = path + CString(";") + m_matlabPath;
        OV_ERROR_UNLESS_KRF(_putenv_s("PATH", pathModif) == 0, "Failed to modify the environment PATH with the Matlab path.",
                            Kernel::ErrorType::BadProcessing);

        this->getLogManager() << Kernel::LogLevel_Trace << "Matlab Path '" << m_matlabPath << "' added to Windows PATH environment variable.\n";
    }
    else { this->getLogManager() << Kernel::LogLevel_Trace << "No need to add matlab to PATH\n"; }
#endif

    if (!openMatlabEngineSafely()) { return false; }

    m_matlabBuffer                = new char[MATLAB_BUFFER + 1];
    m_matlabBuffer[MATLAB_BUFFER] = '\0';
    engOutputBuffer(MATLAB_ENGINE, m_matlabBuffer, MATLAB_BUFFER);
    m_errorDetected = false;

    // add the openvibe toolbox to matlab path
    char curDir[FILENAME_MAX];
    OV_ERROR_UNLESS_KRF(getCurrentDir(curDir, FILENAME_MAX), "Failed to get the execution directory.", Kernel::ErrorType::BadProcessing);

    std::string curDirString = curDir;
    std::replace(curDirString.begin(), curDirString.end(), '\\', '/');

    this->getConfigurationManager().addOrReplaceConfigurationToken(CString("Path_Bin_Abs"), CString(curDirString.c_str()));

    CString cmd = CString("addpath('") + Directories::getDataDir() + "/plugins/matlab');";
    engEvalString(MATLAB_ENGINE, cmd.toASCIIString());
    //if(!checkFailureRoutine(engEvalString(m_matlabEngine, (const char * )l_sOpenvibeToolboxPath) == 0, "An error occurred while adding the path to openvibe toolbox\n")) return false;
    // If there is more than 1 Matlab box in the scenario, the path is set repeatedly
    // resulting in warning messages in the buffer. We don't print them.
    // this->printOutputBufferWithFormat(); 

    CString workingDir = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2);
    sanitizePath(workingDir);

    // Try to find an unused box instance variable name
    while (true)
    {
        std::stringstream ss;
        ss.fill('0');
        ss << "OV_BOX_0x" << std::setw(8) << std::hex << System::Math::randomI() << "_0x" << std::setw(8) << std::hex << System::Math::randomI();
        m_boxInstanceVariableName = ss.str().c_str();
        this->getLogManager() << Kernel::LogLevel_Trace << "Checking if variable " << m_boxInstanceVariableName << " is in use...\n";
        mxArray* tmp = engGetVariable(MATLAB_ENGINE, m_boxInstanceVariableName.toASCIIString());
        if (!tmp)
        {
            // Found unused name
            break;
        }
        mxDestroyArray(tmp);
    }
    this->getLogManager() << Kernel::LogLevel_Trace << "Selected variable name " << m_boxInstanceVariableName << "\n";

    this->getLogManager() << Kernel::LogLevel_Trace << "Setting working directory to " << workingDir << "\n";
    cmd = CString("cd '") + workingDir + CString("'");
    if (!checkFailureRoutine(engEvalString(MATLAB_ENGINE, cmd) == 0, "An error occurred while changing the working directory\n")) { return false; }

    // executes the pre-run routine that defines the global identifiers for streams and stimulations codes
    cmd = CString("run '") + Directories::getDataDir() + "/plugins/matlab/OV_define.m'";
    if (!checkFailureRoutine(engEvalString(MATLAB_ENGINE, cmd) == 0, "An error occurred while calling OV_define.m")) { return false; }

    cmd = CString("run '") + Directories::getDataDir() + "/plugins/matlab/OV_stimulations.m'";
    if (!checkFailureRoutine(engEvalString(MATLAB_ENGINE, cmd) == 0, "An error occurred while calling OV_stimulations.m")) { return false; }

    // executes the pre-run routine that construct the ov_box object
    cmd = m_boxInstanceVariableName + " = OV_createBoxInstance(" + std::to_string(this->getStaticBoxContext().getInputCount()).c_str() + ","
          + std::to_string(this->getStaticBoxContext().getOutputCount()).c_str() + ");";
    if (!checkFailureRoutine(engEvalString(MATLAB_ENGINE, cmd) == 0, "An error occurred while calling OV_createBoxInstance.m")) { return false; }

    //First call to a function of the openvibe toolbox
    // if it fails, the toolbox may be not installed
    mxArray* box = engGetVariable(MATLAB_ENGINE, m_boxInstanceVariableName.toASCIIString());
    OV_ERROR_UNLESS_KRF(box, "Failed to create the box instance with OV_createBoxInstance function.", Kernel::ErrorType::BadProcessing);

    m_initializeFunc   = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 3);
    m_processFunc      = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 4);
    m_uninitializeFunc = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 5);

    // Check that the files actually exist
    CString filename;
    filename = workingDir + m_initializeFunc + ".m";
    OV_ERROR_UNLESS_KRF(FS::Files::fileExists(filename.toASCIIString()), "Cannot open '" << filename << "'", Kernel::ErrorType::BadFileRead);
    filename = workingDir + m_processFunc + ".m";
    OV_ERROR_UNLESS_KRF(FS::Files::fileExists(filename.toASCIIString()), "Cannot open '" << filename << "'", Kernel::ErrorType::BadFileRead);
    filename = workingDir + m_uninitializeFunc + ".m";
    OV_ERROR_UNLESS_KRF(FS::Files::fileExists(filename.toASCIIString()), "Cannot open '" << filename << "'", Kernel::ErrorType::BadFileRead);

    CString names  = "{";
    CString types  = "{";
    CString values = "{";
    CString tmp;
    CIdentifier typeID;

    for (size_t i = 6; i < getStaticBoxContext().getSettingCount(); ++i)
    {
        // get the setting name
        getStaticBoxContext().getSettingName(i, tmp);
        names = names + "'" + tmp + "' ";

        //setting type
        getStaticBoxContext().getSettingType(i, typeID);
        std::stringstream ss;
        ss << typeID.id();
        types = types + "uint64(" + ss.str().c_str() + ") ";

        //setting value, typed

        tmp = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), i);
        if (typeID == OV_TypeId_Stimulation)
        {
            uint64_t code = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), i);
            std::stringstream ss1;
            ss1 << code;
            values = values + CString(ss1.str().c_str()) + CString(" ");
            // we keep the stimulation codes as doubles, to be able to put them in arrays with other doubles (such as timings). 
            // they are still comparable with uint64_t matlab values.
        }
        else
        {
            if (typeID == OV_TypeId_Stimulation || typeID == OV_TypeId_Boolean || typeID == OV_TypeId_Integer || typeID == OV_TypeId_Float)
            {
                values = values + tmp + " "; // we store the value, these types are readable by matlab directly
            }
            else { values = values + "'" + tmp + "' "; } // we store them as matlab strings using '
        }
    }
    names += "}";
    types += "}";
    values += "}";

    // On Windows, Matlab doesn't sometimes notice .m files have been changed, esp. if you have matlab box running while you change them
    if (!checkFailureRoutine(engEvalString(MATLAB_ENGINE, "clear functions;") == 0, "An error occurred while calling matlab 'clear functions;'\n"))
    {
        return false;
    }

    cmd = m_boxInstanceVariableName + " = OV_setSettings(" + m_boxInstanceVariableName + "," + names + "," + types + "," + values + ");";
    if (!checkFailureRoutine(engEvalString(MATLAB_ENGINE, cmd.toASCIIString()) == 0, "Error calling [OV_setSettings]\n")) { return false; }

    // we set the box clock frequency in the box structure, so it's accessible in the user scripts if needed
    tmp = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
    cmd = m_boxInstanceVariableName + ".clock_frequency = " + tmp + ";";
    if (!checkFailureRoutine(engEvalString(MATLAB_ENGINE, cmd) == 0, "An error occurred while setting the clock frequency\n")) { return false; }

    cmd = m_boxInstanceVariableName + " = " + m_initializeFunc + "(" + m_boxInstanceVariableName + ");";
    if (!checkFailureRoutine(engEvalString(MATLAB_ENGINE, cmd) == 0, "An error occurred while calling the initialize function\n")) { return false; }

    m_helper->setMatlabEngine(MATLAB_ENGINE);
    m_helper->setBoxInstanceVariableName(m_boxInstanceVariableName);

    return true;
}

bool CBoxAlgorithmMatlabScripting::closeMatlabEngineSafely()
{
    if (MATLAB_ENGINE == nullptr) { return true; }
    this->getLogManager() << Kernel::LogLevel_Trace << "Trying to close Matlab engine\n";
#if defined TARGET_OS_Windows
    __try
    {
#endif
        if (MATLAB_ENGINE) { if (engClose(MATLAB_ENGINE) != 0) { this->getLogManager() << Kernel::LogLevel_ImportantWarning << "Could not close Matlab engine.\n"; } }
#if defined TARGET_OS_Windows
    }
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        this->getLogManager() << Kernel::LogLevel_Error << "Closing MATLAB engine failed.\n"
                << "\tTo use this box you must have MATLAB (32 bits version) installed on your computer.\n";
        return false;
    }
#endif

    return true;
}

bool CBoxAlgorithmMatlabScripting::uninitialize()
{
    delete m_helper;
    if (MATLAB_ENGINE != nullptr)
    {
        CString cmd = m_boxInstanceVariableName + " = " + m_uninitializeFunc + "(" + m_boxInstanceVariableName + ");";
        if (!checkFailureRoutine(engEvalString(MATLAB_ENGINE, cmd) == 0, "An error occurred while calling the uninitialize function\n")
        ) { }   // NOP, we still want to deallocate below
        // Remove the box from the matlab workspace
        cmd = "clear " + m_boxInstanceVariableName + ";";
        if (!checkFailureRoutine(engEvalString(MATLAB_ENGINE, cmd) == 0, "An error occurred while clearing the box\n")
        ) { }   // NOP, we still want to deallocate below
    }

    closeMatlabEngineSafely();
    if (m_matlabBuffer)
    {
        delete[] m_matlabBuffer;
        m_matlabBuffer = nullptr;
    }

    for (size_t i = 0; i < m_decoders.size(); ++i)
    {
        m_decoders[i]->uninitialize();
        delete m_decoders[i];
    }
    for (size_t i = 0; i < m_encoders.size(); ++i)
    {
        m_encoders[i]->uninitialize();
        delete m_encoders[i];
    }

    return true;
}

bool CBoxAlgorithmMatlabScripting::processClock(Kernel::CMessageClock& /*msg*/)
{
    if (!MATLAB_ENGINE) { return true; }
    const std::string cmd = std::string(m_boxInstanceVariableName) + ".clock = " + std::to_string(CTime(this->getPlayerContext().getCurrentTime()).toSeconds())
                            + ";";
    OV_ERROR_UNLESS_KRF(engEvalString(MATLAB_ENGINE, cmd.c_str()) == 0, "An error occurred while updating the box clock.", Kernel::ErrorType::BadProcessing);
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

bool CBoxAlgorithmMatlabScripting::process()
{
    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();

    // In openvibe, pointers to managers can change between the calls to process(), so we need to provide current ones every time
    m_helper->setManagers(&this->getLogManager(), &this->getErrorManager());

    for (size_t i = 0; i < getStaticBoxContext().getInputCount(); ++i)
    {
        for (size_t j = 0; j < boxContext.getInputChunkCount(i); ++j)
        {
            m_decoders[i]->decode(j);

            CIdentifier type;
            getStaticBoxContext().getInputType(i, type);
            bool unknownStream     = true;
            bool receivedSomething = false;

            if (m_decoders[i]->isHeaderReceived())
            {
                receivedSomething = true;
                // this->getLogManager() << Kernel::LogLevel_Debug << "Received header\n";

                if (type == OV_TypeId_StreamedMatrix)
                {
                    CMatrix* matrix = static_cast<Toolkit::TStreamedMatrixDecoder<CBoxAlgorithmMatlabScripting>*>(m_decoders[i])->getOutputMatrix();
                    if (!checkFailureRoutine(m_helper->setStreamedMatrixInputHeader(i, matrix), "Error calling [OV_setStreamMatrixInputHeader]\n"))
                    {
                        return false;
                    }

                    m_nInputHeaderSent++;
                    unknownStream = false;
                }

                if (type == OV_TypeId_Signal)
                {
                    CMatrix* matrix          = static_cast<Toolkit::TSignalDecoder<CBoxAlgorithmMatlabScripting>*>(m_decoders[i])->getOutputMatrix();
                    const uint64_t frequency = static_cast<Toolkit::TSignalDecoder<CBoxAlgorithmMatlabScripting>*>(m_decoders[i])->getOutputSamplingRate();
                    if (!checkFailureRoutine(m_helper->setSignalInputHeader(i, matrix, frequency),
                                             "Error calling [OV_setSignalInputHeader]\n")) { return false; }

                    m_nInputHeaderSent++;
                    unknownStream = false;
                }

                if (type == OV_TypeId_FeatureVector)
                {
                    CMatrix* matrix = static_cast<Toolkit::TFeatureVectorDecoder<CBoxAlgorithmMatlabScripting>*>(m_decoders[i])->getOutputMatrix();
                    if (!checkFailureRoutine(m_helper->setFeatureVectorInputHeader(i, matrix),
                                             "Error calling [OV_setFeatureVectorInputHeader]\n")) { return false; }

                    m_nInputHeaderSent++;
                    unknownStream = false;
                }

                if (type == OV_TypeId_Spectrum)
                {
                    CMatrix* matrix          = static_cast<Toolkit::TSpectrumDecoder<CBoxAlgorithmMatlabScripting>*>(m_decoders[i])->getOutputMatrix();
                    CMatrix* freqAbscissa    = static_cast<Toolkit::TSpectrumDecoder<CBoxAlgorithmMatlabScripting>*>(m_decoders[i])->getOutputFrequencyAbscissa();
                    const uint64_t frequency = static_cast<Toolkit::TSpectrumDecoder<CBoxAlgorithmMatlabScripting>*>(m_decoders[i])->getOutputSamplingRate();

                    if (!checkFailureRoutine(m_helper->setSpectrumInputHeader(i, matrix, freqAbscissa, frequency),
                                             "Error calling [OV_setSpectrumInputHeader]\n")) { return false; }

                    m_nInputHeaderSent++;
                    unknownStream = false;
                }

                if (type == OV_TypeId_ChannelLocalisation)
                {
                    CMatrix* matrix    = static_cast<Toolkit::TChannelLocalisationDecoder<CBoxAlgorithmMatlabScripting>*>(m_decoders[i])->getOutputMatrix();
                    const bool dynamic = static_cast<Toolkit::TChannelLocalisationDecoder<CBoxAlgorithmMatlabScripting>*>(m_decoders[i])->getOutputDynamic();
                    if (!checkFailureRoutine(m_helper->setChannelLocalisationInputHeader(i, matrix, dynamic),
                                             "Error calling [OV_setChannelLocalizationInputHeader]\n")) { return false; }

                    m_nInputHeaderSent++;
                    unknownStream = false;
                }

                if (type == OV_TypeId_ExperimentInfo)
                {
                    OV_WARNING_K(
                        "The Experiment Information Stream is not implemented with the Matlab Scripting Box. If this is relevant for your usage, please contact the official development Team.")
                    ;
                    m_nInputHeaderSent++;
                    unknownStream = false;
                }

                if (type == OV_TypeId_Stimulations)
                {
                    if (!checkFailureRoutine(m_helper->setStimulationsInputHeader(i), "Error calling [OV_setStimulationsInputHeader]\n")) { return false; }

                    m_nInputHeaderSent++;
                    unknownStream = false;
                }
            }

            if (m_decoders[i]->isBufferReceived())
            {
                // this->getLogManager() << Kernel::LogLevel_Debug << "Received buffer\n";
                receivedSomething = true;
                // 
                if (type == OV_TypeId_StreamedMatrix || this->getTypeManager().isDerivedFromStream(type, OV_TypeId_StreamedMatrix))
                {
                    CMatrix* matrix      = static_cast<Toolkit::TSignalDecoder<CBoxAlgorithmMatlabScripting>*>(m_decoders[i])->getOutputMatrix();
                    const uint64_t start = boxContext.getInputChunkStartTime(i, j);
                    const uint64_t end   = boxContext.getInputChunkEndTime(i, j);
                    if (!checkFailureRoutine(m_helper->addStreamedMatrixInputBuffer(i, matrix, start, end),
                                             "Error calling [OV_addInputBuffer (Streamed Matrix or child stream)]\n")) { return false; }
                    unknownStream = false;
                }

                if (type == OV_TypeId_Stimulations)
                {
                    CStimulationSet* stimSet = static_cast<Toolkit::TStimulationDecoder<CBoxAlgorithmMatlabScripting>*>(m_decoders[i])->getOutputStimulationSet();
                    const uint64_t start     = boxContext.getInputChunkStartTime(i, j);
                    const uint64_t end       = boxContext.getInputChunkEndTime(i, j);
                    if (stimSet->size() > 0)
                    {
                        this->getLogManager() << Kernel::LogLevel_Trace << "Inserting stimulation set with size " << stimSet->size() << "\n";
                    }
                    if (!checkFailureRoutine(m_helper->addStimulationsInputBuffer(i, stimSet, start, end),
                                             "Error calling [OV_addInputBuffer (Stimulations)]\n")) { return false; }
                    unknownStream = false;
                }
            }

            if (m_decoders[i]->isEndReceived())
            {
                this->getLogManager() << Kernel::LogLevel_Info << "Received end\n";
                receivedSomething = true;
                unknownStream     = false;
                // @FIXME should something additional be done here?
            }

            OV_ERROR_UNLESS_KRF(!receivedSomething || !unknownStream, "Unknown Stream Type " << type.str() << " on input [" << i << "].",
                                Kernel::ErrorType::BadProcessing);
        }
    }

    if (m_nInputHeaderSent < getStaticBoxContext().getInputCount())
    {
        // not ready to process 
        return true;
    }


    // CALL TO PROCESS FUNCTION
    std::string command = std::string(m_boxInstanceVariableName.toASCIIString()) + " = " + m_processFunc.toASCIIString() + "("
                          + m_boxInstanceVariableName.toASCIIString() + ");";
    if (!checkFailureRoutine(engEvalString(MATLAB_ENGINE, command.c_str()) == 0, "Error calling the Process function.\n")) { return false; }

    // Go through every output in the matlab box and copy the data to the C++ side
    for (size_t i = 0; i < getStaticBoxContext().getOutputCount(); ++i)
    {
        // now we check for pending output chunk to be sent (output type independent call)
        command = std::string("OV_PENDING_COUNT_TMP = OV_getNbPendingOutputChunk(") + m_boxInstanceVariableName.toASCIIString() + ", "
                  + std::to_string(i + 1) + ");";
        if (!checkFailureRoutine(engEvalString(MATLAB_ENGINE, command.c_str()) == 0, "Error calling [OV_getNbPendingOutputChunk].\n")) { return false; }

        mxArray* pending      = engGetVariable(MATLAB_ENGINE, "OV_PENDING_COUNT_TMP");
        const double dPending = *mxGetPr(pending);

        CIdentifier typeID;
        getStaticBoxContext().getOutputType(i, typeID);

        for (size_t c = 0; c < size_t(dPending); ++c)
        {
            // If no header were ever sent, we need to extract header information in the matlab box
            // This header must have been set prior to sending the very first buffer. 
            // @FIXME the practice used below of assigning to getters is nasty, it should be refactored to e.g. using getter/setter pairs
            if (!m_oHeaderStates[i])
            {
                bool unknownType = true;
                if (typeID == OV_TypeId_StreamedMatrix)
                {
                    CMatrix* matrixToSend = static_cast<Toolkit::TStreamedMatrixEncoder<CBoxAlgorithmMatlabScripting>*>(m_encoders[i])->getInputMatrix();
                    if (!checkFailureRoutine(m_helper->getStreamedMatrixOutputHeader(i, matrixToSend),
                                             "Error calling [OV_getStreamedMatrixOutputHeader]. Did you correctly set the output header in the matlab structure ?\n")
                    ) { return false; }

                    unknownType = false;
                }
                else if (typeID == OV_TypeId_Signal)
                {
                    CMatrix* matrixToSend = static_cast<Toolkit::TSignalEncoder<CBoxAlgorithmMatlabScripting>*>(m_encoders[i])->getInputMatrix();
                    uint64_t sampling     = static_cast<Toolkit::TSignalEncoder<CBoxAlgorithmMatlabScripting>*>(m_encoders[i])->getInputSamplingRate();
                    if (!checkFailureRoutine(m_helper->getSignalOutputHeader(i, matrixToSend, sampling),
                                             "Error calling [OV_getSignalOutputHeader]. Did you correctly set the output header in the matlab structure ?\n")
                    ) { return false; }

                    // Set the new sampling rate
                    static_cast<Toolkit::TSignalEncoder<CBoxAlgorithmMatlabScripting>*>(m_encoders[i])->getInputSamplingRate() = sampling;

                    unknownType = false;
                }
                else if (typeID == OV_TypeId_FeatureVector)
                {
                    CMatrix* matrixToSend = static_cast<Toolkit::TFeatureVectorEncoder<CBoxAlgorithmMatlabScripting>*>(m_encoders[i])->getInputMatrix();
                    if (!checkFailureRoutine(m_helper->getFeatureVectorOutputHeader(i, matrixToSend),
                                             "Error calling [OV_getFeatureVectorOutputHeader]. Did you correctly set the output header in the matlab structure ?\n")
                    ) { return false; }

                    unknownType = false;
                }
                else if (typeID == OV_TypeId_Spectrum)
                {
                    CMatrix* matrixToSend = static_cast<Toolkit::TSpectrumEncoder<CBoxAlgorithmMatlabScripting>*>(m_encoders[i])->getInputMatrix();
                    CMatrix* freqAbscissa = static_cast<Toolkit::TSpectrumEncoder<CBoxAlgorithmMatlabScripting>*>(m_encoders[i])->getInputFrequencyAbscissa();
                    uint64_t frequency    = static_cast<Toolkit::TSpectrumEncoder<CBoxAlgorithmMatlabScripting>*>(m_encoders[i])->getInputSamplingRate();

                    if (!checkFailureRoutine(m_helper->getSpectrumOutputHeader(i, matrixToSend, freqAbscissa, frequency),
                                             "Error calling [OV_getSpectrumOutputHeader]. Did you correctly set the output header in the matlab structure ?\n")
                    ) { return false; }

                    unknownType = false;
                }
                else if (typeID == OV_TypeId_ChannelLocalisation)
                {
                    CMatrix* matrixToSend = static_cast<Toolkit::TChannelLocalisationEncoder<CBoxAlgorithmMatlabScripting>*>(m_encoders[i])->getInputMatrix();
                    bool dynamic          = static_cast<Toolkit::TChannelLocalisationEncoder<CBoxAlgorithmMatlabScripting>*>(m_encoders[i])->getInputDynamic();
                    if (!checkFailureRoutine(m_helper->getChannelLocalisationOutputHeader(i, matrixToSend, dynamic),
                                             "Error calling [OV_getChannelLocalizationOutputHeader]. Did you correctly set the output header in the matlab structure ?\n")
                    ) { return false; }

                    // Set the new channel localisation
                    static_cast<Toolkit::TChannelLocalisationEncoder<CBoxAlgorithmMatlabScripting>*>(m_encoders[i])->getInputDynamic() = dynamic;

                    unknownType = false;
                }
                else if (typeID == OV_TypeId_ExperimentInfo)
                {
                    OV_WARNING_K(
                        "The Experiment Information Stream is not implemented with the Matlab Scripting Box. If this is relevant for your usage, please contact the official development Team.")
                    ;
                    unknownType = false;
                }
                else if (typeID == OV_TypeId_Stimulations)
                {
                    CStimulationSet* stimSet = static_cast<Toolkit::TStimulationEncoder<CBoxAlgorithmMatlabScripting>*>(m_encoders[i])->getInputStimulationSet();
                    if (!checkFailureRoutine(m_helper->getStimulationsOutputHeader(i, stimSet),
                                             "Error calling [OV_getStimulationsOutputHeader]. Did you correctly set the output header in the matlab structure ?\n")
                    ) { return false; }

                    unknownType = false;
                }

                OV_ERROR_UNLESS_KRF(!unknownType, "Unknown Stream Type on output [" << i << "].", Kernel::ErrorType::BadProcessing);

                m_encoders[i]->encodeHeader();
                boxContext.markOutputAsReadyToSend(i, 0, 0);

                m_oHeaderStates[i] = true;
            }


            bool unknownType = true;
            uint64_t start   = 0;
            uint64_t end     = 0;

            if (typeID == OV_TypeId_StreamedMatrix || this->getTypeManager().isDerivedFromStream(typeID, OV_TypeId_StreamedMatrix))
            {
                CMatrix* matrixToSend = static_cast<Toolkit::TStreamedMatrixEncoder<CBoxAlgorithmMatlabScripting>*>(m_encoders[i])->getInputMatrix();
                if (!checkFailureRoutine(m_helper->popStreamedMatrixOutputBuffer(i, matrixToSend, start, end),
                                         "Error calling [OV_popOutputBufferReshape] for Streamed Matrix stream or child stream.\n")) { return false; }

                unknownType = false;
            }

            if (typeID == OV_TypeId_Stimulations)
            {
                CStimulationSet* stimSet = static_cast<Toolkit::TStimulationEncoder<CBoxAlgorithmMatlabScripting>*>(m_encoders[i])->getInputStimulationSet();
                stimSet->clear();
                if (!checkFailureRoutine(m_helper->popStimulationsOutputBuffer(i, stimSet, start, end),
                                         "Error calling [OV_popOutputBuffer] for Stimulation stream.\n")) { return false; }
                unknownType = false;
            }

            OV_ERROR_UNLESS_KRF(!unknownType, "Unknown Stream Type on output [" << i << "].", Kernel::ErrorType::BadProcessing);

            m_encoders[i]->encodeBuffer();
            boxContext.markOutputAsReadyToSend(i, start, end);
        }
        mxDestroyArray(pending);
    }

    return true;
}

bool CBoxAlgorithmMatlabScripting::processInput(const size_t /*index*/)
{
    OV_ERROR_UNLESS_KRF(getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess(),
                        "Error while marking algorithm as ready to process", Kernel::ErrorType::Internal);
    return true;
}

bool CBoxAlgorithmMatlabScripting::printOutputBufferWithFormat()
{
    // the buffer for the console

    // @FIXME allow seeing more of the error message!

    std::stringstream ss;
    ss << m_matlabBuffer;
    if (!ss.str().empty())
    {
        const size_t errIdx1 = ss.str().find("??? ");
        const size_t errIdx2 = ss.str().find("Error: ");
        const size_t errIdx3 = ss.str().find("Error ");

        // Find the earliest error message
        size_t errIdx = std::string::npos;
        if (errIdx == std::string::npos || (errIdx1 != std::string::npos && errIdx1 < errIdx)) { errIdx = errIdx1; }
        if (errIdx == std::string::npos || (errIdx2 != std::string::npos && errIdx2 < errIdx)) { errIdx = errIdx2; }
        if (errIdx == std::string::npos || (errIdx3 != std::string::npos && errIdx3 < errIdx)) { errIdx = errIdx3; }

        size_t warnIdx = ss.str().find("Warning: ");
        if (errIdx == std::string::npos && warnIdx == std::string::npos)
        {
            this->getLogManager() << Kernel::LogLevel_Info << "\n---- MATLAB BUFFER - INFO ----\n" << ss.str().substr(0, (warnIdx < errIdx) ? warnIdx : errIdx) << "\n";
        }
        if (warnIdx != std::string::npos)
        {
            this->getLogManager() << Kernel::LogLevel_Warning << "\n---- MATLAB BUFFER - WARNING ----\n" << ss.str().substr(warnIdx, errIdx) << "\n";
        }
        if (errIdx != std::string::npos)
        {
            this->getLogManager() << Kernel::LogLevel_Error << "\n---- MATLAB BUFFER - ERROR ! ----\n" << ss.str().substr(errIdx) << "\n";
            m_errorDetected = true;
        }
    }
    return true;
}

}  // namespace Matlab
}  // namespace Plugins
}  // namespace OpenViBE
#endif // TARGET_HAS_ThirdPartyMatlab
