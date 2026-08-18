#include "ebml/CWriter.h"

namespace EBML {

CWriter::CWriter(IWriterCallback& callback) { m_impl = createWriter(callback); }
CWriter::~CWriter() { m_impl->release(); }

bool CWriter::openChild(const CIdentifier& identifier) { return m_impl->openChild(identifier); }
bool CWriter::setChildData(const void* buffer, const size_t size) { return m_impl->setChildData(buffer, size); }
bool CWriter::closeChild() { return m_impl->closeChild(); }

void CWriter::release() {}

}  // namespace EBML
