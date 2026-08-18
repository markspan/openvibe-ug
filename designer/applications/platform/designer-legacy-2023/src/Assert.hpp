///-------------------------------------------------------------------------------------------------
/// 
/// \file Assert.hpp
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

#include <array>
#include <exception>
#include <stdexcept>
#include <sstream>
#include <fs/Files.h>

#include <openvibe/ovAssert.h>
#include <openvibe/kernel/error/CErrorManager.hpp>
#include <openvibe/kernel/error/CError.hpp>

#define OV_ERROR_D(description, type, returnValue) OV_ERROR(description, type, returnValue, m_kernelCtx.getErrorManager(), m_kernelCtx.getLogManager())
#define OV_ERROR_DRF(description, type) OV_ERROR(description, type, false, m_kernelCtx.getErrorManager(), m_kernelCtx.getLogManager())
#define OV_ERROR_DRZ(description, type) OV_ERROR(description, type, 0, m_kernelCtx.getErrorManager(), m_kernelCtx.getLogManager())
#define OV_ERROR_DRU(description, type) OV_ERROR(description, type, CIdentifier::undefined(), m_kernelCtx.getErrorManager(), m_kernelCtx.getLogManager())
#define OV_ERROR_DRV(description, type) OV_ERROR(description, type, void(), m_kernelCtx.getErrorManager(), m_kernelCtx.getLogManager())
#define OV_ERROR_DRN(description, type) OV_ERROR(description, type, nullptr, m_kernelCtx.getErrorManager(), m_kernelCtx.getLogManager())

#define OV_ERROR_UNLESS_D(expression, description, type, returnValue) OV_ERROR_UNLESS(expression, description, type, returnValue, m_kernelCtx.getErrorManager(), m_kernelCtx.getLogManager())
#define OV_ERROR_UNLESS_DRF(expression, description, type) OV_ERROR_UNLESS(expression, description, type, false, m_kernelCtx.getErrorManager(), m_kernelCtx.getLogManager())
#define OV_ERROR_UNLESS_DRZ(expression, description, type) OV_ERROR_UNLESS(expression, description, type, 0, m_kernelCtx.getErrorManager(), m_kernelCtx.getLogManager())
#define OV_ERROR_UNLESS_DRU(expression, description, type) OV_ERROR_UNLESS(expression, description, type, CIdentifier::undefined(), m_kernelCtx.getErrorManager(), m_kernelCtx.getLogManager())
#define OV_ERROR_UNLESS_DRV(expression, description, type) OV_ERROR_UNLESS(expression, description, type, void() , m_kernelCtx.getErrorManager(), m_kernelCtx.getLogManager())
#define OV_ERROR_UNLESS_DRN(expression, description, type) OV_ERROR_UNLESS(expression, description, type, nullptr, m_kernelCtx.getErrorManager(), m_kernelCtx.getLogManager())

#define OV_FATAL_D(description, type) OV_FATAL(description, type, m_kernelCtx.getLogManager())
#define OV_FATAL_UNLESS_D(expression, description, type) OV_FATAL_UNLESS(expression, description, type, m_kernelCtx.getLogManager())

#ifndef _MSC_VER
#define NOEXCEPT noexcept
#else
#define NOEXCEPT
#endif

class DesignerException final : public std::runtime_error
{
public:
    explicit DesignerException(OpenViBE::Kernel::CErrorManager& errorManager)
        : std::runtime_error("Designer caused an exception"), m_ErrorManager(errorManager) {}

    const char* what() const NOEXCEPT override { return m_ErrorManager.getLastErrorString(); }

    std::string GetErrorString() const
    {
        std::string errorMessage;
        const OpenViBE::Kernel::CError* error = m_ErrorManager.getLastError();
        while (error) {
            std::array<char, 1024> location;
            FS::Files::getFilename(error->getErrorLocation(), location.data());
            errorMessage += "Message: " + std::string(error->getErrorString()) + "\nFile: " + location.data() + "\n";
            error = error->getNestedError();
        }
        m_ErrorManager.releaseErrors();
        return errorMessage;
    }

    void ReleaseErrors() const NOEXCEPT { m_ErrorManager.releaseErrors(); }

    OpenViBE::Kernel::CErrorManager& m_ErrorManager;
};

#define OV_EXCEPTION_D(description, type) \
do { \
    m_kernelCtx.getErrorManager().pushError(type, static_cast<const std::ostringstream&>(std::ostringstream() << description).str().c_str(), __FILE__, __LINE__ ); \
    m_kernelCtx.getLogManager() << OpenViBE::Kernel::LogLevel_Fatal << "[Error description] = " << description << "; [Error code] = " << size_t((type)) << "\n"; \
    throw DesignerException(m_kernelCtx.getErrorManager()); \
} while(0)

#define OV_EXCEPTION_UNLESS_D(expression, description, type) \
do { if (!(expression)) { OV_EXCEPTION_D(description, type); } } while(0)
