#include "ebml/CWriterHelper.h"

namespace EBML {

CWriterHelper::CWriterHelper() { m_impl = createWriterHelper(); }
CWriterHelper::~CWriterHelper() { m_impl->release(); }

bool CWriterHelper::connect(IWriter* writer) { return m_impl->connect(writer); }
bool CWriterHelper::disconnect() { return m_impl->disconnect(); }

bool CWriterHelper::openChild(const CIdentifier& identifier) { return m_impl->openChild(identifier); }
bool CWriterHelper::closeChild() { return m_impl->closeChild(); }

bool CWriterHelper::setInt(const int64_t value) { return m_impl->setInt(value); }
bool CWriterHelper::setUInt(const uint64_t value) { return m_impl->setUInt(value); }
bool CWriterHelper::setFloat(const float value) { return m_impl->setFloat(value); }
bool CWriterHelper::setDouble(const double value) { return m_impl->setDouble(value); }
bool CWriterHelper::setBinary(const void* buffer, const size_t size) { return m_impl->setBinary(buffer, size); }
bool CWriterHelper::setStr(const char* value) { return m_impl->setStr(value); }

}  // namespace EBML
