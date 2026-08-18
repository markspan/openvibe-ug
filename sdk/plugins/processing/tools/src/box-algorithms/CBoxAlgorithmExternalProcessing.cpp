///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmExternalProcessing.cpp
/// \brief Classes implementation for the Box External Processing.
/// \author Alexis Placet (Mensia Technologies SA).
/// \version 1.0.
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

#include "CBoxAlgorithmExternalProcessing.hpp"

#include <chrono>
#include <cstdlib>
#include <stdlib.h>
#include <random>
#include <thread>
#include <algorithm>

#ifdef TARGET_OS_Windows
#include <process.h>
#include <Windows.h>
#include <ctime>
#else
#include <spawn.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <unistd.h>
extern char **environ;
#endif

#include <system/ovCTime.h>

namespace OpenViBE {
namespace Plugins {
namespace Tools {

static Kernel::ELogLevel convertLogLevel(const Communication::ELogLevel level)
{
    switch (level) {
        case Communication::LogLevel_Info: return Kernel::LogLevel_Info;
        case Communication::LogLevel_Warning: return Kernel::LogLevel_Warning;
        case Communication::LogLevel_Error: return Kernel::LogLevel_Error;
        case Communication::LogLevel_Fatal: return Kernel::LogLevel_Fatal;
        case Communication::LogLevel_Max:
        case Communication::LogLevel_Unknown:
        default: return Kernel::LogLevel_Info;
    }
}

uint64_t CBoxAlgorithmExternalProcessing::getClockFrequency()
{
    if (m_isGenerator) {
        // We slow down the generator type boxes by default, in order to limit syncing
        // In fast forward we limit the syncing even more by setting the frequency to 1Hz
        if (this->getPlayerContext().getStatus() == Kernel::EPlayerStatus::Forward) { return 1LL << 32; }
        return 16LL << 32;
    }
    return 128LL << 32;
}

bool CBoxAlgorithmExternalProcessing::initialize()
{
    m_port = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 3);

    // Check that the port is not in the system range
    OV_ERROR_UNLESS_KRF((m_port >= 49152 && m_port <= 65535) || (m_port == 0),
                        "Port [" << m_port << "] is invalid. It must be either 0 or a number in the range 49152-65535.", Kernel::ErrorType::BadConfig);

    // Settings
    const Kernel::IBox& staticboxCtx = this->getStaticBoxContext();

    for (size_t i = 8; i < staticboxCtx.getSettingCount(); ++i) {
        CString name;
        staticboxCtx.getSettingName(i, name);

        CIdentifier type;
        staticboxCtx.getSettingType(i, type);

        const CString value = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), i);

        OV_FATAL_UNLESS_K(m_messaging.addParameter(i, type.id(), name.toASCIIString(), value.toASCIIString()),
                          "Failed to add a parameter: " << i, Kernel::ErrorType::Internal);
    }

    // Inputs
    for (size_t i = 0; i < staticboxCtx.getInputCount(); ++i) {
        CIdentifier type;
        staticboxCtx.getInputType(i, type);

        if (type == OV_TypeId_Stimulations) { m_decoders[i].initialize(*this, i); }

        CString name;
        staticboxCtx.getInputName(i, name);

        OV_FATAL_UNLESS_K(m_messaging.addInput(i, type.id(), name.toASCIIString()), "Failed to add an input: " << i, Kernel::ErrorType::Internal);
    }

    // Outputs
    for (size_t i = 0; i < staticboxCtx.getOutputCount(); ++i) {
        CIdentifier type;
        staticboxCtx.getOutputType(i, type);

        CString name;
        staticboxCtx.getOutputName(i, name);

        if (!m_messaging.addOutput(i, type.id(), name.toASCIIString())) {
            this->getLogManager() << Kernel::LogLevel_Error << "Failed to add an output: " << i << "\n";
            return false;
        }
    }

