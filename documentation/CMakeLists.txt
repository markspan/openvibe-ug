message(STATUS "***** Begin META Documentation *****")

# look for doxygen, if not present, no need to generate documentation
FIND_PROGRAM(doxygen_bin "doxygen" PATHS $ENV{CONDA_PREFIX}/bin $ENV{CONDA_PREFIX}/Library/bin NO_DEFAULT_PATH)

if(!doxygen_bin)
	message(STATUS "  FAILED to find doxygen...")
else(!doxygen_bin)
	message(STATUS "  Found doxygen...")
	set(ROOT_DOC_DIR "${CMAKE_SOURCE_DIR}/build/documentation")
	message(STATUS "    Root Folder for Documentation : ${ROOT_DOC_DIR}")

	#----- create folder to put the output from doxygen to -----
	file(MAKE_DIRECTORY "${ROOT_DOC_DIR}")
	file(MAKE_DIRECTORY "${ROOT_DOC_DIR}/html")
	file(MAKE_DIRECTORY "${ROOT_DOC_DIR}/algorithm-doc")
	file(MAKE_DIRECTORY "${ROOT_DOC_DIR}/algorithm-snapshots")
	file(MAKE_DIRECTORY "${ROOT_DOC_DIR}/box-algorithm-doc")
	file(MAKE_DIRECTORY "${ROOT_DOC_DIR}/box-algorithm-snapshots")
	file(MAKE_DIRECTORY "${ROOT_DOC_DIR}/dox")

	message(STATUS "    Folders for Documentation created")
	#----- Run Plugin inspector -----

	set(ENV{PYTHONHOME} $ENV{CONDA_PREFIX})

	if(UNIX)
		execute_process(
			COMMAND  ${CMAKE_INSTALL_FULL_BINDIR}/openvibe-plugin-inspector 
				-d "${ROOT_DOC_DIR}" -p ${CMAKE_INSTALL_FULL_BINDIR}/../lib/ 
				-k "${CMAKE_INSTALL_FULL_BINDIR}/../lib/" 
				-c "${CMAKE_INSTALL_FULL_BINDIR}/../share/openvibe/kernel/openvibe.conf" 
			WORKING_DIRECTORY "${CMAKE_INSTALL_FULL_BINDIR}/"
		)
	elseif(WIN32)
		execute_process(
			COMMAND	${CMAKE_INSTALL_FULL_BINDIR}/openvibe-plugin-inspector.exe
				-d "${ROOT_DOC_DIR}" -p ${CMAKE_INSTALL_FULL_BINDIR}
				-k "${CMAKE_INSTALL_FULL_BINDIR}"
				-c "${CMAKE_INSTALL_FULL_BINDIR}/../share/openvibe/kernel/openvibe.conf" 
				--no-pause
			WORKING_DIRECTORY "${CMAKE_INSTALL_FULL_BINDIR}/"
		)
	endif(UNIX)
	
	message(STATUS "    Plugin inspector Finished")

	#----- Find All dox-part -----
	set(FILES "")
	get_property(CURRENT_PROJECTS GLOBAL PROPERTY OV_PROP_CURRENT_PROJECTS)
	#message(STATUS "    Current Projects : ${CURRENT_PROJECTS}")
	foreach(project ${CURRENT_PROJECTS})
		if(EXISTS "${project}/doc")
			file(GLOB_RECURSE doxs "${project}/*.dox-part")
			set(FILES "${FILES};${doxs}")
		endif(EXISTS "${project}/doc")
	endforeach(project)
	message(STATUS "    FILES : ${FILES}")

	#----- Parsing Dox -----
	foreach(file ${FILES})
		message(STATUS "             Documentation part found ${file}")
		set(dox_tag_name NOTFOUND)
		# iterates on each line of the file to look after begin/end tags "dox_tag_name" stores the name of the variable
		# to use to configure the skeleton file. It is computed from the begin tag.
		file(READ ${file} dox_lines)

		# replaces empty cariage returns with semi colons to be compliant with CMake lists. 
		# note the space before and after the semi colon, this is for CMake not to skip empty lines
		string(REPLACE "\n" " ; " dox_lines " ${dox_lines} ")
		# Replace square brackets by a string key. Square brackets can potentially break the foreach bellow.
		string(REPLACE "[" "SQUARE_BRACKET_OPEN" dox_lines " ${dox_lines} ")
		string(REPLACE "]" "SQUARE_BRACKET_CLOSE" dox_lines " ${dox_lines} ")
		foreach(dox_line ${dox_lines})

			# this regex removes the spaces we added before the loop
			string(REGEX REPLACE "^ (.*) $" "\\1" dox_line ${dox_line})
			# Restore the square brackets in the string if any
			string(REPLACE "SQUARE_BRACKET_OPEN" "[" dox_line " ${dox_line} ")
			string(REPLACE "SQUARE_BRACKET_CLOSE" "]" dox_line " ${dox_line} ")
			# we initialize several variables that will be used in this loop
			set(dox_line_processed   FALSE)
			set(dox_tag_begin NOTFOUND)
			set(dox_tag_end   NOTFOUND)
			set(dox_tag       NOTFOUND)

			# and look for a new tag in this line
			string(REGEX MATCH "\\|[a-zA-Z0-9_]+\\|" dox_tag "${dox_line}")
			if(dox_tag)
				# a tag is found, so we want to know if it is a OVP_DocBegin* or OVP_DocEnd* tag
				string(REGEX MATCH "\\|OVP_DocBegin_[a-zA-Z0-9_]*\\|" dox_tag_begin "${dox_line}")
				string(REGEX MATCH "\\|OVP_DocEnd_[a-zA-Z0-9_]*\\|"   dox_tag_end   "${dox_line}")

				# in case we already have something in dox_tag_name, it means that begin tag has already been processed, 
				# so either we terminate with end tag, either we continue with come content to add in the variable
				if(dox_tag_name AND dox_tag_end)
					# in case we find end tag, we just terminate cleaning the tag and what follows. 
					# We then terminate and create a new CMake variable with the content of this begin/end tagged things.
					string(REGEX REPLACE ".*\\|OVP_DocEnd_([a-zA-Z0-9_]*)\\|.*" "\\1" dox_tag_name_check ${dox_line})
					string(REGEX REPLACE   "\\|OVP_DocEnd_([a-zA-Z0-9_]*)\\|.*" "" dox_line "${dox_line}")
					#message(STATUS "             - Completed tag pair |${dox_tag_name}|")
					set(dox_tag_name_value "${dox_tag_name_value}\n${dox_line}")
					set("Doc_${dox_tag_name}_Content" ${dox_tag_name_value})
					set(dox_tag_name NOTFOUND)
					set(dox_line_processed TRUE)
				endif(dox_tag_name AND dox_tag_end)

				# in case dox_tag_name is empty, it means that begin tag has not yet been found, so we just look at it or skip to next line
				if(NOT dox_tag_name AND dox_tag_begin)
					# in case we find begin tag, we just start saving the CMake variable name, and clean the tag and what comes before. 
					# We then intialize the content of the begin/end tagged thing with what comes after begin tag.
					string(REGEX REPLACE ".*\\|OVP_DocBegin_([a-zA-Z0-9_]*)\\|.*" "\\1" dox_tag_name ${dox_line})
					string(REGEX REPLACE ".*\\|OVP_DocBegin_([a-zA-Z0-9_]*)\\|" "" dox_line "${dox_line}")
					set(dox_tag_name_value "${dox_line}")
					set(dox_line_processed TRUE)
				endif(NOT dox_tag_name AND dox_tag_begin)

				# in case dox tag is not OVP_DocBegin* or OVP_DocEnd* just print a warning and continue
				if(NOT dox_line_processed)
					message(STATUS "             - Unexpected tag ${dox_tag} in ${file} will be ignored")
				endif(NOT dox_line_processed)
			endif(dox_tag)

			# in case this line was not processed, either because it does not have any tag, either because the tag was unexpected, 
			# we just append the whole line to the content of the current variable
			if(dox_tag_name AND NOT dox_line_processed)
				# in case we don't find the end tag, just append this new line to the current content
				set(dox_tag_name_value "${dox_tag_name_value}\n${dox_line}")
			endif(dox_tag_name AND NOT dox_line_processed)
		endforeach(dox_line)
	endforeach(file)
	# Install the remaining resource files
	# now we have stored all the begin/end tagged things in variable, we just have to configure the skeleton configuration files with those variables.
	# note that the skeleton files should be prepared to receive the CMake variables with anywhere it is needed.
	# in order to do so, we look after all the (.dox-skeleton) files and call the configure command to build the final documentation (.dox) file.
	FILE(GLOB_RECURSE dox_skeletons "${ROOT_DOC_DIR}/*.dox-skeleton")
	FOREACH(skeleton ${dox_skeletons})
		GET_FILENAME_COMPONENT(skeleton_filename ${skeleton} NAME_WE)
		CONFIGURE_FILE("${skeleton}" "${ROOT_DOC_DIR}/dox/${skeleton_filename}.dox" @ONLY)
	ENDFOREACH(skeleton)

	configure_file("${ROOT_DOC_DIR}/box-algorithm-doc/Doc_BoxAlgorithms.dox" "${ROOT_DOC_DIR}/dox/Doc_BoxAlgorithms.dox" COPYONLY)

	#----- Config & Run doxygen -----
	CONFIGURE_FILE("${CMAKE_SOURCE_DIR}/documentation/Doxyfile-skeleton" "${CMAKE_SOURCE_DIR}/documentation/Doxyfile" @ONLY)
	CONFIGURE_FILE("${CMAKE_SOURCE_DIR}/documentation/Main-skeleton.md" "${CMAKE_SOURCE_DIR}/documentation/Main.md" @ONLY)
	MESSAGE(STATUS "Running doxygen: ${doxygen_bin}")
	EXECUTE_PROCESS(
		COMMAND ${doxygen_bin} "${CMAKE_SOURCE_DIR}/documentation/Doxyfile"
		WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/documentation/"
	)


endif(!doxygen_bin)


