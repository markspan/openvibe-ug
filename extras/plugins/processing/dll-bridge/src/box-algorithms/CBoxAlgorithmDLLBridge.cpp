///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmDLLBridge.cpp
/// \author Jussi T. Lindgren (Inria)
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

#include "CBoxAlgorithmDLLBridge.hpp"

#include <iostream>

#if defined(TARGET_OS_Windows)
#include <windows.h>
#elif defined(TARGET_OS_Linux)
#include <dlfcn.h>
#endif


namespace OpenViBE {
namespace Plugins {
namespace DLLBridge {

// #define DLLBRIDGE_DEBUG
#if defined(DLLBRIDGE_DEBUG)
static void dumpMatrix(Kernel::ILogManager& mgr, const CMatrix& mat, const CString& desc)
{
    mgr << Kernel::LogLevel_Info << desc << "\n";
    for (size_t i = 0; i < mat.getDimensionSize(0); i++)
    {
        mgr << Kernel::LogLevel_Info << "Row " << i << ": ";
        for (size_t j = 0; j < mat.getDimensionSize(1); j++) { mgr << mat.getBuffer()[i * mat.getDimensionSize(1) + j] << " "; }
        mgr << "\n";
    }
}
#endif

#if defined(TARGET_OS_Windows)
#define OPEN_FUNC(file, library) \
std::string tmp = file.toASCIIString();\
std::replace(tmp.begin(), tmp.end(), '/', '\\');\
file = tmp.c_str();\
library = LoadLibrary(file.toASCIIString());

#define PROC_FUNC GetProcAddress
#define ERR_FUNC GetLastError
#define CLOSE_FUNC FreeLibrary
#else
#define OPEN_FUNC(file, library) library = dlopen(file.toASCIIString(), RTLD_LAZY);
#define PROC_FUNC dlsym
#define ERR_FUNC dlerror
#define CLOSE_FUNC dlclose
#endif

// This function is called once when user presses play in Designer
bool CDLLBridge::initialize()
{
    this->getLogManager() << Kernel::LogLevel_Debug << "Initializing\n";

    m_encoder       = nullptr;
    m_decoder       = nullptr;
    m_library       = nullptr;
    m_initialize    = nullptr;
    m_processHeader = nullptr;
    m_process       = nullptr;
    m_uninitialize  = nullptr;

    m_dllFile    = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);
    m_parameters = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);

    if (m_dllFile == CString("")) {
        this->getLogManager() << Kernel::LogLevel_Error << "Need a DLL file\n";
        return false;
    }

    OPEN_FUNC(m_dllFile, m_library)
    if (!m_library) {
        this->getLogManager() << Kernel::LogLevel_Error << "Failed to load " << m_dllFile << ", error code " << int64_t(ERR_FUNC()) << "\n";
        return false;
    }

    m_initialize = INITFUNC(PROC_FUNC(m_library, "box_init"));
    if (!m_initialize) {
        this->getLogManager() << Kernel::LogLevel_Error << "Unable to find box_init() in the DLL\n";
        return false;
    }

    m_process = PROCESSFUNC(PROC_FUNC(m_library, "box_process"));
    if (!m_process) {
        this->getLogManager() << Kernel::LogLevel_Error << "Unable to find box_process() in the DLL\n";
        return false;
    }

    m_processHeader = PROCESSHEADERFUNC(PROC_FUNC(m_library, "box_process_header"));
    if (!m_processHeader) {
        this->getLogManager() << Kernel::LogLevel_Error << "Unable to find box_process_header() in the DLL\n";
        return false;
    }

    m_uninitialize = UNINITFUNC(PROC_FUNC(m_library, "box_uninit"));
    if (!m_uninitialize) {
        this->getLogManager() << Kernel::LogLevel_Error << "Unable to find box_uninit() in the DLL\n";
        return false;
    }

    const Kernel::IBox& boxContext = this->getStaticBoxContext();
    boxContext.getInputType(0, m_inputTypeID);
    if (m_inputTypeID == OV_TypeId_StreamedMatrix) {
        Toolkit::TStreamedMatrixDecoder<CDLLBridge>* decoder = new Toolkit::TStreamedMatrixDecoder<CDLLBridge>;
        Toolkit::TStreamedMatrixEncoder<CDLLBridge>* encoder = new Toolkit::TStreamedMatrixEncoder<CDLLBridge>;

        decoder->initialize(*this, 0);
        encoder->initialize(*this, 0);

        m_decoder = decoder;
        m_encoder = encoder;
    }
    else if (m_inputTypeID == OV_TypeId_Signal) {
        Toolkit::TSignalDecoder<CDLLBridge>* decoder = new Toolkit::TSignalDecoder<CDLLBridge>;
        Toolkit::TSignalEncoder<CDLLBridge>* encoder = new Toolkit::TSignalEncoder<CDLLBridge>;

        decoder->initialize(*this, 0);
        encoder->initialize(*this, 0);

        m_decoder = decoder;
        m_encoder = encoder;
    }
    else {
        this->getLogManager() << Kernel::LogLevel_Error << "Unknown input type " << m_inputTypeID << ". This should never happen.\n";
        return false;
    }

    this->getLogManager() << Kernel::LogLevel_Trace << "DLL box_init() : Calling\n";

