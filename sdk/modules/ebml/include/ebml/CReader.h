#pragma once

#include "IReader.h"

namespace EBML {
class EBML_API CReader final : public IReader
{
public:

    explicit CReader(IReaderCallback& callback);
    ~CReader() override;
    bool processData(const void* buffer, const size_t size) override;
    CIdentifier getCurrentNodeID() const override;
    size_t getCurrentNodeSize() const override;
    void release() override;

protected:

    IReader* m_impl = nullptr;

private:

    CReader() = delete;
};
}  // namespace EBML
