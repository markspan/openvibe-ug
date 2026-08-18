#include "Processor.h"

#include <iostream>
#include <fstream>

#include <chrono>
#include <ctime>

#include <fs/Files.h>
#include <system/ovCMath.h>
#include <thread>

namespace OpenViBE {
namespace Tracker {

void playerLaunch(const char* xmlFile, const char* args, bool playFast, bool noGUI, uint32_t identifier)
{
    std::string designer = std::string(Directories::getBinDir().toASCIIString()) + "/designer.exe";
    std::string argsAll  = std::string(" --no-session-management ") + (args ? args : "") + (noGUI ? "--no-gui " : "") + (playFast ? "--play-fast " : " --play ")
                           + " " + "\"" + xmlFile + "\"";
    if (!FS::Files::directoryExists(Directories::getUserDataDir().toASCIIString())) { FS::Files::createPath(Directories::getUserDataDir().toASCIIString()); }

    std::stringstream ss;
    ss << std::string(Directories::getLogDir().toASCIIString()) << "/tracker-processor-dump-" << identifier << ".txt";
    std::string outputDump = ss.str();

    std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    std::ofstream outStream(outputDump.c_str(), std::ios::app);
    outStream << std::endl << "Date of launch: " << std::ctime(&now) << std::endl;
    outStream << "Trying to launch: " << designer << std::endl;
    outStream << "Args: " << argsAll << std::endl;
    outStream << "Logging to: " << outputDump << std::endl;
    outStream << std::endl;
    outStream.close();

#if TARGET_OS_Windows

    STARTUPINFO si;
    PROCESS_INFORMATION pi;

    // set the size of the structures
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));

    SECURITY_ATTRIBUTES sa;
    sa.nLength              = sizeof(sa);
    sa.lpSecurityDescriptor = nullptr;
    sa.bInheritHandle       = TRUE;

    HANDLE h = CreateFile(outputDump.c_str(), FILE_APPEND_DATA, FILE_SHARE_WRITE | FILE_SHARE_READ, &sa, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

    si.dwFlags |= STARTF_USESTDHANDLES;
    si.hStdInput  = nullptr;
    si.hStdError  = h;
    si.hStdOutput = h;

    argsAll = std::string("\"") + designer + std::string("\"") + std::string(" ") + argsAll;

    LPSTR argsLp = const_cast<char*>(argsAll.c_str());

    BOOL retVal = CreateProcess(designer.c_str(),   // the path
                                argsLp,             // Command line
                                nullptr,            // Process handle not inheritable
                                nullptr,            // Thread handle not inheritable
                                TRUE,               // Set handle inheritance to FALSE
                                0,                  // No creation flags
                                nullptr,            // Use parent's environment block
                                nullptr,            // Use parent's starting directory 
                                &si,                // Pointer to STARTUPINFO structure
                                &pi                 // Pointer to PROCESS_INFORMATION structure (removed extra parentheses)
    );
    // Close process and thread handles. 
    if (!retVal) { std::cout << "err: " << GetLastError() << "\n"; }
    else { WaitForSingleObject(pi.hProcess, INFINITE); }

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(h);
#else

    auto cmd = designer + argsAll + " >" + outputDump;

//  std::cout << "Call is: " << cmd << "\n";

    if (system(cmd.c_str()) != 0)
    {
        std::cout << "Launch of [" << cmd << "] failed\n";
    }
#endif
}

bool Processor::configure(const char* filename)
{
    const std::string usedFilename = (filename ? filename : m_xmlFilename.c_str());

    if (usedFilename.length() == 0) {
        log() << Kernel::LogLevel_Error << "Error: Please set processor filename first\n";
        return false;
    }

    const CString expandedName = m_kernelCtx.getConfigurationManager().expand(usedFilename.c_str());

    const std::string designer   = std::string(Directories::getBinDir().toASCIIString()) + "/designer --no-session-management --open ";
    const std::string outputDump = std::string(Directories::getDistRootDir().toASCIIString()) + "/tracker-processor-configure-dump.txt";
    const std::string cmd        = designer + expandedName.toASCIIString() + " >" + outputDump;

    if (system(cmd.c_str()) != 0) {
        log() << Kernel::LogLevel_Error << "Launch of [" << cmd << "] failed\n";
        return false;
    }
    return true;
}

}  // namespace Tracker
}  // namespace OpenViBE
