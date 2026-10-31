#pragma once

/**
 * @file Charmeleon.h
 * @brief Launching and closing Charmeleon, the external impedance checker for EEGO and TMSi amplifiers.
 *
 * Charmeleon opens the amplifier itself, so it must be gone before an acquisition opens it.
 *
 * UG fork: new file, not in upstream OpenViBE, which checks impedances in the server itself.
 */

#include <string>
#include <vector>

namespace Charmeleon {

/**
 * @brief Finds Charmeleon.exe: the AcquisitionServer_CharmeleonPath token if set, else where its
 *        installer put it.
 * @param configuredPath Value of the configuration token, may be empty.
 * @return Full path of Charmeleon.exe, or an empty string when it is not installed.
 */
std::string findExecutable(const std::string& configuredPath);

/**
 * @brief A Charmeleon the server started, to take its montage over when it closes.
 *
 * Charmeleon 2.2 and later saves its montage when the server closes it (--save-on-close) and writes which
 * file it ended with to the result file (--result); see takeResult().
 */
struct Launched
{
    void* process = nullptr;  ///< The process (a Windows HANDLE), signalled when it ends; null when none was started
    std::string resultFile;   ///< Where Charmeleon writes, on closing, which montage file it ended with
};

/**
 * @brief Starts Charmeleon, or brings it to the front when it already runs.
 * @param executable Full path of Charmeleon.exe.
 * @param montage    Montage (.json) for Charmeleon to open, empty for its default layout. A Charmeleon
 *                   that already runs keeps the montage it has.
 * @param arguments  Further arguments, e.g. which amplifier to open; see AbstractDriver::charmeleonArguments().
 * @param launched   Filled when a new Charmeleon started, which then saves its montage when closed and says
 *                   which file it ended with; left empty when one already ran. Pass it to takeResult() once
 *                   the process has ended.
 * @return False when it could not be started.
 */
bool launch(const std::string& executable, const std::string& montage = {}, const std::vector<std::string>& arguments = {},
            Launched* launched = nullptr);

/// What a Charmeleon the server started reported when it closed.
struct Result
{
    bool valid   = false;  ///< False when it wrote nothing, e.g. a Charmeleon before 2.2, or one that crashed
    std::string file;      ///< The montage file it ended with, empty for none
    bool saved   = false;  ///< True when it saved the montage during its run
    bool unsaved = false;  ///< True when it closed with changes that are not saved
};

/**
 * @brief Reads what a Charmeleon reported on closing, and releases @p launched: its process handle and result file.
 *
 * Call it once the process has ended; @p launched is empty afterwards.
 */
Result takeResult(Launched& launched);

/**
 * @brief Closes every running Charmeleon, so that it releases the amplifier.
 *
 * Asks each one to close first, which lets it release the amplifier cleanly (and Charmeleon 2.2 save its
 * montage), and terminates those still running after @p timeoutMs.
 * @return Number of instances that were running.
 */
int closeAll(unsigned long timeoutMs = 3000);

}  // namespace Charmeleon
