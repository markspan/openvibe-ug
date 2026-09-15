# ---------------------------------
# Computes the project version.
#
# The git tag is the source of truth. ov_compute_version() must be called from the root
# CMakeLists *before* project(), so it deliberately uses find_program()/execute_process()
# rather than find_package(Git), which is not safe to call before project().
#
# Exports, all in the caller's scope:
#   OPENVIBE_MAJOR_VERSION / OPENVIBE_MINOR_VERSION / OPENVIBE_PATCH_VERSION
#       Strictly numeric. These keep their historical names because ~110 project(... VERSION ...)
#       calls and CMake/launchers/resource-file.rc-base already interpolate them, and because
#       project(), SOVERSION, qt_add_qml_module() and the .rc FILEVERSION field all reject
#       anything that is not an integer.
#   OPENVIBE_VERSION_PRERELEASE   "beta.1", or empty for a release.
#   OPENVIBE_VERSION_FULL         Display and packaging string: "4.0.0", "4.0.0-beta.1",
#                                 or "4.0.0-42-gabc1234" when the commit is not tagged.
#   OPENVIBE_VERSION_SERIES       "4.0" -- for anything a user's data lives under, so that
#                                 patches and betas share it and only minor bumps separate.
#   OPENVIBE_VERSION_COMMITS      Commits since the tag; 0 exactly on a tag.
#   OPENVIBE_VERSION_HASH         Short commit hash, empty exactly on a tag.
#
# Resolution order, one entry per provenance the version can legitimately come from:
#   1. -DOPENVIBE_VERSION=...  explicit override; also the rescue lever when the rest has failed
#   2. CI_COMMIT_TAG           a GitLab release pipeline
#   3. git describe            a git checkout (local development, non-tag CI)
#   4. VERSION.txt             a source archive, substituted by git-archive's export-subst
#   5. hard error
# It never falls back to a placeholder: a silent 0.0.0 is how CI used to stamp the wrong version
# into users' saved scenarios.
# ---------------------------------

