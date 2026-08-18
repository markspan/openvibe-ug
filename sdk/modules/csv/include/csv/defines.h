#pragma once

#if defined CSV_Shared
#   if defined TARGET_OS_Windows
#       define CSV_API_Export __declspec(dllexport)
#       define CSV_API_Import __declspec(dllimport)
#   elif defined TARGET_OS_Linux || defined TARGET_OS_MacOS
#       define CSV_API_Export __attribute__((visibility("default")))
#       define CSV_API_Import __attribute__((visibility("default")))
#   else
#       define CSV_API_Export
#       define CSV_API_Import
#   endif
#else
#   define CSV_API_Export
#   define CSV_API_Import
#endif

#if defined CSV_Exports
#   define CSV_API CSV_API_Export
#else
#   define CSV_API CSV_API_Import
#endif
