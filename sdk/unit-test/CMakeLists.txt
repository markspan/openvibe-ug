project(OpenViBESDKTest VERSION ${OPENVIBE_MAJOR_VERSION}.${OPENVIBE_MINOR_VERSION}.${OPENVIBE_PATCH_VERSION})

# ------------------
# Project variables
# ------------------

if(NOT DEFINED OVT_TEST_DATA_DIR)
	MESSAGE(FATAL_ERROR "Path to tests input data files is not set")
endif()

# ---------------------
# Project dependencies
# ---------------------

# Add some search directory to module path
set(OVT_CMAKE_DIR ${OV_BASE_DIR}/cmake-modules)
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${OVT_CMAKE_DIR})

# --------------------
# Test configuration
# --------------------

# ${PROJECT_BINARY_DIR}/Testing/Temporary is automatically created by CTest for log files
# We create a temporary subdir for our test that will be create before each test is run
# and removed afterwards (see the repo root's CTestCustom.cmake.in)
set(OVT_TEST_TEMPORARY_DIR ${OV_BASE_BIN_DIR}/Testing/Temporary/OVT/)

# Create test data directory for big or binary files
file(MAKE_DIRECTORY ${OVT_TEST_DATA_DIR})

# Utility variable for subproject
set(OVT_OPENVIBE_DATA ${CMAKE_CURRENT_BINARY_DIR}/data/openvibe)

# ----------------------
# CMake tree processing
# ----------------------

# Process tools first.
# Some important variables are defined within tools projects for their use by other
# projects as:
# - OVT_UNIT_TOOLKIT_INCLUDE_DIR
# - OVT_UNIT_TOOLKIT_TARGET_FILE
# - OVT_ACCURACY_TARGET_FILE
add_subdirectory(unit-toolkit)

include_directories(${OVT_UNIT_TOOLKIT_INCLUDE_DIR})

# Process unit tests
# When adding a new test driver, driver directory must be added here
add_subdirectory(ov-base)
add_subdirectory(openvibe-module-csv)
add_subdirectory(openvibe-module-fs)
add_subdirectory(openvibe-module-ebml)
add_subdirectory(openvibe-module-xml)
add_subdirectory(openvibe-module-socket)
add_subdirectory(openvibe-module-system)
add_subdirectory(openvibe-toolkit)
add_subdirectory(openvibe-plugin-stream-codecs)
