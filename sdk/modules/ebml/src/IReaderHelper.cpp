#include "ebml/IReaderHelper.h"

#include <string>
#include <cstring>

namespace EBML {
namespace {
class CReaderHelper final : public IReaderHelper
{
public:
    CReaderHelper() { }
    uint64_t getUInt(const void* buffer, const size_t size) override;
    int64_t getInt(const void* buffer, const size_t size) override;
    double getDouble(const void* buffer, const size_t size) override;
    const char* getStr(const void* buffer, const size_t size) override;
    void release() override;

    std::string m_Str;
};
}  // namespace

uint64_t CReaderHelper::getUInt(const void* buffer, const size_t size)
{
    uint64_t result = 0;
    for (size_t i = 0; i < size; ++i)
    {
        result <<= 8;
        result |= reinterpret_cast<const unsigned char*>(buffer)[i];
    }
    return result;
}

int64_t CReaderHelper::getInt(const void* buffer, const size_t size)
{
    int64_t result = 0;
    if (size != 0 && reinterpret_cast<const unsigned char*>(buffer)[0] & 0x80) { result = -1; }

    for (size_t i = 0; i < size; ++i)
    {
        result <<= 8;
        result |= reinterpret_cast<const unsigned char*>(buffer)[i];
    }
    return result;
}

double CReaderHelper::getDouble(const void* buffer, const size_t size)
{
    switch (size)
    {
        case 4:
        {
            float res;
            uint32_t data;
            data = uint32_t(getUInt(buffer, size));
            memcpy(&res, &data, sizeof(res));
            return double(res);
        }

        case 8:
        {
            double res;
            uint64_t data;
            data = getUInt(buffer, size);
            memcpy(&res, &data, sizeof(res));
            return res;
        }

        case 0:
        case 10:
        default: return 0.0;
    }
}

const char* CReaderHelper::getStr(const void* buffer, const size_t size)
{
    if (size) { m_Str.assign(reinterpret_cast<const char*>(buffer), size_t(size)); }
    else { m_Str = ""; }
    return m_Str.c_str();
}

void CReaderHelper::release() { delete this; }

EBML_API IReaderHelper* createReaderHelper() { return new CReaderHelper(); }

}  // namespace EBML
