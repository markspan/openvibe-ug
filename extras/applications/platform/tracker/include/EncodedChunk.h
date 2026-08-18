#pragma once

#include <vector>

#include "Chunk.h"

namespace OpenViBE {
namespace Tracker {

/**
 * \class EncodedChunk
 * \brief Data class for encoded chunks
 * \details 
 * Stream content in OpenViBE is carried as encoded chunks. Although this inherits from Chunk, 
 * this type is a bit special since its not explicitly have (header, buffer, end) subclasses. This is
 * because in its encoded form, the three are not differentiated but coded into bufferData.
 *
 * \note Here we use uint32_t for m_streamIdx as we assume it has fixed bitcount (unlike size_t which may change).
 */
class EncodedChunk : public Chunk
{
public:
    // These are actually passed around in OpenViBE
    std::vector<uint8_t> m_Buffer;

    // These are convenience information for Tracker
    uint64_t m_StreamType = 0;
    size_t m_StreamIndex  = 0;
};

// Note that we do not store this chunk type in EncodedChunk because when we receive EncodedChunks, 
// the type is not available until 'bufferData' has been decoded -- especially the case for the End type?
enum class EChunkType { Header = 0, Buffer = 1, End = 2, };
}  // namespace Tracker
}  // namespace OpenViBE
