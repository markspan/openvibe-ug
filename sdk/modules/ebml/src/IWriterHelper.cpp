#include "ebml/IWriterHelper.h"
#include "ebml/IWriter.h"

#include <cstdlib>
#include <cstring>

namespace EBML {
namespace {
class CWriterHelper final : public IWriterHelper
{
public:

    CWriterHelper() { }
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
    void release() override;

protected:

    IWriter* m_writer = nullptr;
};
}  // namespace

bool CWriterHelper::connect(IWriter* writer)
{
    m_writer = writer;
    return m_writer ? true : false;
}

bool CWriterHelper::disconnect()
{
    if (!m_writer) { return false; }
    m_writer = nullptr;
    return true;
}

// ________________________________________________________________________________________________________________
//

bool CWriterHelper::openChild(const CIdentifier& identifier) { return m_writer ? m_writer->openChild(identifier) : false; }

bool CWriterHelper::closeChild() { return m_writer ? m_writer->closeChild() : false; }

// ________________________________________________________________________________________________________________
//

bool CWriterHelper::setInt(const int64_t value)
{
    unsigned char buffer[8];

    size_t size = 8;
    if (value == 0x00000000000000LL) { size = 0; }
    else if (value >= -0x00000000000080LL && value <= 0x0000000000007fLL) { size = 1; }
    else if (value >= -0x00000000008000LL && value <= 0x00000000007fffLL) { size = 2; }
    else if (value >= -0x00000000800000LL && value <= 0x000000007fffffLL) { size = 3; }
    else if (value >= -0x00000080000000LL && value <= 0x0000007fffffffLL) { size = 4; }
    else if (value >= -0x00008000000000LL && value <= 0x00007fffffffffLL) { size = 5; }
    else if (value >= -0x00800000000000LL && value <= 0x007fffffffffffLL) { size = 6; }
    else if (value >= -0x80000000000000LL && value <= 0x7fffffffffffffLL) { size = 7; }
    //else { size = 8; }

    for (size_t i = 0; i < size; ++i) { buffer[size - i - 1] = static_cast<unsigned char>((value >> (i * 8)) & 0xff); }

    return m_writer->setChildData(buffer, size);
}

bool CWriterHelper::setUInt(const uint64_t value)
{
    size_t size = 8;
    unsigned char buffer[8];

    if (value == 0x000000000000000LL) { size = 0; }
    else if (value < 0x000000000000100LL) { size = 1; }
    else if (value < 0x000000000010000LL) { size = 2; }
    else if (value < 0x000000001000000LL) { size = 3; }
    else if (value < 0x000000100000000LL) { size = 4; }
    else if (value < 0x000010000000000LL) { size = 5; }
    else if (value < 0x001000000000000LL) { size = 6; }
    else if (value < 0x100000000000000LL) { size = 7; }

    for (size_t i = 0; i < size; ++i) { buffer[size - i - 1] = static_cast<unsigned char>((value >> (i * 8)) & 0xff); }

    return m_writer->setChildData(buffer, size);
}

bool CWriterHelper::setFloat(const float value)
{
    uint32_t res;
    unsigned char buffer[8];
    memcpy(&res, &value, sizeof(value));

    const size_t size = (value != 0 ? 4 : 0);
    for (size_t i = 0; i < size; ++i) { buffer[size - i - 1] = static_cast<unsigned char>((res >> (i * 8)) & 0xff); }
    return m_writer->setChildData(buffer, size);
}

bool CWriterHelper::setDouble(const double value)
{
    uint64_t res;
    unsigned char buffer[8];
    memcpy(&res, &value, sizeof(value));
    const size_t size = (value != 0 ? 8 : 0);
    for (size_t i = 0; i < size; ++i) { buffer[size - i - 1] = static_cast<unsigned char>((res >> (i * 8)) & 0xff); }
    return m_writer->setChildData(buffer, size);
}

bool CWriterHelper::setBinary(const void* buffer, const size_t size) { return m_writer->setChildData(buffer, size); }

bool CWriterHelper::setStr(const char* value) { return m_writer->setChildData(value, strlen(value)); }

// ________________________________________________________________________________________________________________
//

void CWriterHelper::release() { delete this; }

// ________________________________________________________________________________________________________________
//

EBML_API IWriterHelper* createWriterHelper() { return new CWriterHelper(); }

}  // namespace EBML
