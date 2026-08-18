///-------------------------------------------------------------------------------------------------
/// 
/// \file defines.hpp
/// \copyright Copyright (C) 2022 Inria
///
/// This program is free software: you can redistribute it and/or modify
/// it under the terms of the GNU Affero General Public License as published
/// by the Free Software Foundation, either version 3 of the License, or
/// (at your option) any later version.
///
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU Affero General Public License for more details.
///
/// You should have received a copy of the GNU Affero General Public License
/// along with this program.  If not, see <https://www.gnu.org/licenses/>.
/// 
///-------------------------------------------------------------------------------------------------

#pragma once

//___________________________________________________________________//
//                                                                   //
// API Definition                                                    //
//___________________________________________________________________//
//                                                                   //

// Taken from
// - http://people.redhat.com/drepper/dsohowto.pdf
// - http://www.nedprod.com/programs/gccvisibility.html
#if defined LMAV_Shared
#if defined TARGET_OS_Windows
#define LMAV_API_Export __declspec(dllexport)
#define LMAV_API_Import __declspec(dllimport)
#elif defined TARGET_OS_Linux
#define LMAV_API_Export __attribute__((visibility("default")))
#define LMAV_API_Import __attribute__((visibility("default")))
#else
#define LMAV_API_Export
#define LMAV_API_Import
#endif
#else
#define LMAV_API_Export
#define LMAV_API_Import
#endif

#if defined LMAV_Exports
#define LMAV_API LMAV_API_Export
#else
#define LMAV_API LMAV_API_Import
#endif
