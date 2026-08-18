///-------------------------------------------------------------------------------------------------
/// 
/// \file Processor.h
/// \author J. T. Lindgren / Inria.
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

#pragma once

#include <openvibe/ov_all.h>
#include <functional>

#include "StreamBundle.h"
#include "Contexted.h"

namespace OpenViBE {
namespace Tracker {
/// <summary> A signal processing component that can receive/return data. </summary>
///
/// Processor in OpenViBE tracker is a kind of bridge to Designer that is used to send data in, do some processing with a scenario, and get the processed data back.
/// A processor can also be one - directional.
///
/// <seealso cref="Contexted" />
class Processor : protected Contexted
{
public:
    explicit Processor(const Kernel::IKernelContext& ctx) : Contexted(ctx) {}

    // Set the processor XML file
    virtual bool initialize(const std::string& xmlFile) = 0;
    virtual bool uninitialize() = 0;

    // Get the processor XML filename
    virtual const std::string& getFilename() const { return m_xmlFilename; }

    // Connect the processor to a specific StreamBundle to read from
    virtual bool setNewSource(StreamBundle* source, bool sendHeader, bool sendEnd) = 0;
    // Connect the processor to a specific StreamBundle to write to
    virtual bool setNewTarget(StreamBundle* target) = 0;

    // Launch Designer to configure the processor
    virtual bool configure(const char* filename); // If filename is NULL, use internal

    // Launch the Player
    virtual bool play(const bool playFast, const std::function<bool(CTime)>& quitCallback) = 0;
    // Stop the Player
    virtual bool stop() = 0;
    virtual bool isRunning() const = 0;

    virtual CTime getCurrentTime() const = 0;

    // Communication port tcp/ip port IDs
    virtual bool setProcessorPorts(const uint32_t sendPort, const uint32_t recvPort) = 0;
    virtual bool getProcessorPorts(uint32_t& sendPort, uint32_t& recvPort) const = 0;

    // Command line arguments passed to the processor (Designer) launch call
    bool setArguments(const char* args)
    {
        m_arguments = std::string(args);
        return true;
    }

    const char* getArguments() const { return m_arguments.c_str(); }

    // Serialize state to configuration manager
    virtual bool save() = 0;
    virtual bool load() = 0;

protected:
    std::string m_xmlFilename;
    std::string m_arguments;
    bool m_playFast = false;
};
}  // namespace Tracker
}  // namespace OpenViBE
