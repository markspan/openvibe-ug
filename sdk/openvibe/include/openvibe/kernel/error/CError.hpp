///-------------------------------------------------------------------------------------------------
/// 
/// \file CError.hpp
/// \brief Error for Kernel implementation.
///
/// CError aims at providing information about an error occurring in the framework. 
/// One specific concept here is the notion of nested errors.
/// In a call stack, it can be interesting to catch a n - 1 level error and enhance it instead of rethrowing it directy unchanged.
/// In this case, we say the level n - 1 error is nested into the level n error.
/// 
/// \author Charles Garraud (Inria) & Thibaut Monseigne (Inria).
/// \version 1.0.
/// \date 12/07/2016.
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

#include "ErrorType.hpp"
#include <ov_common_defines.h>
#include <memory>

namespace OpenViBE {
namespace Kernel {

/// <summary> Error for Kernel.
///
/// CError aims at providing information about an error occurring in the framework. 
/// One specific concept here is the notion of nested errors.
/// In a call stack, it can be interesting to catch a n - 1 level error and enhance it instead of rethrowing it directy unchanged.
/// In this case, we say the level n - 1 error is nested into the level n error. </summary>
class OV_API CError
{
public:

    //--------------------------------------------------
    //------------ Constructor / Destructor ------------
    //--------------------------------------------------
    /// <summary> Initializes a new instance of the <see cref="CError"/> class. </summary>
    /// <param name="type"> The type. </param>
    /// <param name="description"> The description. </param>
    /// <param name="nestedError"> The nested error. </param>
    /// <param name="filename"> The filename. </param>
    /// <param name="line"> The line in the file. </param>
    CError(const ErrorType type, const std::string& description, CError* nestedError, const std::string& filename, const size_t line)
        : m_errorType(type), m_nestedError(new std::unique_ptr<CError>(nestedError)),
          m_description(new std::string(description)), m_location(new std::string(filename + ":" + std::to_string(line))) { }

    /// <summary> Finalizes an instance of the <see cref="CError"/> class. </summary>
    ~CError()
    {
        delete m_nestedError;
        delete m_description;
        delete m_location;
    }

    /// <summary> Prohibits the constructor by copy. </summary>
    CError(const CError&) = delete;

    /// <summary> Prohibits the copy. </summary>
    CError& operator=(const CError&) = delete;

    //--------------------------------------------------
    //----------------- Getter/Setter ------------------
    //--------------------------------------------------
    /// <summary> Retrieve error description. </summary>
    /// <returns> The error description. </returns>
    const char* getErrorString() const { return m_description->c_str(); }

    /// <summary> Retrieve error location. </summary>
    /// <returns> The error location with file:line format. </returns>
    const char* getErrorLocation() const { return m_location->c_str(); }

    /// <summary> Retrieve error type. </summary>
    ErrorType getErrorType() const { return m_errorType; }

    /// <summary> Retrieve nested error. </summary>
    /// <returns> The nested error if there is one, nullptr otherwise. </returns>
    /// <remarks>The error keeps the ownership of the nested error. Therefore the API consumer is not responsible for its life cycle / management</remarks>
    const CError* getNestedError() const { return m_nestedError->get(); }

private:

    ErrorType m_errorType;                  ///< The error type <see cref="ErrorType"/>.
    std::unique_ptr<CError>* m_nestedError; ///< keep an inheritance of errors (pointer for shared library).
    std::string* m_description;             ///< The error description (pointer for shared library).
    std::string* m_location;                ///< The error location (pointer for shared library).
};

/// \deprecated Use the CError class instead
OV_Deprecated("Use the CError class instead")
typedef CError IError;  ///< Keep previous compatibility. Avoid to used it, intended to be removed.

}  // namespace Kernel
}  // namespace OpenViBE
