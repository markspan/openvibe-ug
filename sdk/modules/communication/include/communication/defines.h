#pragma once

#include <ov_common_defines.h>

#if defined Communication_Shared
#   if defined TARGET_OS_Windows
#       define Communication_API_Export __declspec(dllexport)
#       define Communication_API_Import __declspec(dllimport)
#   elif defined TARGET_OS_Linux || defined TARGET_OS_MacOS
#       define Communication_API_Export __attribute__((visibility("default")))
#       define Communication_API_Import __attribute__((visibility("default")))
#   else
#       define Communication_API_Export
#       define Communication_API_Import
#   endif
#else
#   define Communication_API_Export
#   define Communication_API_Import
#endif

#if defined Communication_Exports
#   define Communication_API Communication_API_Export
#else
#   define Communication_API Communication_API_Import
#endif
