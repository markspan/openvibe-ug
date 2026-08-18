///-------------------------------------------------------------------------------------------------
/// 
/// \file ITrackerPlugin.h
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

#include "Contexted.h"
#include "StreamBundle.h"
#include "Workspace.h"
#include "Selection.h"
#include "ParallelExecutor.h"

namespace OpenViBE {
namespace Tracker {
/// <summary> Tracker Plugin. </summary>
///
/// Tracker Plugins are processing plugins specific to the OpenViBE Tracker.
/// The idea is to allow plugins to access the whole track content(StreamBundle) with a simple interfaceand minimal overhead.
///
/// In detail, the difference between Tracker Plugins and Box Plugins is as follows. 
/// A box plugin wraps openvibe box code, and subsequently the code will have
/// to deal with the classical openvibe objects such as encoders, decoders, and so on.
/// The Tracker Plugin, on the other hand, gives the programmer access to the StreamBundle structure.
/// This allows the plugin code to requestand manipulate the streamsand their chunks freely.
/// A tracker plugin can also add or remove streams to the bundle.
///  
/// Tracker plugins do not currently have explicit parameters, but you
/// can pass parameters in using configuration tokens, and then request / set them
/// via m_kernelCtx.getConfigurationManager() interface.
///  
/// Note that the StreamBundle given as input to a Tracker Plugin is in not safeguarded against "bad" modifications by the plugin.
///
/// \todo tracker and box plugins might be refactorable to be under same interface.
/// \todo allow non-inplace mode operation.
/// \todo derive different subclasses for the 'capabilities' instead of one monolithic interface ?
class ITrackerPlugin : protected Contexted
{
public:
    // For the moment these are mutually exclusive capabilities; Tracker will prefer 'workspace' if the plugin supports it.
    enum class ECapabilities { Tracks = 1LL, Workspace = 2LL };

    // Constructor
    explicit ITrackerPlugin(const Kernel::IKernelContext& ctx) : Contexted(ctx) { }

    // @param track The input track. It can be modified by the process() call.
    // @return True on success, False otherwise
    // @note If the plugin changes the track, it must set the tracks 'dirty bit' as true.
    virtual bool process(StreamBundle& /*track*/) { return false; }

    // @param wp The input workspace. It can be modified by the process() call.
    // @param exec A reference to a parallel executor that the plugin can use (if it prefers)
    // @return True on success, False otherwise
    // @note If the plugin changes any track in the workspace, it must set the tracks 'dirty bit' as true.
    // @note Passing the parallel executor as a parameter is not too neat, ideally it'd be part of IKernelContext but that'd require importing the exec to SDK.
    virtual bool process(Workspace& /*wp*/, ParallelExecutor& /*exec*/) { return false; }

    // @param Capability to ask for
    // @return True if the plugin supports this
    // @note By default, the plugins are expected to have the track processing capability. If not, override this default.
    virtual bool hasCapability(const ECapabilities capability) { return (capability == ECapabilities::Tracks); }

    // @return the name of the plugin
    virtual std::string getName() { return std::string("Unnamed"); }
};
}  // namespace Tracker
}  // namespace OpenViBE
