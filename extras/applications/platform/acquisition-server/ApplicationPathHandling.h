#pragma once

// SPDX - License - Identifier: AGPL - 3.0 - only
// SPDX-FileCopyrightText: Inria <florent.leray@inria.fr>

// Standard Library
#include <string>
#include <vector>
#include <iostream>
#include <algorithm>
#include <cstdlib> // For std::getenv
#include <filesystem>

// OS Specific Includes
#ifdef _WIN32
#include <windows.h>
#define DLL_EXTENSION ".dll"
#elif defined(__APPLE__)
#include <mach-o/dyld.h>
#define DLL_EXTENSION ".dylib"
#else
#include <unistd.h>
#include <limits.h>
#define DLL_EXTENSION ".so"
#endif

namespace fs =  std::filesystem;


/**
* @brief Custom string start check (Utility).
*/
bool startsWith(const std::string& fullString, const std::string& prefix);

/**
* @brief Retrieves the full absolute path of the current executable.
*/
fs::path getExecutablePath();

/**
* @brief Retrieves the directory containing the executable.
*/
fs::path getExecutableDir();

/**
* @brief Resolves the OS-specific user configuration directory (AppData/.config).
*/
void resolveOsDependentPaths(fs::path& userConfigBase, const fs::path& exeDir);

/**
* @brief Logic to detect if portable mode is active.
*/
bool detectPortableMode(const fs::path& exeDir);

/**
* @brief Finds files with a specific extension in a directory.
*/
std::vector<std::string> findFiles(const std::string& dirPath, const std::string& fileExt);

/**
* @brief Removes extension from a list of paths.
*/
std::vector<std::string> removeExt(const std::vector<std::string>& files, const std::string& fileExt);
