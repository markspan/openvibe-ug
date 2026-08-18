project(OVASDrivers VERSION ${OPENVIBE_MAJOR_VERSION}.${OPENVIBE_MINOR_VERSION}.${OPENVIBE_PATCH_VERSION})

# We call this first as some dependency getters have been coded to not find the libs on x64 until supported
# ---------------------------------
if(WIN32)
    add_definitions(-DWIN32_LEAN_AND_MEAN)
endif(WIN32)


#file(GLOB_RECURSE SRC_FILES ./*.cpp ./*.h ./*.hpp)

set(SRC_FILES
    egi-ampserver/ovasCDriverEGIAmpServer.cpp
    egi-ampserver/ovasCDriverEGIAmpServer.h
    generic-oscillator/ovasCDriverGenericOscillator.cpp 
    generic-oscillator/ovasCDriverGenericOscillator.h
    generic-raw-reader/ovasCDriverGenericRawFileReader.cpp
    generic-raw-reader/ovasCDriverGenericRawFileReader.h
    generic-raw-reader/ovasCDriverGenericRawReader.cpp
    generic-raw-reader/ovasCDriverGenericRawReader.h
    generic-raw-reader/ovasCDriverGenericRawTelnetReader.cpp
    generic-raw-reader/ovasCDriverGenericRawTelnetReader.h
    generic-time-signal/ovasCDriverGenericTimeSignal.cpp
    generic-time-signal/ovasCDriverGenericTimeSignal.h
    generic-sawtooth/ovasCDriverGenericSawTooth.h
    generic-sawtooth/ovasCDriverGenericSawTooth.cpp
    simulated-deviator/ovasCDriverSimulatedDeviator.cpp
    simulated-deviator/ovasCDriverSimulatedDeviator.h)

add_library(${PROJECT_NAME} STATIC ${SRC_FILES})


target_compile_definitions(${PROJECT_NAME} 
                           PUBLIC -DTARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines
)

target_include_directories(${PROJECT_NAME}
PUBLIC .
)


target_link_libraries(${PROJECT_NAME}
                      openvibe
                      openvibe-common
                      openvibe-toolkit
                      openvibe-module-socket
                      openvibe-module-system
                      openvibe-module-fs
                      openvibe-module-lsl
                      Boost::boost
                      Boost::system
                      Eigen3::Eigen
                      # drivers
                      brainproducts-amplifier-sdk
                      OVASCore
)

add_definitions(-DTARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines)

# Install files
install(TARGETS ${PROJECT_NAME}
		RUNTIME DESTINATION ${DIST_BINDIR}
		LIBRARY DESTINATION ${DIST_LIBDIR}
		ARCHIVE DESTINATION ${DIST_LIBDIR})
