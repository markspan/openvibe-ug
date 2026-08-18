#pragma once

#include "IWriterHelper.h"

namespace EBML {
class EBML_API CWriterHelper final : public IWriterHelper
{
public:

    CWriterHelper();
    ~CWriterHelper() override;
    bool connect(IWriter* writer) override;
    bool disconnect() override;
    bool openChild(const CIdentifier& identifier) override;
    bool closeChild() override;
    bool setInt(const int64_t value) override;
    bool setUInt(const uint64_t value) override;
    bool setFloat(const float value) override;
    bool setDouble(const double value) override;
    bool setBinary(const void* buffer, const size_t size) override;
    bool setStr(const char* value) override;
    void release() override { }

protected:

    IWriterHelper* m_impl = nullptr;
};
}  // namespace EBML
