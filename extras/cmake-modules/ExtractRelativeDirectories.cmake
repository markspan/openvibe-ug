#=============================================================================
# @brief Extracts a list of unique, relative directory paths from a list of 
#        relative file paths.
#
# This macro iterates over a list of relative file paths and determines the 
# directory of each file. It then compiles a new list containing only the 
# unique directory paths. This is useful, for example, for defining 
# include directories based on the location of source files.
#
# @param FILE_LIST_VAR The input variable name containing the list of relative 
#                      file paths (e.g., source files).
# @param DIR_LIST_VAR The output variable name to store the resulting list of 
#                     unique, relative directory paths. This variable will be 
#                     created or overwritten.
# @example
# # Assuming 'SOURCE_FILES' is: 
# # "src/file_a.cpp" "src/sub/file_b.cpp" "tests/test_a.cpp"
# #
# # extract_relative_directories(SOURCE_FILES UNIQUE_DIRS)
# #
# # 'UNIQUE_DIRS' will be: 
# # "src" "src/sub" "tests"
macro(extract_relative_directories FILE_LIST_VAR DIR_LIST_VAR)

    # Initialize the output list
    set(${DIR_LIST_VAR})

    foreach(current_file ${${FILE_LIST_VAR}})
        get_filename_component(current_dir ${current_file} DIRECTORY)
		if(NOT "${current_file}" STREQUAL "" AND "${current_dir}" STREQUAL "")
             set(current_dir ".")
        endif()
        list(APPEND ${DIR_LIST_VAR} ${current_dir})
    endforeach()
	
    list(REMOVE_DUPLICATES ${DIR_LIST_VAR})

    # Clean up local variable to prevent pollution in the calling scope
    #unset(current_file)
    unset(current_dir)

endmacro()
