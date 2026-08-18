

# Macro to detect the target architecture based on the Generator Platform.
# This is specifically robust for Visual Studio (checking the -A argument).
macro(DetectTargetArchitecture)

    # Initialize flags
    set(IS_X86_64_ARCH FALSE)
    set(IS_X86_32_ARCH FALSE)
    set(IS_ARM_64_ARCH FALSE)
    set(IS_ARM_32_ARCH FALSE)

    # Retrieve the Generator Platform (e.g., "x64", "Win32", "ARM64").
    # This variable is set by the -A option in CMake for VS Generators.
    set(TARGET_PLATFORM "${CMAKE_GENERATOR_PLATFORM}")
    
    # If CMAKE_GENERATOR_PLATFORM is empty, CMake might be using the default.
    # For Visual Studio 2019+, the default depends on the host, but usually x64.
    # However, rely on CMAKE_VS_PLATFORM_NAME if available as a fallback.
    if("${TARGET_PLATFORM}" STREQUAL "" AND DEFINED CMAKE_VS_PLATFORM_NAME)
        set(TARGET_PLATFORM "${CMAKE_VS_PLATFORM_NAME}")
    endif()

    # Normalize to lowercase
    string(TOLOWER "${TARGET_PLATFORM}" TARGET_PLATFORM_LOWER)

    # 1. Visual Studio specific detection logic
    if(MSVC OR CMAKE_GENERATOR MATCHES "Visual Studio")
    
        if(TARGET_PLATFORM_LOWER MATCHES "x64")
            set(IS_X86_64_ARCH TRUE)
            
        elseif(TARGET_PLATFORM_LOWER MATCHES "win32")
            # "Win32" in VS generator context always means x86 32-bit
            set(IS_X86_32_ARCH TRUE)
            
        elseif(TARGET_PLATFORM_LOWER MATCHES "arm64")
            set(IS_ARM_64_ARCH TRUE)
            
        elseif(TARGET_PLATFORM_LOWER MATCHES "arm")
            set(IS_ARM_32_ARCH TRUE)
        endif()

    # 2. Fallback for other Generators (Ninja, Makefiles) using pointer size
    # Since we avoid CMAKE_SYSTEM_PROCESSOR, we use pointer size for bitness check.
    # Note: Distinguishing x64 from ARM64 without CMAKE_SYSTEM_PROCESSOR or compiler defines
    # is tricky in pure CMake without compiling a test program.
    else()
    
        # Use CMAKE_SIZEOF_VOID_P as a hard fact about the target compiled code.
        if(CMAKE_SIZEOF_VOID_P EQUAL 8)
            # It is 64-bit. Now, is it x64 or ARM64?
            # We can try checking CMAKE_CXX_COMPILER_ARCHITECTURE_ID (Newer CMake)
            if(CMAKE_CXX_COMPILER_ARCHITECTURE_ID MATCHES "ARM64|aarch64")
                set(IS_ARM_64_ARCH TRUE)
            else()
                # Default assumption for 64-bit if not ARM (likely x86_64)
                set(IS_X86_64_ARCH TRUE)
            endif()
            
        elseif(CMAKE_SIZEOF_VOID_P EQUAL 4)
            # It is 32-bit.
            if(CMAKE_CXX_COMPILER_ARCHITECTURE_ID MATCHES "ARM")
                set(IS_ARM_32_ARCH TRUE)
            else()
                # Default assumption for 32-bit (likely x86)
                set(IS_X86_32_ARCH TRUE)
            endif()
        endif()
        
    endif()

endmacro()


# Macro to determine the base output directory structure
# based on the detected architecture and the build configuration.
#
# Requires: DetectTargetArchitecture() to be called beforehand.
# Sets: OUTPUT_BASE variable.
macro(SetOutputBaseVariable)

    # 0.Detect target architecture
    DetectTargetArchitecture()
    
    # 1. Determine the Architecture Folder Name
    # We rely on the boolean flags set by the previous architecture detection macro.
    if(IS_X86_64_ARCH)
        set(ARCH_FOLDER_NAME "x64")
    elseif(IS_X86_32_ARCH)
        set(ARCH_FOLDER_NAME "x86")
    elseif(IS_ARM_64_ARCH)
        set(ARCH_FOLDER_NAME "arm64")
    elseif(IS_ARM_32_ARCH)
        set(ARCH_FOLDER_NAME "arm")
    else()
        # Fallback if detection failed or is unknown
        set(ARCH_FOLDER_NAME "unknown_arch")
    endif()

    # 2. Determine the Configuration Folder Name
    get_property(GENERATOR_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)

    if(${GENERATOR_MULTI_CONFIG})
        set(CONFIG_FOLDER_NAME "$<CONFIG>")
    else()
        if(NOT CMAKE_BUILD_TYPE)
            set(CMAKE_BUILD_TYPE "Debug")
        endif()
        set(CONFIG_FOLDER_NAME "${CMAKE_BUILD_TYPE}")
    endif()

    # 3. Assemble the Output Base Path
    # Structure: [BuildDir]/[Arch]/[Config]
    # Example VS:    C:/Project/build/x64/$<CONFIG>
    # Example Ninja: C:/Project/build/x64/Release
    set(OUTPUT_BASE "${CMAKE_BINARY_DIR}/${ARCH_FOLDER_NAME}/${CONFIG_FOLDER_NAME}")

