project(openvibe-parameters-object VERSION ${OPENVIBE_MAJOR_VERSION}.${OPENVIBE_MINOR_VERSION}.${OPENVIBE_PATCH_VERSION})

add_library(${PROJECT_NAME} SHARED
parameterBoolObject.h
parameterBoolObject.cpp
parameterColorObject.h
parameterColorObject.cpp
parameterColorGradientObject.h
parameterColorGradientObject.cpp
parameterDoubleObject.h
parameterDoubleObject.cpp
parameterIntObject.h
parameterIntObject.cpp
parameterListIntObject.h
parameterListIntObject.cpp
parameterListStringObject.h
parameterListStringObject.cpp
parameterMultiSelectObject.h
parameterMultiSelectObject.cpp
parameterObject.h
parameterObject.cpp
parameterPathObject.h
parameterPathObject.cpp
parameterStringObject.h
parameterStringObject.cpp
parameterStringVectorObject.h
parameterStringVectorObject.cpp
parametersObject.h
)

generate_export_header(${PROJECT_NAME}
                       BASE_NAME "OV_PARAMETERS_OBJECT"
)

target_include_directories(${PROJECT_NAME}
                           PUBLIC
                            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
                            $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
                            $<INSTALL_INTERFACE:include> #TODO check
                          )

target_link_libraries(${PROJECT_NAME} PUBLIC 
Qt6::Core
openvibe)

set_target_properties(${PROJECT_NAME} PROPERTIES
                      VERSION ${PROJECT_VERSION}
                      SOVERSION ${PROJECT_VERSION_MAJOR}
                      FOLDER ${APP_FOLDER}
)

# Install files
install(TARGETS ${PROJECT_NAME}
		RUNTIME DESTINATION ${DIST_BINDIR}
		LIBRARY DESTINATION ${DIST_LIBDIR}
		ARCHIVE DESTINATION ${DIST_LIBDIR})
