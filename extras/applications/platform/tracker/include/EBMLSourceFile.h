#pragma once

#include <iostream>
#include <thread>
#include <deque>
#include <vector>

#include "../../../../plugins/processing/file-io/src/ovp_defines.h"

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

#include <system/ovCTime.h>
#include <fs/Files.h>

#include "Contexted.h"

namespace OpenViBE {
namespace Tracker {

/**
 * \class EBMLSourceFile 
 * \brief Reads bytes from an .ov file
 * \author J. T. Lindgren
 *
 * @note some ov files which have lots of stimulation chunks take a long time to import
 * when launching the Tracker from visual studio. this is probably due to memory allocation, similar to slow simple dsp grammar parsing.
 *
 *
 */
class EBMLSourceFile final : protected Contexted
{
public:
    explicit EBMLSourceFile(const Kernel::IKernelContext& ctx) : Contexted(ctx) { }

    bool initialize(const char* signalFile)
    {
        log() << Kernel::LogLevel_Trace << "EBMLSource: Initializing with " << signalFile << "\n";

        m_file = FS::Files::open(signalFile, "rb");
        if (!m_file) { return false; }

        m_src = std::string(signalFile);

        return true;
    }

    bool uninitialize()
    {
        log() << Kernel::LogLevel_Trace << "EBMLSource: Uninitializing\n";

        if (m_file) {
            fclose(m_file);
            m_file = nullptr;
        }

        return true;
    }

    bool isEOF() const
    {
        if (!m_file) { return true; }
        return (feof(m_file) != 0 ? true : false);
    }

    bool read(std::vector<uint8_t>& bytes, const size_t numBytes) const
    {
        if (!m_file) {
            log() << Kernel::LogLevel_Error << "Error: No EBML source file set\n";
            return false;
        }

        if (isEOF()) {
            log() << Kernel::LogLevel_Trace << "EBML Source file EOF reached\n";
            return false;
        }

        bytes.resize(numBytes);
        const size_t s = fread(&bytes[0], sizeof(uint8_t), numBytes, m_file);
        bytes.resize(s); // array size tells how much was read

        return true;
    }

    const std::string& getSource() const { return m_src; }

protected:
    std::string m_src;
    FILE* m_file = nullptr;
};
}  // namespace Tracker
}  // namespace OpenViBE