function(ov_compute_version)
	set(rawVersion "")
	set(versionSource "")

	# 1. Explicit override, for distribution packagers who repack the sources, and as the rescue
	#    lever when every other source has failed. Deliberately not an environment variable as
	#    well: an env var leaks silently from the shell into unrelated builds and is not recorded
	#    in CMakeCache.txt, so a stale one is invisible exactly where you would look for it.
	if(DEFINED OPENVIBE_VERSION AND NOT "${OPENVIBE_VERSION}" STREQUAL "")
		set(rawVersion "${OPENVIBE_VERSION}")
		set(versionSource "-DOPENVIBE_VERSION")
	endif()

	# 2. CI_COMMIT_TAG is set by GitLab on a tag pipeline. Taking the tag from the environment
	#    means a release build needs no git history at all -- a depth-1 clone of the tagged
	#    commit is enough, so the release pipeline does not pay for a full clone.
	if(rawVersion STREQUAL "" AND NOT "$ENV{CI_COMMIT_TAG}" STREQUAL "")
		set(rawVersion "$ENV{CI_COMMIT_TAG}")
		set(versionSource "CI_COMMIT_TAG")
	endif()

	# 3. git describe. --tags is required: every 3.x release tag in this repository is
	#    lightweight, and a bare describe considers annotated tags only -- which is how the
	#    version used to resolve to the 2018 tag 2.2.0. --match keeps non-release tags such as
	#    pre-dev-merge-qt6 out. --long forces the uniform TAG-N-gHASH shape so there is one
	#    parse path. Deliberately no --always (it yields a bare hash that cannot be parsed) and
	#    no --first-parent (it resolves to an older tag here).
	if(rawVersion STREQUAL "" AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.git")
		find_program(GIT_EXECUTABLE NAMES git)
		if(GIT_EXECUTABLE)
			execute_process(
				COMMAND "${GIT_EXECUTABLE}" describe --tags --long --match "[0-9]*.[0-9]*.[0-9]*"
				WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
				OUTPUT_VARIABLE gitDescribe
				ERROR_VARIABLE gitDescribeError
				RESULT_VARIABLE gitDescribeResult
				OUTPUT_STRIP_TRAILING_WHITESPACE)
			if(gitDescribeResult EQUAL 0 AND NOT gitDescribe STREQUAL "")
				set(rawVersion "${gitDescribe}")
				set(versionSource "git describe")
			endif()
		endif()
	endif()

	# 4. VERSION.txt, whose contents git-archive rewrites through the export-subst attribute set
	#    on it in .gitattributes. This is what lets someone building from a downloaded source
	#    archive get the right version without having to know, or guess, what it should be.
	#    In a real checkout it still holds the literal $Format:...$ placeholder, which is the
	#    signal that git above should have answered instead.
	if(rawVersion STREQUAL "" AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/VERSION.txt")
		file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VERSION.txt" versionFile)
		string(STRIP "${versionFile}" versionFile)
		if(NOT versionFile MATCHES "^\\$Format:")
			set(rawVersion "${versionFile}")
			set(versionSource "VERSION.txt")
		endif()
	endif()

	if(rawVersion STREQUAL "")
		message(FATAL_ERROR
			"Cannot determine the OpenViBE version.\n"
			"  No -DOPENVIBE_VERSION=<version> was given, CI_COMMIT_TAG is unset, 'git describe'"
			" produced nothing (${gitDescribeError}), and VERSION.txt is absent or unsubstituted.\n"
			"  Building from a git checkout? Ensure git is on PATH and that a release tag is"
			" reachable from HEAD. A shallow clone is the usual cause: 'git describe' cannot see a"
			" tag that the truncated history does not reach.\n"
			"  Building from a source archive? It should carry a substituted VERSION.txt; if it"
			" does not, the archive was not produced by git-archive. Pass -DOPENVIBE_VERSION=<version>.")
	endif()

	# Split off the describe suffix. The trailing anchors make the greedy (.+) stop in the right
	# place, so a prerelease that itself contains hyphens survives.
	set(commitCount 0)
	set(commitHash "")
	if(rawVersion MATCHES "^(.+)-([0-9]+)-g([0-9a-fA-F]+)$")
		set(versionTag "${CMAKE_MATCH_1}")
		set(commitCount "${CMAKE_MATCH_2}")
		set(commitHash "${CMAKE_MATCH_3}")
	else()
		set(versionTag "${rawVersion}")
	endif()

	if(NOT versionTag MATCHES "^([0-9]+)\\.([0-9]+)\\.([0-9]+)(-(.+))?$")
		message(FATAL_ERROR
			"OpenViBE version '${versionTag}' (from ${versionSource}, raw '${rawVersion}') is not"
			" of the form MAJOR.MINOR.PATCH[-PRERELEASE].")
	endif()
	set(major "${CMAKE_MATCH_1}")
	set(minor "${CMAKE_MATCH_2}")
	set(patch "${CMAKE_MATCH_3}")
	set(prerelease "${CMAKE_MATCH_5}")

	# Guard against the tag-hijack hazard described in CLAUDE.md. sdk/, designer/ and extras/
	# were grafted in from separate repositories whose own 1.x/2.x tags remain reachable from
	# HEAD; if one of those is ever annotated or fetched again it would silently become the
	# describe anchor. Anything below the monorepo era is a mistake, so say so loudly instead of
	# quietly building a wrong version.
	if(major LESS 3)
		message(FATAL_ERROR
			"OpenViBE version resolved to '${rawVersion}' (from ${versionSource}), whose major"
			" version ${major} predates the monorepo. This normally means 'git describe' anchored"
			" on a tag imported from the archived sdk/designer/extras repositories. Check"
			" 'git tag --list' and the remotes, or pass -DOPENVIBE_VERSION=<version>.")
	endif()

	set(fullVersion "${major}.${minor}.${patch}")
	if(NOT prerelease STREQUAL "")
		set(fullVersion "${fullVersion}-${prerelease}")
	endif()
	if(commitCount GREATER 0)
		set(fullVersion "${fullVersion}-${commitCount}-g${commitHash}")
	endif()

	set(OPENVIBE_MAJOR_VERSION      "${major}"              PARENT_SCOPE)
	set(OPENVIBE_MINOR_VERSION      "${minor}"              PARENT_SCOPE)
	set(OPENVIBE_PATCH_VERSION      "${patch}"              PARENT_SCOPE)
	set(OPENVIBE_VERSION_PRERELEASE "${prerelease}"         PARENT_SCOPE)
	set(OPENVIBE_VERSION_FULL       "${fullVersion}"        PARENT_SCOPE)
	set(OPENVIBE_VERSION_SERIES     "${major}.${minor}"     PARENT_SCOPE)
	set(OPENVIBE_VERSION_COMMITS    "${commitCount}"        PARENT_SCOPE)
	set(OPENVIBE_VERSION_HASH       "${commitHash}"         PARENT_SCOPE)
	set(OPENVIBE_VERSION_SOURCE     "${versionSource}"      PARENT_SCOPE)
endfunction()
