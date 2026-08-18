///-------------------------------------------------------------------------------------------------
/// 
/// \file CErrorManager.hpp
/// \brief Error manager kernel default implementation
///
/// This manager is reponsible for handling errors in the framework.
/// Errors in the framework are considered as not acceptable behavior that can be detected and handled by the system.
/// The concept of error is thus independant of warning or fatal crashes that must be handled separately.
/// 
/// \author Charles Garraud (Inria) & Thibaut Monseigne (Inria).
/// \version 1.0.
/// \date 13/07/2016.
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

#include "CError.hpp"
#include "ErrorType.hpp"

#include <ov_common_defines.h>
#include <mutex>
#include <memory>

namespace OpenViBE {
namespace Kernel {

/// <summary> Error manager kernel default implementation. </summary>
class OV_API CErrorManager
{
public:

    /// <summary> Initializes a new instance of the <see cref="CErrorManager" /> class. </summary>
    CErrorManager() : m_managerGuard(new std::mutex), m_topError(new std::unique_ptr<CError>(nullptr)) { }

    /// <summary> Finalizes an instance of the <see cref="CErrorManager" /> class. </summary>
    ~CErrorManager()
    {
        this->releaseErrors();
        delete m_managerGuard;
        delete m_topError;
    }

    /// <summary> Push error to the manager. </summary>
    /// <param name="type"> The error type. </param>
    /// <param name="description"> A self-explanatory description of the error. </param>
    /// <param name="filename"> The source file where the error was detected. </param>
    /// <param name="line"> The line number where the error was detected. </param>
    /// <remarks> Errors already added to the manager will be nested in the newly added error. </remarks>
    void pushError(const ErrorType type, const std::string& description, const std::string& filename = "NoLocationInfo", size_t line = 0) const;

    /// <summary> Push error to the manager. </summary>
    /// <param name="type"> The error type. </param>
    /// <param name="description"> A self-explanatory description of the error. </param>
    /// <remarks> Errors already added to the manager will be nested in the newly added error. </remarks>
    /// \deprecated Use same method with std::string parameter instead.
    OV_Deprecated("Use same method with std::string parameter instead.")
    void pushError(const ErrorType type, const char* description) const { this->pushError(type, description, "NoLocationInfo", 0); }

    /// <summary> Push error to the manager. </summary>
    /// <param name="type"> The error type. </param>
    /// <param name="description"> A self-explanatory description of the error. </param>
    /// <param name="filename"> The source file where the error was detected. </param>
    /// <param name="line"> The line number where the error was detected. </param>
    /// <remarks> Errors already added to the manager will be nested in the newly added error. </remarks>
    /// \deprecated Use the pushError() method with std::string parameter instead.
    OV_Deprecated("Use the pushError() method with std::string parameter instead.")
    void pushErrorAtLocation(const ErrorType type, const char* description, const char* filename, size_t line) const
    {
        this->pushError(type, description, filename, line);
    }

    /// <summary> Release manager errors.
    ///
    /// Release last error added to the manager and potentially all the nested errors recursively. </summary>
    /// <remarks> After this call, do not use pointers to CError retrieved before. It will lead to unexpected behavior mostly due to dangling pointers. </remarks>
    void releaseErrors() const;

    /// <summary> Check for existing errors in the manager. </summary>
    /// <returns> <c>true</c> if manager contains errors, <c>false</c> otherwise. </returns>
    bool hasError() const;

    /// <summary> Get last error added to the manager. </summary>
    /// <returns> The error if manager contains errors, <c>nullptr</c> otherwise. </returns>
    const CError* getLastError() const;

    /// <summary> Get description of last error added to the manager. </summary>
    /// <returns> The description if manager contains errors, empty string otherwise. </returns>
    const char* getLastErrorString() const;

    /// <summary> Get type of last error added to the manager. </summary>
    /// <returns> The type if manager contains error, <c>ErrorType::NoErrorFound</c> otherwise. </returns>
    ErrorType getLastErrorType() const;

private:

    mutable std::mutex* m_managerGuard;     ///< The mutex manager guard.
    std::unique_ptr<CError>* m_topError;    ///< The top error.
};

/// \deprecated Use the CErrorManager class instead
OV_Deprecated("Use the CErrorManager class instead")
typedef CErrorManager IErrorManager;    ///< Keep previous compatibility. Avoid to used it, intended to be removed.

}  // namespace Kernel
}  // namespace OpenViBE