    const bool mustGenerateConnectionID = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 4);

    if (mustGenerateConnectionID) { m_connectionID = generateConnectionID(32); }
    else {
        const CString connectionID = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 5);
        m_connectionID             = connectionID.toASCIIString();
    }

    OV_ERROR_UNLESS_KRF(m_messaging.listen(m_port),
                        "Could not listen to TCP port: " << m_port << ". This port may be already used by another service. Please try another one.",
                        Kernel::ErrorType::BadNetworkConnection);

    if (m_port == 0) {
        m_messaging.getSocketPort(m_port);
        this->getLogManager() << Kernel::LogLevel_Info << "Box is now listening on TCP port [" << m_port << "].\n";
    }

    m_shouldLaunchProgram = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 0);

    if (m_shouldLaunchProgram) {
        const CString programPath = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 1);
        const CString arguments   = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 2);

        if (!launchThirdPartyProgram(programPath.toASCIIString(), arguments.toASCIIString())) {
            // Error message in the function
            return false;
        }

        this->getLogManager() << Kernel::LogLevel_Info << "Third party program [" << programPath.toASCIIString() << "] started.\n";
    }

    const auto startTime = System::Time::zgetTime();

    bool clientConnected    = false;
    m_hasReceivedEndMessage = false;

    m_acceptTimeout = CTime(double(FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 6))).time();
    m_isGenerator   = FSettingValueAutoCast(*this->getBoxAlgorithmContext(), 7);

    while (System::Time::zgetTime() - startTime < m_acceptTimeout) {
        if (m_messaging.accept()) {
            clientConnected = true;
            this->getLogManager() << Kernel::LogLevel_Info << "Client connected to the server.\n";
            break;
        }

        const Communication::MessagingServer::ELibraryError error = m_messaging.getLastError();
        if (error == Communication::MessagingServer::ELibraryError::BadAuthenticationReceived) {
            OV_WARNING_K("A client sent a bad authentication.");
            break;
        }
        if (error == Communication::MessagingServer::ELibraryError::NoAuthenticationReceived) { OV_WARNING_K("The client has not sent authentication."); }
    }

    OV_ERROR_UNLESS_KRF(clientConnected, "No client connected before the timeout.", Kernel::ErrorType::Internal);


    // Now synchronize once with the client so it can perform its initialize before
    // the processing starts
    m_messaging.pushSync();


    while (m_messaging.isConnected() && !m_messaging.waitForSyncMessage()) {
        if (!m_messaging.waitForSyncMessage()) { std::this_thread::sleep_for(std::chrono::milliseconds(1)); }
    }

    m_syncTimeout  = CTime(0.0625).time();
    m_lastSyncTime = System::Time::zgetTime();
    return true;
}

bool CBoxAlgorithmExternalProcessing::uninitialize()
{
    for (auto& decoder : m_decoders) { decoder.second.uninitialize(); }

    if (!m_hasReceivedEndMessage) {
        const bool result = m_messaging.close();

#ifdef TARGET_OS_Windows
        if (m_shouldLaunchProgram && m_extProcessId > 0) {
            DWORD exitCode;

            // Wait for external process to stop by himself, terminate it if necessary
            const auto startTime = System::Time::zgetTime();
            while (System::Time::zgetTime() - startTime < m_acceptTimeout) {
                GetExitCodeProcess(HANDLE(m_extProcessId), &exitCode);

                if (exitCode != STILL_ACTIVE) { break; }
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }


            if (exitCode == STILL_ACTIVE) {
                OV_ERROR_UNLESS_KRF(TerminateProcess(HANDLE(m_extProcessId), EXIT_FAILURE), "Failed to kill third party program.", Kernel::ErrorType::Unknown);
            }
            else if (exitCode != 0) { OV_WARNING_K("Third party program [" << m_extProcessId << "] has terminated with exit code [" << int(exitCode) << "]"); }
        }
#else
        if (m_shouldLaunchProgram && m_extProcessId != 0)
        {
            int status;
            pid_t pid = waitpid(m_extProcessId, &status, WNOHANG);
            
            // Wait for external process to stop by himself, terminate it after 10s
            auto startTime = System::Time::zgetTime();
            while (pid == 0 && System::Time::zgetTime() - startTime < m_acceptTimeout)
            {
                // Check if the program has hung itself
                pid = waitpid(m_extProcessId, &status, WNOHANG);
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }

            if (pid != 0)
            {
                if (WIFEXITED(status)) { OV_WARNING_K("Third party program [" << m_extProcessId << "] has terminated with exit code [" << WEXITSTATUS(status) << "]"); }
                else if (WIFSIGNALED(status)) { OV_WARNING_K("Third party program [" << m_extProcessId << "] killed by signal [" << WTERMSIG(status) << "]"); }
            }
            else
            {
                kill(m_extProcessId, SIGTERM);
                waitpid(m_extProcessId, &status, 0);
                this->getLogManager() << Kernel::LogLevel_Info << "Third party program [" << m_extProcessId << "] exited with status [" << WEXITSTATUS(status) << "]\n";
            }
        }
#endif
        return result;
    }

    return true;
}

