#pragma once

#include "IReaderHelper.h"

namespace EBML {
class EBML_API CReaderHelper final : public IReaderHelper
{
public:

    CReaderHelper();
    ~CReaderHelper() override;
    uint64_t getUInt(const void* buffer, const size_t size) override;
    int64_t getInt(const void* buffer, const size_t size) override;
    double getDouble(const void* buffer, const size_t size) override;
    const char* getStr(const void* buffer, const size_t size) override;
    void release() override;

protected:

    IReaderHelper* m_impl = nullptr;
};
}  // namespace EBML
