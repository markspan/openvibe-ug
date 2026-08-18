#include <cstdint>
#include <random>
#include <iostream>
#include <sstream>
#include <iomanip>

int main(int /*argc*/, char** /*argv*/)
{
    std::random_device rd;
    std::default_random_engine rng(rd());
    std::uniform_int_distribution<uint32_t> uni(0, std::numeric_limits<uint32_t>::max() - 1);

    for (int i = 0; i < 16; ++i) {
        const uint32_t v1 = uni(rng), v2 = uni(rng);
        std::stringstream ss;
        ss.fill('0');
        ss << "(0x" << std::setw(8) << std::hex << v1 << ", 0x" << std::setw(8) << std::hex << v2 << ")";
        std::cout << "#define OV_ClassId_\t\t\tOpenViBE::CIdentifier" << ss.str() << std::endl;
    }

    return 0;
}
