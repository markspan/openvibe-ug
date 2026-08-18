# ---------------------------------
# create CMake variables that holds
# - latest git commit hash
# - branch name
# ---------------------------------

# codename = the name of the current branch
EXECUTE_PROCESS(COMMAND git rev-parse --abbrev-ref HEAD
	WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
	OUTPUT_VARIABLE  PROJECT_BRANCH)
# command output may contain carriage return
STRING(REGEX REPLACE "\n" "" PROJECT_BRANCH "${PROJECT_BRANCH}")

# commithash = short hash of latest revision
EXECUTE_PROCESS(COMMAND git rev-parse --short HEAD
	WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
	OUTPUT_VARIABLE  PROJECT_COMMITHASH)
# command output may contain carriage return
STRING(REGEX REPLACE "\n" "" PROJECT_COMMITHASH "${PROJECT_COMMITHASH}")
