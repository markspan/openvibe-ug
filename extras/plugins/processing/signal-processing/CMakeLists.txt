project(openvibe-plugins-signal-processing VERSION ${OPENVIBE_MAJOR_VERSION}.${OPENVIBE_MINOR_VERSION}.${OPENVIBE_PATCH_VERSION})

file(GLOB_RECURSE SRC_FILES src/*.cpp src/*.h src/*.hpp src/*.inl)

add_library(${PROJECT_NAME} SHARED ${SRC_FILES}
	"../../../contrib/packages/wavelet2d/wavelet2s.cpp"
	"../../../contrib/packages/wavelet2d/wavelet2s.h")

target_link_libraries(${PROJECT_NAME}
					  openvibe
					  openvibe-common
					  openvibe-toolkit
					  openvibe-module-eigen
					  Boost::boost
					  Eigen3::Eigen
					  fftw3
)

target_include_directories(${PROJECT_NAME}
						   PRIVATE
						   src
						   src/algorithms/basic
						   src/algorithms/connectivity
)

set_target_properties(${PROJECT_NAME} PROPERTIES
					  VERSION ${PROJECT_VERSION}
					  SOVERSION ${PROJECT_VERSION_MAJOR}
					  FOLDER ${PLUGINS_FOLDER}
					  COMPILE_FLAGS "-DOVP_Exports -DOVP_Shared"
)

add_definitions(-DTARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines)

include_directories("../../../contrib/packages/wavelet2d")

add_definitions(-D_USE_MATH_DEFINES)

if(OV_COMPILE_TESTS)
	ADD_SUBDIRECTORY("test")
endif(OV_COMPILE_TESTS)

file(COPY signals/ DESTINATION ${BUILD_DATADIR}/scenarios/signals)
file(COPY box-tutorials/ DESTINATION ${BUILD_DATADIR}/scenarios/box-tutorials)

# -----------------------------
# Install files
# -----------------------------
install(TARGETS ${PROJECT_NAME}
	RUNTIME DESTINATION ${DIST_BINDIR}
	LIBRARY DESTINATION ${DIST_LIBDIR}
	ARCHIVE DESTINATION ${DIST_LIBDIR})

install(DIRECTORY signals/       DESTINATION ${DIST_DATADIR}/openvibe/scenarios/signals)
install(DIRECTORY box-tutorials/ DESTINATION ${DIST_DATADIR}/openvibe/scenarios/box-tutorials)
	
