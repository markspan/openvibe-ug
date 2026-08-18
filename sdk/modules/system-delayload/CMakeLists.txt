project(openvibe-module-system-delayload VERSION ${OPENVIBE_MAJOR_VERSION}.${OPENVIBE_MINOR_VERSION}.${OPENVIBE_PATCH_VERSION})

add_library(${PROJECT_NAME} STATIC src/DelayLoadHook.cpp)

if(MSVC)
	target_link_libraries(${PROJECT_NAME} delayimp.lib)
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES
					  FOLDER ${MODULES_FOLDER})

install(TARGETS ${PROJECT_NAME}
		ARCHIVE DESTINATION ${DIST_LIBDIR})