bool CBoxAlgorithmExternalProcessing::processClock(Kernel::CMessageClock& /*msg*/) { return this->getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess(); }

bool CBoxAlgorithmExternalProcessing::processInput(const size_t /*index*/)
{
    this->getBoxAlgorithmContext()->markAlgorithmAsReadyToProcess();
    return true;
}

bool CBoxAlgorithmExternalProcessing::process()
{
    if (m_messaging.isInErrorState()) {
        const std::string errorString = Communication::MessagingServer::getErrorString(m_messaging.getLastError());
        OV_ERROR_KRF("Error state connection: " << errorString << ".\n This may be due to a broken client connection.",
                     Kernel::ErrorType::BadNetworkConnection);
    }

    if (m_hasReceivedEndMessage == false && m_messaging.isEndReceived() == true) {
        this->getLogManager() << Kernel::LogLevel_Info << "The third party program has ended the communication.\n";
        m_messaging.close();
        m_hasReceivedEndMessage = true;
    }

    OV_ERROR_UNLESS_KRF(m_messaging.pushTime(this->getPlayerContext().getCurrentTime()), "Failed to push Time.", Kernel::ErrorType::BadNetworkConnection);

    const Kernel::IBox& staticboxCtx = this->getStaticBoxContext();
    Kernel::IBoxIO& boxCtx           = this->getDynamicBoxContext();

    // Send input EBML to the client

    bool hasSentDataToClient = false;

    for (size_t i = 0; i < staticboxCtx.getInputCount(); ++i) {
        auto maybeStimulationDecoder = m_decoders.find(i);
        for (size_t j = 0; j < boxCtx.getInputChunkCount(i); ++j) {
            if (!m_hasReceivedEndMessage) {
                uint64_t startTime         = 0;
                uint64_t endTime           = 0;
                size_t chunkSize           = 0;
                const uint8_t* chunkBuffer = nullptr;

                OV_FATAL_UNLESS_K(boxCtx.getInputChunk(i, j, startTime, endTime, chunkSize, chunkBuffer),
                                  "Failed to get input chunk [" << i << "][" << j << "]", Kernel::ErrorType::Internal);

                std::shared_ptr<std::vector<uint8_t>> ebml(new std::vector<uint8_t>(chunkBuffer, chunkBuffer + chunkSize));

                // We only encode stimulation stream chunks if they contain stimulations
                if (maybeStimulationDecoder != m_decoders.end()) {
                    maybeStimulationDecoder->second.decode(j, false); // The input will be marked as deprecated later

                    // Cache empty chunks, we will send them when a stimulation or a signal chunk arrives
                    if (maybeStimulationDecoder->second.getOutputStimulationSet()->size() == 0) {
                        m_packetHistory.emplace(startTime, endTime, i, ebml);
                        OV_FATAL_UNLESS_K(boxCtx.markInputAsDeprecated(i, j), "Failed to mark input as deprecated", Kernel::ErrorType::Internal);
                        break;
                    }
                }

                // Empty the history before to send useful data
                while (!m_packetHistory.empty()) {
                    OV_ERROR_UNLESS_KRF(m_messaging.pushEBML(m_packetHistory.front().index, m_packetHistory.front().startTime, m_packetHistory.front().endTime,
                                            m_packetHistory.front().EBML), "Failed to push EBML.", Kernel::ErrorType::BadNetworkConnection);

                    m_packetHistory.pop();
                }

                // Push the last EBML
                OV_ERROR_UNLESS_KRF(m_messaging.pushEBML(i, startTime, endTime, ebml), "Failed to push EBML.", Kernel::ErrorType::BadNetworkConnection);
                hasSentDataToClient = true;
            }

            OV_FATAL_UNLESS_K(boxCtx.markInputAsDeprecated(i, j), "Failed to mark input as deprecated", Kernel::ErrorType::Internal);
        }
    }

    if (hasSentDataToClient || m_isGenerator || System::Time::zgetTime() - m_lastSyncTime > m_syncTimeout) {
        m_lastSyncTime = System::Time::zgetTime();
        // Here, we send a sync message to tell to the client that we have no more data to send.
        // Generators do not have input data, so the box never has to send data to the external program
        // and thus needs to perform syncing on each tick.

        OV_ERROR_UNLESS_KRF(m_messaging.pushSync(), "Failed to push sync message.", Kernel::ErrorType::BadNetworkConnection);

        if (!m_hasReceivedEndMessage) {
            this->log();

            uint64_t packetId;
            size_t index;
            uint64_t startTime;
            uint64_t endTime;
            std::shared_ptr<const std::vector<uint8_t>> ebml;

            bool receivedSync = false;
            while (!receivedSync && !m_hasReceivedEndMessage && !m_messaging.isInErrorState() && m_messaging.isConnected()) {
                receivedSync = m_messaging.waitForSyncMessage();
                while (m_messaging.popEBML(packetId, index, startTime, endTime, ebml)) {
                    OV_ERROR_UNLESS_KRF(boxCtx.appendOutputChunkData(index, ebml->data(), ebml->size()),
                                        "Failed to append output chunk data.", Kernel::ErrorType::Internal);

                    OV_ERROR_UNLESS_KRF(boxCtx.markOutputAsReadyToSend(index, startTime, endTime),
                                        "Failed to mark output as ready to send.", Kernel::ErrorType::Internal);
                }
                if (!receivedSync) { std::this_thread::sleep_for(std::chrono::milliseconds(1)); }
            }
        }
    }

    return true;
}

