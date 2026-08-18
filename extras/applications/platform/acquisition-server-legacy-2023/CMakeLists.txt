message(STATUS  "Skipping Legacy Acquisition Server")
return()

if(OV_DISABLE_GTK)
  message(STATUS  "Skipping Legacy Acquisition Server, no GTK")
  return()
endif(OV_DISABLE_GTK)

project(openvibe-acquisition-server VERSION ${OPENVIBE_MAJOR_VERSION}.${OPENVIBE_MINOR_VERSION}.${OPENVIBE_PATCH_VERSION})

# We call this first as some dependency getters have been coded to not find the libs on x64 until supported
# ---------------------------------
if(WIN32)
	add_definitions(-DWIN32_LEAN_AND_MEAN)
endif(WIN32)

if (NOT GTK2_FOUND)
  message(SEND_ERROR "GTK2 not found - skipping building ${PROJECT_NAME}")
  return()
endif()
if (NOT BOOST_FOUND)
  message(SEND_ERROR "BOOST not found - skipping building ${PROJECT_NAME}")
  return()
endif()
if (NOT Eigen3_FOUND)
  message(SEND_ERROR "Eigen3 not found - skipping building ${PROJECT_NAME}")
  return()
endif()
if (NOT OPENAL_FOUND) 
message(SEND_WARNING "OPENAL not found - skipping building ${PROJECT_NAME}")
  return()
endif()


include("FindOpenViBEContributions")

file(GLOB_RECURSE SRC_FILES src/*.cpp src/*.h src/*.hpp src/*.inl include/*.h include/*.hpp ${CMAKE_SOURCE_DIR}/extras/contrib/common/contribAcquisitionServer.*)

if(PATH_OPENVIBE_CONTRIBUTIONS)
	include("${CMAKE_SOURCE_DIR}/extras/contrib/common/contribAcquisitionServer.cmake")
endif(PATH_OPENVIBE_CONTRIBUTIONS)

set(ADDITIONAL_PATH "${CMAKE_SOURCE_DIR}/extras/plugins/server-extensions/")
file(GLOB_RECURSE ADDITIONAL_SRC_FILES ${ADDITIONAL_PATH}/*.cpp ${ADDITIONAL_PATH}/*.h ${ADDITIONAL_PATH}/*.hpp)
set(SRC_FILES "${SRC_FILES};${ADDITIONAL_SRC_FILES}")

add_executable(${PROJECT_NAME} ${SRC_FILES})

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
					  gtk2
					  OpenAL::OpenAL
					  LSL::lsl
					  # drivers
					  brainproducts-amplifier-sdk
					  tmsi
					  micromed
)

if(WIN32)
	target_link_libraries(${PROJECT_NAME} ws2_32)
endif(WIN32)

set_property(TARGET ${PROJECT_NAME} PROPERTY FOLDER ${APP_FOLDER})

add_definitions(-DTARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines)

include_directories("src/drivers")
include_directories("include")

# add include paths for non-contrib server plugins
file(GLOB children ${ADDITIONAL_PATH}/*)
foreach(FILENAME ${children})
	# message("Testing ${FILENAME}")
	if(IS_DIRECTORY ${FILENAME})
		# message("Adding ${FILENAME}")
		include_directories(${FILENAME})
	endif(IS_DIRECTORY ${FILENAME})
endforeach(FILENAME)

if(WIN32)	
	# @FIXME CERT getting timeBeginPeriod() linker issues without this
	target_link_libraries(${PROJECT_NAME} winmm)
endif(WIN32)

# These scripts will also add directives to copy the necessary bin dlls if any
include("FindThirdPartyBioSemiLabViewAPI")
include("FindThirdPartyEmotivAPI")
include("FindThirdPartyNeXus")
include("FindThirdPartyMCS")
include("FindThirdPartyMensiaAcquisition")
include("FindThirdPartyNeuroelectricsEnobio3G")
include("FindThirdPartyNeuroServoDeps")
include("FindThirdPartyThinkGearAPI")
include("FindThirdPartyUSBFirstAmpAPI")

include("FindThirdPartyPThread")	# for Windows Fieldtrip
include("FindThirdPartyRT")  # external stims shared memory needs this

if(PATH_OPENVIBE_CONTRIBUTIONS)
	include("${CMAKE_SOURCE_DIR}/extras/contrib/common/contribAcquisitionServerLinkLibs.cmake")
endif(PATH_OPENVIBE_CONTRIBUTIONS)

if(NOT(DISABLE_MENSIA_ACQUISITION_DRIVER))
	add_definitions(-DTARGET_HasMensiaAcquisitionDriver)
endif()

# -----------------------------
# Install files
# -----------------------------
install(TARGETS ${PROJECT_NAME}
	RUNTIME DESTINATION ${DIST_BINDIR}
	LIBRARY DESTINATION ${DIST_LIBDIR}
	ARCHIVE DESTINATION ${DIST_LIBDIR})

configure_file(share/interface.ui-base "${CMAKE_CURRENT_BINARY_DIR}/interface.ui")
configure_file(share/interface.ui-base "${BUILD_DATADIR}/applications/acquisition-server/interface.ui")
file(COPY share/ DESTINATION ${BUILD_DATADIR}/applications/acquisition-server PATTERN "*-base*" EXCLUDE)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/interface.ui" DESTINATION ${DIST_DATADIR}/openvibe/applications/acquisition-server)
install(DIRECTORY share/ DESTINATION ${DIST_DATADIR}/openvibe/applications/acquisition-server PATTERN "*-base" EXCLUDE)

install_root_alias($<TARGET_FILE_NAME:${PROJECT_NAME}>)
