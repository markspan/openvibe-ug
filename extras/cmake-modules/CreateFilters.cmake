# @function get_common_directory
# @brief Finds the longest common directory prefix of a list of file paths.
# 
# This function is essential for creating clean source_group filters when 
# dealing with files spread across multiple external directories.
#
# @param BASE_PATH The base directory to resolve relative paths against (e.g., CMAKE_CURRENT_SOURCE_DIR).
# @param OUT_VAR The variable name in the parent scope to store the common directory path.
# @param ARGN The list of file paths to analyze.
function(get_common_directory BASE_PATH OUT_VAR)
    # The list of external files is passed as an argument (argn)
    set(EXTERNAL_FILES ${ARGN})

    if(NOT EXTERNAL_FILES)
        set(${OUT_VAR} "" PARENT_SCOPE)
        return()
    endif()

    # Start with the absolute directory of the first file as the initial prefix
    list(GET EXTERNAL_FILES 0 first_file)
    
    # Use BASE_PATH to resolve the absolute path for the first file
    get_filename_component(abs_first_file "${first_file}" ABSOLUTE BASE "${BASE_PATH}")
    get_filename_component(current_prefix "${abs_first_file}" DIRECTORY)

    # Iterate over the rest of the files and shorten the prefix if necessary
    foreach(file ${EXTERNAL_FILES})
        # Use BASE_PATH to resolve the absolute path for the current file
        get_filename_component(abs_file_path "${file}" ABSOLUTE BASE "${BASE_PATH}")
        
        # Keep shortening the prefix until it is indeed a prefix of the current file path
        while(NOT "${abs_file_path}" MATCHES "^${current_prefix}/")
            # If the current prefix is the root, stop (to prevent infinite loop)
            if("${current_prefix}" STREQUAL "/")
                break()
            endif()
            
            # Shorten the prefix by one directory level
            get_filename_component(current_prefix "${current_prefix}" DIRECTORY)
        endwhile()
    endforeach()
    
    # Set the result in the calling scope
    set(${OUT_VAR} "${current_prefix}" PARENT_SCOPE)
endfunction()



# @macro create_filters
# @brief Creates Visual Studio project filters (.vcxproj.filters) based on the source file directory structure.
# 
# This command iterates over all source files associated with a target and uses
# the source_group command to establish virtual folders. The virtual folders are
# organized under "Source Files", "Header Files", or "Template Files" based on 
# the file extension, while maintaining the relative path structure.
# 
# @param TARGET_NAME The name of the target (e.g., executable or library) to apply filters to.
macro(create_filters TARGET_NAME)

    # 1. Retrieve all source files associated with the target.
    get_target_property(ALL_FILES ${TARGET_NAME} SOURCES)

    # Initialize lists for separation
    set(SOURCE_FILES "")
    set(HEADER_FILES "")
    set(TEMPLATE_FILES "")
    
    # Initialize lists for external file separation
    set(EXT_SOURCE_FILES "")
    set(EXT_HEADER_FILES "")
    set(EXT_TEMPLATE_FILES "")
    
    # Define the starting point for relative path calculation.
    # We use the current directory to keep the filter paths local and clean.
    set(BASE_DIR "${CMAKE_CURRENT_SOURCE_DIR}")
	get_filename_component(absolute_base_dir "${BASE_DIR}" ABSOLUTE)

    # 2. Separate files into their respective categories based on extension.
    foreach(source_file ${ALL_FILES})
        
        # Check if file exists to prevent issues with non-yet-generated files
        #if(EXISTS "${source_file}")
	        get_filename_component(absolute_source_file "${source_file}" ABSOLUTE)
			if ("${absolute_source_file}" MATCHES "^${absolute_base_dir}/")
				# File is within BASE_DIR (Internal)
				set(DESTINATION_SOURCE_LIST SOURCE_FILES)
				set(DESTINATION_HEADER_LIST HEADER_FILES)
				set(DESTINATION_TEMPLATE_LIST TEMPLATE_FILES)
			else()
				# File is outside BASE_DIR (External)
				set(DESTINATION_SOURCE_LIST EXT_SOURCE_FILES)
				set(DESTINATION_HEADER_LIST EXT_HEADER_FILES)
				set(DESTINATION_TEMPLATE_LIST EXT_TEMPLATE_FILES)
			endif()
			
            get_filename_component(file_ext "${source_file}" EXT)
            string(TOLOWER "${file_ext}" file_ext_lower)
       
            # Files with no extension or common header extensions
            if(NOT file_ext OR file_ext_lower MATCHES "(\\.h|\\.hpp|\\.hxx)")
                list(APPEND ${DESTINATION_HEADER_LIST} "${source_file}")
            
            # Common source extensions
            elseif(file_ext_lower 
                MATCHES "(\\.c|\\.cpp|\\.cxx)")
                list(APPEND ${DESTINATION_SOURCE_LIST} "${source_file}")
            
            # Common template extensions
            elseif(file_ext_lower MATCHES "(\\.tpp|\\.txx)")
                list(APPEND ${DESTINATION_TEMPLATE_LIST} "${source_file}")
            endif()
        #endif()
  
    endforeach()

    # --- Start of the new logic for EXT_BASE_DIR ---
    # Collect all external files for common prefix calculation
    list(APPEND ALL_EXTERNAL_FILES ${EXT_SOURCE_FILES} ${EXT_HEADER_FILES} ${EXT_TEMPLATE_FILES})

    # Calculate the Longest Common Directory Prefix for external files, 
    # using BASE_DIR as the reference for relative paths.
    get_common_directory("${BASE_DIR}" EXT_BASE_DIR ${ALL_EXTERNAL_FILES})
    # --- End of the new logic for EXT_BASE_DIR ---
	
    # 3. Use source_group(TREE ...) for automatic, recursive filter creation.
    # message("XXXXXX ${TARGET_NAME}")
	# message("--- ${SOURCE_FILES} ---")
	# message("--- ${HEADER_FILES} ---")
	# message("--- ${TEMPLATE_FILES} ---")
	# message("")
	# message("--- ${EXT_SOURCE_FILES} ---")
	# message("--- ${EXT_HEADER_FILES} ---")
	# message("--- ${EXT_TEMPLATE_FILES} ---")
	# message("EXT_BASE_DIR: ${EXT_BASE_DIR}")
	# message("XXXXXX")
	# message("")
	
    if(SOURCE_FILES)
        source_group(TREE "${BASE_DIR}" PREFIX "Source Files" FILES ${SOURCE_FILES})
    endif()

    if(HEADER_FILES)
        source_group(TREE "${BASE_DIR}" PREFIX "Header Files" FILES ${HEADER_FILES})
    endif()

    if(TEMPLATE_FILES)
        source_group(TREE "${BASE_DIR}" PREFIX "Template Files" FILES ${TEMPLATE_FILES})
    endif()
	
	if(EXT_SOURCE_FILES)
        source_group(TREE "${EXT_BASE_DIR}" PREFIX "External/Source Files" FILES ${EXT_SOURCE_FILES})
    endif()

    if(EXT_HEADER_FILES)
        source_group(TREE "${EXT_BASE_DIR}" PREFIX "External/Header Files" FILES ${EXT_HEADER_FILES})
    endif()

    if(EXT_TEMPLATE_FILES)
        source_group(TREE "${EXT_BASE_DIR}" PREFIX "External/Template Files" FILES ${EXT_TEMPLATE_FILES})
    endif()
    
endmacro()