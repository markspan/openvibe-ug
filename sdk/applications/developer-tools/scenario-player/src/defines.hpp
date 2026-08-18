///-------------------------------------------------------------------------------------------------
/// 
/// \file defines.hpp
/// \author Charles Garraud.
/// \version 1.0.
/// \date 25/01/2016.
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

#include "TProgramOptions.hpp"

namespace OpenViBE {
/**
* \defgroup ScenarioPlayer Scenario Player
*/

/// <summary> Scenario player list of potential return code. </summary>
/// \ingroup ScenarioPlayer
enum class EPlayerReturnCodes
{
    Success = 0,                ///< No error during execution
    InvalidArg,                 ///< Invalid command-line options
    MissingMandatoryArgument,   ///< A mandatory argument is missing
    BadArg,                     ///< An argument is given with a wrong value
    OpeningFileFailure,         ///< A file could not be opened
    ParsingCommandFailure,      ///< General parsing command error
    UnkownFailure,              ///< Error of unknown type
    KernelLoadingFailure,       ///< Kernel loading failed
    KernelInvalidDesc,          ///< Invalid kernel descriptor
    KernelInvalidContext,       ///< Invalid kernel context
    KernelInternalFailure,      ///< Generic error type for kernel internal error
    ScenarioNotLoaded           ///< Error triggered when an action is requested on an unknown scenario
};

/// <summary> Way of playing a scenario. </summary>
/// \ingroup ScenarioPlayer
enum class EPlayerPlayMode { Standard = 0, Fastfoward };

// Define the common parser to be used in the application
using ProgramOptionParser = ProgramOptions<SProgramOptionsTraits::String, SProgramOptionsTraits::Float, SProgramOptionsTraits::TokenPairList>;

inline std::string toString(const EPlayerReturnCodes code)
{
    switch (code) {
        case EPlayerReturnCodes::Success: return "Success";
        case EPlayerReturnCodes::InvalidArg: return "Invalid Arg";
        case EPlayerReturnCodes::MissingMandatoryArgument: return "Missing Mandatory Argument";
        case EPlayerReturnCodes::BadArg: return "Bad Arg";
        case EPlayerReturnCodes::OpeningFileFailure: return "Opening File Failure";
        case EPlayerReturnCodes::ParsingCommandFailure: return "Parsing Command Failure";
        case EPlayerReturnCodes::UnkownFailure: return "Unkown Failure";
        case EPlayerReturnCodes::KernelLoadingFailure: return "Kernel Loading Failure";
        case EPlayerReturnCodes::KernelInvalidDesc: return "Kernel Invalid Desc";
        case EPlayerReturnCodes::KernelInvalidContext: return "Kernel Invalid Context";
        case EPlayerReturnCodes::KernelInternalFailure: return "Kernel Internal Failure";
        case EPlayerReturnCodes::ScenarioNotLoaded: return "Scenario Not Loaded";
        default: return "Invalid Code";
    }
}
}   // namespace OpenViBE
