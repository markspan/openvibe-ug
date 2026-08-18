#pragma once

#ifdef TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines

#include "../../ovtk_base.h"

#include "../ovtkTCodec.h"

namespace OpenViBE {
namespace Toolkit {
template <class T>
class TDecoderLocal : public T
{
protected:

    Kernel::TParameterHandler<const CMemoryBuffer*> m_iBuffer;


    using T::m_codec;
    using T::m_boxAlgorithm;
    using T::m_connectorIdx;

    virtual void setInputChunk(const CMemoryBuffer* pInputChunkMemoryBuffer) { m_iBuffer = pInputChunkMemoryBuffer; }

    virtual Kernel::TParameterHandler<const CMemoryBuffer*>& getInputMemoryBuffer() { return m_iBuffer; }

    virtual bool isOutputTriggerActive(const CIdentifier oTrigger) { return m_codec->isOutputTriggerActive(oTrigger); }

    virtual bool process(const CIdentifier& oTrigger) { return m_codec->process(oTrigger); }
    virtual bool process() { return m_codec->process(); }

public:
    // We make visible the initialize methods of the superclass (should be TCodec), in the same scope (public)
    using T::initialize;

    /*
    This public function handles every aspects of the decoding process:
    - fill the input memory buffer with a chunk
    - decode it (specific for each decoder)
    - mark input as deprecated
    */
    virtual bool decode(const size_t chunkIdx, const bool markInputAsDeprecated = true)
    {
        this->setInputChunk(m_boxAlgorithm->getDynamicBoxContext().getInputChunk(m_connectorIdx, chunkIdx));
        if (! m_codec->process()) return false;
        if (markInputAsDeprecated) m_boxAlgorithm->getDynamicBoxContext().markInputAsDeprecated(m_connectorIdx, chunkIdx);
        return true;
    }

    // We explicitly delete the decode function taking two integers as parameters
    // in order to raise errors in plugins using the older API
#ifndef TARGET_OS_MacOS // Current clang has a bug which fails to link these
    virtual bool decode(int, int)       = delete;
    virtual bool decode(size_t, size_t) = delete;
#endif

    // The functions that need to be specified by the decoders (specific Trigger ID)
    virtual bool isHeaderReceived() = 0;
    virtual bool isBufferReceived() = 0;
    virtual bool isEndReceived() = 0;
};

/*
This class provides an access to the superclass TDecoder.
Use case : iterating over a vector of TDecoder, calling decode() each time.
You don't need to know which type of decoder is in the vector.
*/
template <class T>
class TDecoder : public TDecoderLocal<TCodec<T>>
{
public:
    virtual ~TDecoder() { }
protected:
    // constructor is protected, ensuring we can't instanciate a TDecoder
    TDecoder() { }
};
}  // namespace Toolkit
}  // namespace OpenViBE

#endif // TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines
