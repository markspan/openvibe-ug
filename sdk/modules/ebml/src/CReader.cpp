#include "ebml/CReader.h"

namespace EBML {

CReader::CReader(IReaderCallback& callback) { m_impl = createReader(callback); }
CReader::~CReader() { m_impl->release(); }

bool CReader::processData(const void* buffer, const size_t size) { return m_impl->processData(buffer, size); }
CIdentifier CReader::getCurrentNodeID() const { return m_impl->getCurrentNodeID(); }
size_t CReader::getCurrentNodeSize() const { return m_impl->getCurrentNodeSize(); }

void CReader::release() {}

}  // namespace EBML
