#pragma once

#include "IWriter.h"

namespace EBML {
class EBML_API CWriter final : public IWriter
{
public:

    explicit CWriter(IWriterCallback& callback);
    ~CWriter() override;
    bool openChild(const CIdentifier& identifier) override;
    bool setChildData(const void* buffer, const size_t size) override;
    bool closeChild() override;
    void release() override;

protected:

    IWriter* m_impl = nullptr;

private:

    CWriter() = delete;
};
}  // namespace EBML