endmacro()




macro(setup_output_paths_generic target dest sub)
get_property(GENERATOR_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)


if(${GENERATOR_MULTI_CONFIG})
  set_target_properties( ${target} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG          ${dest}/bin/${sub})
  set_target_properties( ${target} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE        ${dest}/bin/${sub})
  set_target_properties( ${target} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL     ${dest}/bin/${sub})
  set_target_properties( ${target} PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${dest}/bin/${sub})

  set_target_properties( ${target} PROPERTIES LIBRARY_OUTPUT_DIRECTORY_DEBUG          ${dest}/bin/${sub})
  set_target_properties( ${target} PROPERTIES LIBRARY_OUTPUT_DIRECTORY_RELEASE        ${dest}/bin/${sub})
  set_target_properties( ${target} PROPERTIES LIBRARY_OUTPUT_DIRECTORY_MINSIZEREL     ${dest}/bin/${sub})
  set_target_properties( ${target} PROPERTIES LIBRARY_OUTPUT_DIRECTORY_RELWITHDEBINFO ${dest}/bin/${sub})

  set_target_properties( ${target} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_DEBUG          ${dest}/lib/${sub})
  set_target_properties( ${target} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_RELEASE        ${dest}/lib/${sub})
  set_target_properties( ${target} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_MINSIZEREL     ${dest}/lib/${sub})
  set_target_properties( ${target} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY_RELWITHDEBINFO ${dest}/lib/${sub})
else()
  set_target_properties( ${target} PROPERTIES RUNTIME_OUTPUT_DIRECTORY                ${dest}/bin/${sub})
  set_target_properties( ${target} PROPERTIES LIBRARY_OUTPUT_DIRECTORY                ${dest}/bin/${sub})
  set_target_properties( ${target} PROPERTIES ARCHIVE_OUTPUT_DIRECTORY                ${dest}/lib/${sub})
endif()
  
#message("    ---------------> Installing target ${target} to ${dest} with subdir ${sub}")
install(TARGETS ${target}
  #EXPORT ${TARGET_NAME}Targets
  RUNTIME       DESTINATION ./bin/${sub}
  LIBRARY       DESTINATION ./bin/${sub}
  ARCHIVE       DESTINATION ./lib/${sub}
  PUBLIC_HEADER DESTINATION ./include/${sub}
)
endmacro()


################################################################################
#
# Usage: set_plugin_install_rules(target)
# set rules for the plugin designed by the target
#
################################################################################
macro(setup_output_paths_general target)
  setup_output_paths_generic(${target} ${CMAKE_BINARY_DIR} "") #In good future replace CMAKE_BINARY_DIR by OUTPUT_BASE
endmacro()

################################################################################
#
# Usage: setup_output_paths_services(target)
# set rules for the plugin legacy designed by the target
#
################################################################################
macro(setup_output_paths_services target)
  setup_output_paths_generic(${target} ${CMAKE_BINARY_DIR} "services") #In good future replace CMAKE_BINARY_DIR by OUTPUT_BASE
endmacro()

################################################################################
#
# Usage: setup_output_paths_drivers(target)
# set rules for the plugin legacy designed by the target
#
################################################################################
macro(setup_output_paths_drivers target)
  setup_output_paths_generic(${target} ${CMAKE_BINARY_DIR} "drivers") #In good future replace CMAKE_BINARY_DIR by OUTPUT_BASE
endmacro()

################################################################################
#
# Usage: setup_output_paths_writers(target)
# set rules for the plugin legacy designed by the target
#
################################################################################
macro(setup_output_paths_writers target)
  setup_output_paths_generic(${target} ${CMAKE_BINARY_DIR} "writers") #In good future replace CMAKE_BINARY_DIR by OUTPUT_BASE
endmacro()

