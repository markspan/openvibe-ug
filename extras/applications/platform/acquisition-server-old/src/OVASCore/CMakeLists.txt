project(OVASCore VERSION ${OPENVIBE_MAJOR_VERSION}.${OPENVIBE_MINOR_VERSION}.${OPENVIBE_PATCH_VERSION})

set(SRC
  ovasCAcquisitionServer.h
  ovasCAcquisitionServer.cpp
  #ovasCSettingsHelper.h
  #ovasCSettingsHelper.cpp
  ovasCHeader.h
  ovasCHeader.cpp
  ovasCDriftCorrection.h
  ovasCDriftCorrection.cpp
)

# add additional plugins files
set(ADDITIONAL_PATHS "${CMAKE_SOURCE_DIR}/extras/plugins/server-extensions/fiddler"
                     "${CMAKE_SOURCE_DIR}/extras/contrib/plugins/server-extensions/tcp-tagging"
                     "${CMAKE_SOURCE_DIR}/extras/contrib/plugins/server-extensions/external-stimulations"
)

foreach(ADDITIONAL_PATH ${ADDITIONAL_PATHS})
  file(GLOB_RECURSE ADD_FILES ${ADDITIONAL_PATH}/*.cpp ${ADDITIONAL_PATH}/*.h ${ADDITIONAL_PATH}/*.hpp)
  set(ADDITIONAL_SRC_FILES "${ADD_FILES};${ADDITIONAL_SRC_FILES}")
endforeach(ADDITIONAL_PATH)

add_definitions(-DTARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines)

add_library(${PROJECT_NAME} STATIC
  ${SRC}
  ${ADDITIONAL_SRC_FILES}
)

target_include_directories(${PROJECT_NAME}
  PUBLIC include .)

foreach(ADDITIONAL_PATH ${ADDITIONAL_PATHS})
  target_include_directories(${PROJECT_NAME} PUBLIC ${ADDITIONAL_PATH})
endforeach(ADDITIONAL_PATH)

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
  LSL::lsl
  # drivers
  brainproducts-amplifier-sdk
)

if (UNIX AND NOT APPLE)
  target_link_libraries(${PROJECT_NAME} rt)
endif(UNIX AND NOT APPLE)

if(WIN32)
    # @FIXME CERT getting timeBeginPeriod() linker issues without this
    target_link_libraries(${PROJECT_NAME} winmm)
endif(WIN32)


# These scripts will also add directives to copy the necessary bin dlls if any
# include("FindThirdPartyBioSemiLabViewAPI")
# include("FindThirdPartyEmotivAPI")
# include("FindThirdPartyNeXus")
# include("FindThirdPartyMCS")
# include("FindThirdPartyMensiaAcquisition")
# include("FindThirdPartyNeuroelectricsEnobio3G")
# include("FindThirdPartyNeuroServoDeps")
# include("FindThirdPartyThinkGearAPI")
# include("FindThirdPartyUSBFirstAmpAPI")
#
# include("FindThirdPartyPThread")	# for Windows Fieldtrip
# include("FindThirdPartyRT")  # external stims shared memory needs this
#
#
# if(PATH_OPENVIBE_CONTRIBUTIONS)
#     include("${CMAKE_SOURCE_DIR}/extras/contrib/common/contribAcquisitionServerLinkLibs.cmake")
# endif(PATH_OPENVIBE_CONTRIBUTIONS)
#
# if(NOT(DISABLE_MENSIA_ACQUISITION_DRIVER))
#     add_definitions(-DTARGET_HasMensiaAcquisitionDriver)
# endif()


file(COPY share/ DESTINATION ${BUILD_DATADIR}/applications/acquisition-server PATTERN "*-base*" EXCLUDE)
install(DIRECTORY share/ DESTINATION ${DIST_DATADIR}/openvibe/applications/acquisition-server PATTERN "*-base" EXCLUDE)

# Install files
install(TARGETS ${PROJECT_NAME}
		RUNTIME DESTINATION ${DIST_BINDIR}
		LIBRARY DESTINATION ${DIST_LIBDIR}
		ARCHIVE DESTINATION ${DIST_LIBDIR})
