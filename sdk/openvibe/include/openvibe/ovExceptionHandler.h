///-------------------------------------------------------------------------------------------------
/// 
/// \file ovExceptionHandler.h
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

#include <exception>
#include <functional>
#include <locale.h>
#include <stdexcept>
#include <type_traits>

namespace OpenViBE {
using ExceptionHandlerType = std::function<void(const std::exception&)>;

/**
  * \brief Invokes code and potentially translates exceptions to boolean
  * 
  * \tparam TCallback callable type (e.g. functor) with TCallback() returning boolean
  * 
  * \param callable code that must be guarded against exceptions
  * \param handler callback that handles the exception
  * \return false either if callable() returns false or an exception 
  *         occurs, true otherwise
  * 
  * \details This method is a specific exception-to-boolean translation
  *          method. If an exception is caught, it is handled by calling
  *          the provided exception handler.
  */
template <typename TCallback, typename std::enable_if<std::is_same<bool, typename std::result_of<TCallback()>::type>::value>::type* = nullptr>
bool translateException(TCallback&& callable, const ExceptionHandlerType& handler)
{
    setlocale(LC_ALL, "C");
    try { return callable(); }
    catch (const std::exception& exception)
    {
        handler(exception);
        return false;
    }
    catch (...)
    {
        handler(std::runtime_error("unknown exception"));
        return false;
    }
}
}  // namespace OpenViBE
