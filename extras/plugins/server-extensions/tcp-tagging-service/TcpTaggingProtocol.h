/**
 * SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
 */
#pragma once

#include <cstdint>

namespace OpenViBE {
namespace Services {
namespace TcpTagging {

// Wire-compatible, byte-for-byte identical to the legacy tcp-tagging plugin's
// Tag struct (extras/contrib/plugins/server-extensions/tcp-tagging/ovasCTagStream.h)
// and to what extras/modules/tcptagging's CStimulusSender writes: 3 back-to-back,
// NATIVE-ENDIAN uint64_t, no framing, no byte-swapping.
struct Tag
{
    uint64_t flags = 0;
    uint64_t identifier = 0;
    uint64_t timestamp = 0;
};
static_assert(sizeof(Tag) == 24, "Tag must stay wire-compatible with legacy tcp-tagging (24 raw bytes, no padding)");

// Note: duplicated in the legacy tcp-tagging plugin (ovasCTagStream.h) and in
// extras/modules/tcptagging's IStimulusSender::ETCPTaggingFlags.
enum TCPTaggingFlags : uint64_t
{
    FLAG_FPTIME = (1ULL << 0),               // The time given is fixed point time.
    FLAG_AUTOSTAMP_CLIENTSIDE = (1ULL << 1), // Ignore given stamp, bake timestamp on client side before sending.
    FLAG_AUTOSTAMP_SERVERSIDE = (1ULL << 2)  // Ignore given stamp, bake timestamp on server side when receiving.
};

} // namespace TcpTagging
} // namespace Services
} // namespace OpenViBE