std::string CBoxAlgorithmExternalProcessing::generateConnectionID(const size_t size)
{
    std::default_random_engine generator{ std::random_device()() };
    std::uniform_int_distribution<int> uni(0, 34);

    std::string connectionID;

    for (size_t i = 0; i < size; ++i) {
        const char c = char(uni(generator));
        connectionID.push_back((c < 26) ? ('A' + c) : '1' + (c - 26));
    }

    return connectionID;
}

static std::vector<std::string> splitCommandLine(const std::string& cmdLine)
{
    std::vector<std::string> list;
    std::string arg;
    bool escape                         = false;
    enum { Idle, Arg, QuotedArg } state = Idle;

    for (const char c : cmdLine) {
        if (!escape && c == '\\') {
            escape = true;
            continue;
        }

        switch (state) {
            case Idle:
                if (!escape && c == '"') {
                    state = QuotedArg;
#if defined TARGET_OS_Windows
                    arg += c;
#endif
                }
                else if (escape || c != ' ') {
                    arg += c;
                    state = Arg;
                }
                break;

            case Arg:
                if (!escape && c == '"') {
                    state = QuotedArg;
#if defined TARGET_OS_Windows
                    arg += c;
#endif
                }
                else if (escape || c != ' ') { arg += c; }
                else {
                    list.push_back(arg);
                    arg.clear();
                    state = Idle;
                }
                break;

            case QuotedArg:
                if (!escape && c == '"') {
                    state = arg.empty() ? Idle : Arg;
#if defined TARGET_OS_Windows
                    arg += c;
#endif
                }
                else { arg += c; }
                break;
        }

        escape = false;
    }

    if (!arg.empty()) { list.push_back(arg); }

    return list;
}

char* strToChar(const std::string& s)
{
    char* c = new char[s.size() + 1];
    std::copy(s.begin(), s.end(), c);
    return c;
}