    // Do some initialization in DLL
    int length = int(m_parameters.length());
    int code   = 0;
    m_initialize(&length, m_parameters.toASCIIString(), &code);
    if (code) {
        this->getLogManager() << Kernel::LogLevel_Error << "DLL box_init() : Returned error code " << code << "\n";
        return false;
    }
    this->getLogManager() << Kernel::LogLevel_Trace << "DLL box_init() : Return ok\n";
    return true;
}

// This function is called once when user presses Stop in Designer
bool CDLLBridge::uninitialize()
{
    this->getLogManager() << Kernel::LogLevel_Debug << "Uninitializing\n";

    if (m_uninitialize) {
        this->getLogManager() << Kernel::LogLevel_Trace << "DLL box_uninit() : Calling\n";

        // Do some uninitialization in DLL
        int code = 0;
        m_uninitialize(&code);

        if (code) {
            this->getLogManager() << Kernel::LogLevel_Error << "DLL box_uninit() : Returned error code " << code << "\n";
            return false;
        }
        this->getLogManager() << Kernel::LogLevel_Trace << "DLL box_uninit() : Return ok\n";
    }

    if (m_encoder) {
        m_encoder->uninitialize();
        delete m_encoder;
        m_encoder = nullptr;
    }
    if (m_decoder) {
        m_decoder->uninitialize();
        delete m_decoder;
        m_decoder = nullptr;
    }

    if (m_library) {
        CLOSE_FUNC(m_library);
        m_library = nullptr;
    }

    return true;
}

bool CDLLBridge::processInput(const size_t /*index*/)
{
    getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

// This function is called for every signal chunk (matrix of N samples [channels x samples]).
bool CDLLBridge::process()
{
    this->getLogManager() << Kernel::LogLevel_Debug << "Process chunk\n";

    Kernel::IBoxIO& boxContext = this->getDynamicBoxContext();

    for (uint32_t i = 0; i < boxContext.getInputChunkCount(0); ++i) {
        // m_decoder/m_encoder should point to StreamedMatrix*coder or Signal*Coder by construction,
        // the latter appears to be static castable to the former, in practice. 
        // n.b. dynamic cast does not work here runtime (this does, for the moment). I do not hazard 
        // an explanation here. Anybody wanting to find out the reasons be prepared to digest the 
        // codec template inheritance relationships in toolkit for a while.
        Toolkit::TStreamedMatrixDecoder<CDLLBridge>* decoder = dynamic_cast<Toolkit::TStreamedMatrixDecoder<CDLLBridge>*>(m_decoder);
        Toolkit::TStreamedMatrixEncoder<CDLLBridge>* encoder = dynamic_cast<Toolkit::TStreamedMatrixEncoder<CDLLBridge>*>(m_encoder);

        decoder->decode(i);

        if (decoder->isHeaderReceived()) {
            if (decoder->getOutputMatrix()->getDimensionCount() != 2) {
                this->getLogManager() << Kernel::LogLevel_Error << "Only 2 dimensional matrices supported\n";
                return false;
            }

            int iSampling = 0;

            if (m_inputTypeID == OV_TypeId_Signal) {
                Toolkit::TSignalDecoder<CDLLBridge>* signalDecoder = dynamic_cast<Toolkit::TSignalDecoder<CDLLBridge>*>(m_decoder);
                iSampling                                          = int(signalDecoder->getOutputSamplingRate());
            }

            int iNRows = decoder->getOutputMatrix()->getDimensionSize(0);
            int iNCols = decoder->getOutputMatrix()->getDimensionSize(1);

            this->getLogManager() << Kernel::LogLevel_Trace << "DLL box_process_header() : Calling\n";

            int code = 0, oNRows = 0, oNCols = 0, oSampling = 0;
            m_processHeader(&iNRows, &iNCols, &iSampling, &oNRows, &oNCols, &oSampling, &code);
            if (code) {
                this->getLogManager() << Kernel::LogLevel_Error << "DLL box_process_header() : Returned error code " << code << "\n";
                return false;
            }
            this->getLogManager() << Kernel::LogLevel_Trace << "DLL box_process_header() : Return ok\n";
            this->getLogManager() << Kernel::LogLevel_Trace << "The function declared" << " rowsOut=" << oNRows << " colsOut=" << oNCols << " sRateOut=" << oSampling << "\n";

            encoder->getInputMatrix()->resize(oNRows, oNCols);

            if (m_inputTypeID == OV_TypeId_Signal) {
                Toolkit::TSignalEncoder<CDLLBridge>* signalEncoder = dynamic_cast<Toolkit::TSignalEncoder<CDLLBridge>*>(m_encoder);

                signalEncoder->getInputSamplingRate() = oSampling;
            }

            encoder->encodeHeader();

            boxContext.markOutputAsReadyToSend(0, boxContext.getInputChunkStartTime(0, i), boxContext.getInputChunkEndTime(0, i));
        }

        if (decoder->isBufferReceived()) {
            double* input  = decoder->getOutputMatrix()->getBuffer();
            double* output = encoder->getInputMatrix()->getBuffer();

            this->getLogManager() << Kernel::LogLevel_Trace << "DLL box_process() : Calling\n";

            // Process the sample chunk in DLL
            int errorCode = 0;
            m_process(input, output, &errorCode);
            if (errorCode) {
                this->getLogManager() << Kernel::LogLevel_Error << "DLL box_process() : Returned error code " << errorCode << "\n";
                return false;
            }
            this->getLogManager() << Kernel::LogLevel_Trace << "DLL box_process() : Return ok\n";

            encoder->encodeBuffer();
            boxContext.markOutputAsReadyToSend(0, boxContext.getInputChunkStartTime(0, i), boxContext.getInputChunkEndTime(0, i));
        }
        if (decoder->isEndReceived()) {
            encoder->encodeEnd();
            boxContext.markOutputAsReadyToSend(0, boxContext.getInputChunkStartTime(0, i), boxContext.getInputChunkEndTime(0, i));
        }
    }

    return true;
}

}  // namespace DLLBridge
}  // namespace Plugins
}  // namespace OpenViBE
