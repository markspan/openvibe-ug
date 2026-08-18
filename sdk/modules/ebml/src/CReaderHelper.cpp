#include "ebml/CReaderHelper.h"

namespace EBML {

CReaderHelper::CReaderHelper() { m_impl = createReaderHelper(); }
CReaderHelper::~CReaderHelper() { m_impl->release(); }

uint64_t CReaderHelper::getUInt(const void* buffer, const size_t size) { return m_impl->getUInt(buffer, size); }
int64_t CReaderHelper::getInt(const void* buffer, const size_t size) { return m_impl->getInt(buffer, size); }
double CReaderHelper::getDouble(const void* buffer, const size_t size) { return m_impl->getDouble(buffer, size); }
const char* CReaderHelper::getStr(const void* buffer, const size_t size) { return m_impl->getStr(buffer, size); }

void CReaderHelper::release() {}

}  // namespace EBML
