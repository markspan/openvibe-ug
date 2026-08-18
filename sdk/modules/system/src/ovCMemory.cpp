#include "system/ovCMemory.h"

#include <cstring>

namespace System {

// ________________________________________________________________________________________________________________
//
template <typename T>
bool BigEndianToHost(const uint8_t* buffer, T* value)
{
    if (!buffer || !value) { return false; }
    memset(value, 0, sizeof(T));
    for (size_t i = 0; i < sizeof(T); ++i) { ((uint8_t*)value)[i] = buffer[sizeof(T) - 1 - i]; }
    return true;
}

template <typename T>
bool LittleEndianToHost(const uint8_t* buffer, T* value)
{
    if (!buffer || !value) { return false; }
    memset(value, 0, sizeof(T));
    for (size_t i = 0; i < sizeof(T); ++i) { ((uint8_t*)value)[i] = buffer[i]; }
    return true;
}

template <typename T>
bool HostToBigEndian(const T& value, uint8_t* buffer)
{
    if (!buffer) { return false; }
    memset(buffer, 0, sizeof(T));
    for (size_t i = 0; i < sizeof(T); ++i) { buffer[i] = ((uint8_t*)&value)[sizeof(T) - 1 - i]; }
    return true;
}

template <typename T>
bool HostToLittleEndian(const T& value, uint8_t* buffer)
{
    if (!buffer) { return false; }
    for (size_t i = 0; i < sizeof(T); ++i) { buffer[i] = uint8_t((value >> (i * 8)) & 0xff); }
    return true;
}

// ________________________________________________________________________________________________________________
//

bool Memory::hostToLittleEndian(const uint16_t value, uint8_t* buffer) { return HostToLittleEndian<uint16_t>(value, buffer); }
bool Memory::hostToLittleEndian(const uint32_t value, uint8_t* buffer) { return HostToLittleEndian<uint32_t>(value, buffer); }
bool Memory::hostToLittleEndian(const uint64_t value, uint8_t* buffer) { return HostToLittleEndian<uint64_t>(value, buffer); }
bool Memory::hostToLittleEndian(const int16_t value, uint8_t* buffer) { return HostToLittleEndian<int16_t>(value, buffer); }
bool Memory::hostToLittleEndian(const int value, uint8_t* buffer) { return HostToLittleEndian<int>(value, buffer); }
bool Memory::hostToLittleEndian(const int64_t value, uint8_t* buffer) { return HostToLittleEndian<int64_t>(value, buffer); }

bool Memory::hostToLittleEndian(const float value, uint8_t* buffer)
{
    uint32_t tmp;
    memcpy(&tmp, &value, sizeof(tmp));
    return hostToLittleEndian(tmp, buffer);
}

bool Memory::hostToLittleEndian(const double value, uint8_t* buffer)
{
    uint64_t tmp;
    memcpy(&tmp, &value, sizeof(tmp));
    return hostToLittleEndian(tmp, buffer);
}

bool Memory::hostToLittleEndian(const long double /*value*/, uint8_t* /*buffer*/)
{
    // $$$ TODO
    return false;
}


// ________________________________________________________________________________________________________________
//

bool Memory::hostToBigEndian(const uint16_t value, uint8_t* buffer) { return HostToBigEndian<uint16_t>(value, buffer); }
bool Memory::hostToBigEndian(const uint32_t value, uint8_t* buffer) { return HostToBigEndian<uint32_t>(value, buffer); }
bool Memory::hostToBigEndian(const uint64_t value, uint8_t* buffer) { return HostToBigEndian<uint64_t>(value, buffer); }
bool Memory::hostToBigEndian(const int16_t value, uint8_t* buffer) { return HostToBigEndian<int16_t>(value, buffer); }
bool Memory::hostToBigEndian(const int value, uint8_t* buffer) { return HostToBigEndian<int>(value, buffer); }
bool Memory::hostToBigEndian(const int64_t value, uint8_t* buffer) { return HostToBigEndian<int64_t>(value, buffer); }

bool Memory::hostToBigEndian(const float value, uint8_t* buffer)
{
    uint32_t tmp;
    memcpy(&tmp, &value, sizeof(tmp));
    return hostToBigEndian(tmp, buffer);
}

bool Memory::hostToBigEndian(const double value, uint8_t* buffer)
{
    uint64_t tmp;
    memcpy(&tmp, &value, sizeof(tmp));
    return hostToBigEndian(tmp, buffer);
}

bool Memory::hostToBigEndian(const long double /*value*/, uint8_t* /*buffer*/)
{
    // $$$ TODO
    return false;
}

// ________________________________________________________________________________________________________________
//

bool Memory::littleEndianToHost(const uint8_t* buffer, uint16_t* value) { return LittleEndianToHost<uint16_t>(buffer, value); }
bool Memory::littleEndianToHost(const uint8_t* buffer, uint32_t* value) { return LittleEndianToHost<uint32_t>(buffer, value); }
bool Memory::littleEndianToHost(const uint8_t* buffer, uint64_t* value) { return LittleEndianToHost<uint64_t>(buffer, value); }
bool Memory::littleEndianToHost(const uint8_t* buffer, int16_t* value) { return LittleEndianToHost<int16_t>(buffer, value); }
bool Memory::littleEndianToHost(const uint8_t* buffer, int* value) { return LittleEndianToHost<int>(buffer, value); }
bool Memory::littleEndianToHost(const uint8_t* buffer, int64_t* value) { return LittleEndianToHost<int64_t>(buffer, value); }

bool Memory::littleEndianToHost(const uint8_t* buffer, float* value)
{
    uint32_t tmp;
    const bool b = LittleEndianToHost<uint32_t>(buffer, &tmp);
    memcpy(value, &tmp, sizeof(float));
    return b;
}

bool Memory::littleEndianToHost(const uint8_t* buffer, double* value)
{
    uint64_t tmp;
    const bool b = LittleEndianToHost<uint64_t>(buffer, &tmp);
    memcpy(value, &tmp, sizeof(double));
    return b;
}

bool Memory::littleEndianToHost(const uint8_t* /*buffer*/, long double* /*value*/)
{
    // $$$ TODO
    return false;
}

// ________________________________________________________________________________________________________________
//

bool Memory::bigEndianToHost(const uint8_t* buffer, uint16_t* value) { return BigEndianToHost<uint16_t>(buffer, value); }
bool Memory::bigEndianToHost(const uint8_t* buffer, uint32_t* value) { return BigEndianToHost<uint32_t>(buffer, value); }
bool Memory::bigEndianToHost(const uint8_t* buffer, uint64_t* value) { return BigEndianToHost<uint64_t>(buffer, value); }
bool Memory::bigEndianToHost(const uint8_t* buffer, int16_t* value) { return BigEndianToHost<int16_t>(buffer, value); }
bool Memory::bigEndianToHost(const uint8_t* buffer, int* value) { return BigEndianToHost<int>(buffer, value); }
bool Memory::bigEndianToHost(const uint8_t* buffer, int64_t* value) { return BigEndianToHost<int64_t>(buffer, value); }

bool Memory::bigEndianToHost(const uint8_t* buffer, float* value)
{
    uint32_t tmp;
    const bool b = BigEndianToHost<uint32_t>(buffer, &tmp);
    memcpy(value, &tmp, sizeof(float));
    return b;
}

bool Memory::bigEndianToHost(const uint8_t* buffer, double* value)
{
    uint64_t tmp;
    const bool b = BigEndianToHost<uint64_t>(buffer, &tmp);
    memcpy(value, &tmp, sizeof(double));
    return b;
}

bool Memory::bigEndianToHost(const uint8_t* /*buffer*/, long double* /*value*/)
{
    // $$$ TODO
    return false;
}

}  // namespace System