bool CBoxAlgorithmExternalProcessing::launchThirdPartyProgram(const std::string& programPath, const std::string& arguments)
{
    m_extProcessId = 0;

    const std::vector<std::string> argumentsVector = splitCommandLine(arguments);

    std::vector<std::string> programArguments = { programPath, "--connection-id", m_connectionID, "--port", std::to_string(m_port) };
    programArguments.insert(programArguments.begin() + 1, argumentsVector.cbegin(), argumentsVector.cend()); // Add the arguments after the program path.
    std::vector<char*> argv;

    std::transform(programArguments.begin(), programArguments.end(), std::back_inserter(argv), strToChar);

    argv.push_back(nullptr);

#ifdef TARGET_OS_Windows
    // _spawnp on Windows has a special case for empty program
    int status;
    if (programPath.empty()) {
        status = -1;
        _set_errno(ENOENT);
    }
    else { status = int(_spawnvp(_P_NOWAIT, programPath.c_str(), argv.data())); }
    m_extProcessId = status;
    // _P_DETACH,
#else
    posix_spawn_file_actions_t fileAction;
    int res = posix_spawn_file_actions_init(&fileAction);
    OV_ERROR_UNLESS_KRF(res == 0, "File action could not be initialized. Got error [" << res << "]", Kernel::ErrorType::BadCall);
    res = posix_spawn_file_actions_addclose(&fileAction, STDOUT_FILENO);
    OV_ERROR_UNLESS_KRF(res == 0, "File action 'close' could not be added. Got error [" << res << "]", Kernel::ErrorType::BadCall);

    this->getLogManager() << Kernel::LogLevel_Info << "Run third party program [" << programPath << "] with arguments [" << arguments << "].\n";
    int status = posix_spawnp(&m_extProcessId, programPath.c_str(), &fileAction, nullptr, argv.data(), environ);

#if defined TARGET_OS_Linux  || defined TARGET_OS_MacOS
    // On linux the glibc is bugged and posix_spawnp does not actually work as specified
    // we have to check if the program did not exit immediately with a 127 error code
    // for this we need to wait a bit because it could be that the process has not yet launched

    // wait until the process exists or if it has failed

    bool processExists = false;
    bool processHasFailed = false;
    while (!processExists && !processHasFailed)
    {
        System::Time::sleep(10);
        processExists = (kill(m_extProcessId, 0) == 0);

        int childStatus;
        pid_t pid = waitpid(m_extProcessId, &childStatus, WNOHANG);

        if (pid != 0)
        {
            // The process is dead
            if (WIFEXITED(childStatus))
            {
                // If the exit status is 0 this means that maybe we will actually succeed in launching the program
                if (WEXITSTATUS(childStatus) != 0)
                {
                    m_extProcessId = 0;
                    status = WEXITSTATUS(childStatus);
                }
            }
            else
            {
                OV_WARNING_K("The third party process died");
                m_extProcessId = 0;
                status = 1;
            }
            processHasFailed = true;
        }
    }

#endif

#endif

    for (size_t i = 0; i < argv.size() - 1; ++i) { delete[] argv[i]; }

#if defined TARGET_OS_Linux || defined TARGET_OS_MacOS
    if (status != 0)
    {
        const std::string errorMessage = "Failed to launch the program [" + programPath + "]: ";
        m_extProcessId = 0;
        OV_ERROR_KRF( errorMessage.c_str() << "[" << status << "]", Kernel::ErrorType::BadResourceCreation);
    }
#else
    if (status == -1) {
        std::string errorMessage = "Failed to launch the program [" + programPath + "]:";

        switch (errno) {
            case E2BIG:
                errorMessage += "Argument list exceeds 1024 bytes.\n";
                break;

            case EINVAL:
                errorMessage += "Mode argument is invalid.\n";
                break;

            case ENOENT:
                errorMessage += "File or path is not found.\n";
                break;

            case ENOEXEC:
                errorMessage += "Specified file is not executable or has invalid executable-file format.\n";
                break;

            case ENOMEM:
                errorMessage += "Not enough memory is available to execute the new process.\n";
                break;

            default:
                errorMessage += "Unknown error.\n";
                break;
        }

        OV_ERROR_KRF(errorMessage.c_str(), Kernel::ErrorType::BadResourceCreation);
    }
#endif

    return true;
}

void CBoxAlgorithmExternalProcessing::log()
{
    Communication::ELogLevel logLevel;
    std::string logMessage;
    uint64_t packetId;

    while (m_messaging.popLog(packetId, logLevel, logMessage)) {
        this->getLogManager() << convertLogLevel(logLevel) << "From third party program: " << logMessage << "\n";
    }
}

}  // namespace Tools
}  // namespace Plugins
}  // namespace OpenViBE
