function(ov_configure_rc)
       set(options )
       set(oneValueArgs NAME ICON_PATH)
       SET(multiValueArgs )
       cmake_parse_arguments(OV_CONFIGURE_RC "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
       SET(GENERATED_RC_FILE "${CMAKE_BINARY_DIR}/resource-files/${OV_CONFIGURE_RC_NAME}.rc")
       if(OV_CONFIGURE_RC_ICON_PATH)
               # Numeric ID, not a name: the Windows shell and Qt's platform plugin look the
               # application icon up by ordinal, and a string-named resource is invisible to both.
               # Lowest ID wins when several icon groups are present, hence 1.
               set(CONFIGURE_ICON "1 ICON DISCARDABLE \"${OV_CONFIGURE_RC_ICON_PATH}\"")
       endif()
       if(NOT(PROJECT_PRODUCT_NAME))
               set(PROJECT_PRODUCT_NAME "${OV_CONFIGURE_RC_NAME}")
       endif()
       set(FILE_DESCRIPTION "${PROJECT_PRODUCT_NAME} for Win32")

       configure_file(
               ${OV_LAUNCHER_SOURCE_PATH}/resource-file.rc-base
               ${GENERATED_RC_FILE}
               @ONLY)
endfunction()