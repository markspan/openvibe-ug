#include <gtest/gtest.h>

// ReSharper disable CppUnusedIncludeDirective
#include "CMatrixTest.hpp"
#include "CTimeTest.hpp"
#include "CIdentifierTest.hpp"
#include "CStimulationSetTest.hpp"
#include "CNameValuePairListTest.hpp"
#include "CErrorManagerTest.hpp"
#include "CParameterTest.hpp"

int main(int argc, char* argv[])
{
    try {
        testing::InitGoogleTest(&argc, argv);
        return RUN_ALL_TESTS();
    }
    catch (std::exception&) { return 1; }
}
