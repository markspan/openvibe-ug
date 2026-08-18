#pragma once

#include <openvibe/ov_all.h>

namespace OpenViBE {
namespace Kernel {
class CBuffer final : public CMemoryBuffer
{
public:

    CBuffer() { }
    explicit CBuffer(const CBuffer& buffer);

    CBuffer& operator=(const CBuffer& buffer);
};
}  // namespace Kernel
}  // namespace OpenViBE
