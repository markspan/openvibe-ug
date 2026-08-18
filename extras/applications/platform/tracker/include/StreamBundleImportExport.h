//
// OpenViBE Tracker
//
//
// n.b. having this as a separate file so EBML/Demuxer dependencies do not get pulled into StreamBundle
//

#pragma once

#include <openvibe/ov_all.h>

#include "StreamBundle.h"

namespace OpenViBE {
namespace Tracker {
// These functions import/export stream bundles from .ov files
StreamBundle* readStreamBundleFromFile(const Kernel::IKernelContext& ctx, const char* filename, bool memorySaveMode);
bool saveStreamBundleToFile(const Kernel::IKernelContext& ctx, StreamBundle* track, const char* filename);
}  // namespace Tracker
}  // namespace OpenViBE
