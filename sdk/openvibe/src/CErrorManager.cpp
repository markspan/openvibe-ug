///-------------------------------------------------------------------------------------------------
/// 
/// \file CErrorManager.cpp
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

#include <string>

#include "kernel/error/CErrorManager.hpp"

namespace OpenViBE {
namespace Kernel {

void CErrorManager::pushError(const ErrorType type, const std::string& description, const std::string& filename, size_t line) const
{
    std::lock_guard<std::mutex> lock(*m_managerGuard);
    auto* const lastTopError = m_topError->release();
    m_topError->reset(new CError(type, description, lastTopError, filename, line));
}

void CErrorManager::releaseErrors() const
{
    std::lock_guard<std::mutex> lock(*m_managerGuard);
    m_topError->reset(nullptr);
}

bool CErrorManager::hasError() const
{
    std::lock_guard<std::mutex> lock(*m_managerGuard);
    return (*m_topError != nullptr);
}

const CError* CErrorManager::getLastError() const
{
    std::lock_guard<std::mutex> lock(*m_managerGuard);
    return m_topError->get();
}

const char* CErrorManager::getLastErrorString() const
{
    std::lock_guard<std::mutex> lock(*m_managerGuard);
    return (*m_topError ? (*m_topError)->getErrorString() : "");
}

ErrorType CErrorManager::getLastErrorType() const
{
    std::lock_guard<std::mutex> lock(*m_managerGuard);
    return (*m_topError ? (*m_topError)->getErrorType() : ErrorType::NoErrorFound);
}

}  // namespace Kernel
}  // namespace OpenViBE
