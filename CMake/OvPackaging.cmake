# SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
# SPDX-License-Identifier: AGPL-3.0-only

# Packaging for OpenViBE.
#
# Build a package with:   cpack --config <build dir>/CPackConfig.cmake
#
# This module must be included *last* from the top-level CMakeLists.txt: include(CPack) snapshots the
# CPACK_* variables, so anything set after it is ignored.

set(CPACK_PACKAGE_NAME "OpenViBE")
set(CPACK_PACKAGE_VENDOR "Inria")
set(CPACK_PACKAGE_VERSION ${PROJECT_VERSION})
set(CPACK_PACKAGE_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Software platform for brain-computer interfaces and real-time neurosciences")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://openvibe.inria.fr")
set(CPACK_PACKAGE_FILE_NAME "openvibe-${PROJECT_VERSION}-${BUILD_ARCH}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "openvibe-${PROJECT_VERSION}")

# One package containing the whole install tree. The build defines no install components, so this is
# also what CPack would do by default; stated explicitly so that adding a COMPONENT somewhere cannot
# silently turn the installer into a component-selection wizard.
set(CPACK_MONOLITHIC_INSTALL ON)

# Quote CPACK_* values when generating the config, so paths containing spaces survive.
set(CPACK_VERBATIM_VARIABLES ON)

# The default build type is RelWithDebInfo; stripping would throw away the debug info on purpose put there.
set(CPACK_STRIP_FILES OFF)

# CPack picks the license page renderer from the file extension, and COPYING has none.
configure_file(${CMAKE_SOURCE_DIR}/COPYING ${CMAKE_BINARY_DIR}/COPYING.txt COPYONLY)
set(CPACK_RESOURCE_FILE_LICENSE ${CMAKE_BINARY_DIR}/COPYING.txt)

if(WIN32)
	set(CPACK_GENERATOR NSIS ZIP)
else()
	set(CPACK_GENERATOR TGZ)
endif()

# #############################################################################
# Windows NSIS installer
# #############################################################################

if(WIN32)
	set(CPACK_NSIS_PACKAGE_NAME "OpenViBE ${PROJECT_VERSION}")
	set(CPACK_NSIS_DISPLAY_NAME "OpenViBE ${PROJECT_VERSION}")

	if(BUILD_ARCH STREQUAL "x64")
		set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES64")
	else()
		set(CPACK_NSIS_INSTALL_ROOT "$PROGRAMFILES")
	endif()

	# The old script hand-rolled this via a HKLM InstallDir probe in .onInit followed by RMDir /r.
	set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)

	set(CPACK_NSIS_MODIFY_PATH OFF)
	set(CPACK_NSIS_HELP_LINK "https://openvibe.inria.fr")
	set(CPACK_NSIS_URL_INFO_ABOUT "https://openvibe.inria.fr")
	set(CPACK_NSIS_CONTACT "openvibe-contact@inria.fr")

	# Branding. Without an explicit icon the wizard uses the stock NSIS one, and without
	# MANIFEST_DPI_AWARE it is bitmap-scaled and blurry on a scaled display.
	# Checked here because CPack does not validate the path: a missing icon silently yields the stock
	# one, or a makensis failure minutes into packaging.
	set(OV_INSTALLER_ICON "${OV_ICON_FILE}")
	if(NOT EXISTS "${OV_INSTALLER_ICON}")
		message(FATAL_ERROR "Installer icon not found: ${OV_INSTALLER_ICON}")
	endif()
	set(CPACK_NSIS_MUI_ICON "${OV_INSTALLER_ICON}")
	set(CPACK_NSIS_MUI_UNIICON "${OV_INSTALLER_ICON}")
	set(CPACK_NSIS_BRANDING_TEXT "OpenViBE ${PROJECT_VERSION} - Inria")
	set(CPACK_NSIS_MANIFEST_DPI_AWARE ON)

	# Wizard header bitmap (MUI_HEADERIMAGE_BITMAP). Must be a BMP at the MUI-standard 150x57, and must
	# be 24bpp: NSIS renders an alpha channel unreliably, which shows up as a black or fringed header.
	# Right-aligned artwork, because MUI draws the page title over the left of this bitmap.
	set(OV_INSTALLER_HEADER "${CMAKE_CURRENT_LIST_DIR}/packaging/nsis-header.bmp")
	if(NOT EXISTS "${OV_INSTALLER_HEADER}")
		message(FATAL_ERROR "Installer header bitmap not found: ${OV_INSTALLER_HEADER}")
	endif()
	# Unlike MUI_ICON, this path reaches NSIS's File command, which rejects forward slashes outright
	# ("no files found") and fails the build. Hence native separators, doubled so they survive being
	# re-parsed out of the generated CPackConfig.cmake.
	file(TO_NATIVE_PATH "${OV_INSTALLER_HEADER}" OV_INSTALLER_HEADER_NATIVE)
	string(REPLACE "\\" "\\\\" OV_INSTALLER_HEADER_NATIVE "${OV_INSTALLER_HEADER_NATIVE}")
	set(CPACK_PACKAGE_ICON "${OV_INSTALLER_HEADER_NATIVE}")

	if(TARGET designer)
		set(CPACK_NSIS_INSTALLED_ICON_NAME "bin\\\\designer.exe")
		# CPack prefixes CPACK_NSIS_EXECUTABLES_DIRECTORY (default "bin") but, unlike
		# CPACK_PACKAGE_EXECUTABLES, does not append ".exe" -- so spell it out.
		set(CPACK_NSIS_MUI_FINISHPAGE_RUN "designer.exe")
	else()
		message(FATAL_ERROR "No target designer found!")
	endif()

	# Start-Menu entries, as <basename under bin/> then <label>; CPack appends ".exe" itself.
	# Deliberately flat and short: nested Start-Menu folders are a Windows 7 idiom, and the developer
	# tools are terminal utilities that ship in bin/ without a shortcut.
	# Guarded on the target existing, since conditional builds (e.g. OV_DISABLE_GTK) drop whole apps
	# and a shortcut to a file that was never installed is worse than no shortcut.
	set(CPACK_PACKAGE_EXECUTABLES "")
	if(TARGET designer)
		list(APPEND CPACK_PACKAGE_EXECUTABLES "designer" "OpenViBE Designer")
	else()
		message(STATUS "Packaging: no Start-Menu shortcut for 'designer', target not built")
	endif()
	if(TARGET openvibe-acquisition-server)
		list(APPEND CPACK_PACKAGE_EXECUTABLES "openvibe-acquisition-server" "OpenViBE Acquisition Server")
	else()
		message(STATUS "Packaging: no Start-Menu shortcut for 'openvibe-acquisition-server', target not built")
	endif()
endif()

include(CPack)
