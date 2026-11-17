include(TestUtilities)

file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/scenarios-tests DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

add_validation_test_visualization(scenarios-tests Display-cue-image)
add_validation_test_visualization(scenarios-tests ERP-plot)
add_validation_test_visualization(scenarios-tests Multimodal-Graz-visualization)
# if (NOT APPLE)
#    add_validation_test_visualization(scenarios-tests Power-Spectrum)
#    add_validation_test_visualization(scenarios-tests Topography)
# endif()
add_validation_test_visualization(scenarios-tests P300-speller)  # UG fork
# UG fork: the speller's scripts, in Lua and in Python, all the way to the letters. They pass only when their check box says so.
foreach(LANGUAGE lua python)
    add_validation_test_visualization(scenarios-tests P300-speller-${LANGUAGE})
    set_tests_properties(run_P300-speller-${LANGUAGE} PROPERTIES PASS_REGULAR_EXPRESSION "P300 speller scripts: 2 letters, as expected")
endforeach()
add_validation_test_visualization(scenarios-tests Simple-visualization)

# UG fork: the part of the P300 speller display that needs neither Qt nor the player
add_executable(test-p300-speller P300SpellerStateTest.cpp ../src/box-algorithms/P300SpellerState.cpp)
target_include_directories(test-p300-speller PRIVATE ../src/box-algorithms)
set_property(TARGET test-p300-speller PROPERTY FOLDER ${TESTS_FOLDER})
target_link_libraries(test-p300-speller PRIVATE tinyxml2::tinyxml2 GTest::GTest GTest::Main)

include(GoogleTest)
gtest_discover_tests(test-p300-speller)

# UG fork: the parts of the Signal display that need neither Qt nor the player: its automatic vertical scale
# (SignalScaling.h) and its ring of samples (CTraceBuffer)
add_executable(test-signal-display SignalScalingTest.cpp TraceBufferTest.cpp ../src/CTraceBuffer.cpp)
target_include_directories(test-signal-display PRIVATE ../src)
set_property(TARGET test-signal-display PROPERTY FOLDER ${TESTS_FOLDER})
target_link_libraries(test-signal-display PRIVATE GTest::GTest GTest::Main)
gtest_discover_tests(test-signal-display)

# UG fork: the LSL display's reader, against LSL streams the test sends itself on this computer
add_executable(test-lsl-reader LSLReaderTest.cpp ../src/CLSLReader.cpp ../src/CTraceBuffer.cpp)
target_include_directories(test-lsl-reader PRIVATE ../src)
set_property(TARGET test-lsl-reader PROPERTY FOLDER ${TESTS_FOLDER})
target_link_libraries(test-lsl-reader PRIVATE LSL::lsl GTest::GTest GTest::Main)
gtest_discover_tests(test-lsl-reader DISCOVERY_TIMEOUT 30)

# UG fork: the LSL XDF writer: records LSL streams the test sends itself, and reads the file back
add_executable(test-lsl-recorder LSLRecorderTest.cpp ../src/CLSLRecorder.cpp ../src/XDFWriter.cpp)
target_include_directories(test-lsl-recorder PRIVATE ../src)
set_property(TARGET test-lsl-recorder PROPERTY FOLDER ${TESTS_FOLDER})
target_link_libraries(test-lsl-recorder PRIVATE LSL::lsl GTest::GTest GTest::Main)
gtest_discover_tests(test-lsl-recorder DISCOVERY_TIMEOUT 30)
