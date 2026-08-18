#include "ovkCBuffer.h"

namespace OpenViBE {
namespace Kernel {

CBuffer::CBuffer(const CBuffer& buffer)
{
    this->CMemoryBuffer::setSize(buffer.getSize(), true);
    memcpy(this->CMemoryBuffer::getDirectPointer(), buffer.getDirectPointer(), buffer.getSize());
}

CBuffer& CBuffer::operator=(const CBuffer& buffer)
{
    this->CMemoryBuffer::setSize(buffer.getSize(), true);
    memcpy(this->CMemoryBuffer::getDirectPointer(), buffer.getDirectPointer(), buffer.getSize());
    return *this;
}

}  // namespace Kernel
}  // namespace OpenViBE
