// Appeasing Intellisense by including the following
#include "ov_common_defines.h"
#include <cstdint>

int main()
{
#if defined(WIN32) || (defined(LINUX) && defined(__GXX_EXPERIMENTAL_CXX0X__))
    static_assert(sizeof(uint64_t) >= 8, "uint64_t is not at least 8 bytes");
    static_assert(sizeof(uint32_t) >= 4, "uint32_t is not at least 4 bytes");
    static_assert(sizeof(uint16_t) >= 2, "uint16_t is not at least 2 bytes");
    static_assert(sizeof(uint8_t) >= 1, "uint8_t is not at least 1 byte");

    static_assert(sizeof(int64_t) >= 8, "int64_t is not at least 8 bytes");
    static_assert(sizeof(int) >= 4, "int is not at least 4 bytes");
    static_assert(sizeof(int16_t) >= 2, "int16_t is not at least 2 bytes");
    static_assert(sizeof(int8_t) >= 1, "int8_t is not at least 1 byte");

#if defined(LINUX)
    static_assert(sizeof(long double)>=10, "long double is not at least 10 bytes");
#endif
    // Float80 seems to be the same size as double on Win at the time of writing, but its not widely used in openvibe.  
    static_assert(sizeof(double) >= 8, "double is not at least 8 bytes");
    static_assert(sizeof(float) >= 4, "float is not at least 4 bytes");
#endif

    return 0;
}
