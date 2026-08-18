#pragma once

#include <ov_common_defines.h>

#if defined FS_Shared
#   if defined TARGET_OS_Windows
#       define FS_API_Export __declspec(dllexport)
#       define FS_API_Import __declspec(dllimport)
#   elif defined TARGET_OS_Linux || defined TARGET_OS_MacOS
#       define FS_API_Export __attribute__((visibility("default")))
#       define FS_API_Import __attribute__((visibility("default")))
#   else
#       define FS_API_Export
#       define FS_API_Import
#   endif
#else
#   define FS_API_Export
#   define FS_API_Import
#endif

#if defined FS_Exports
#   define FS_API FS_API_Export
#else
#   define FS_API FS_API_Import
#endif
