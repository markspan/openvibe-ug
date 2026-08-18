///-------------------------------------------------------------------------------------------------
/// 
/// \file ovtKernelContext.cpp
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

#include <iostream>

#include "ovtKernelContext.h"

namespace OpenViBE {
namespace Test {
bool ctx::initialize()
{
    const CString kernelFile = Directories::getLib("kernel");
    CString error;

    if (!m_KernelLoader.load(kernelFile, &error)) {
        std::cerr << "ERROR: impossible to load kernel from file located at: " << kernelFile << std::endl;
        std::cerr << "ERROR: kernel error: " << error << std::endl;
        return false;
    }

    m_KernelLoader.initialize();

    Kernel::IKernelDesc* kernelDesc{ nullptr };
    m_KernelLoader.getKernelDesc(kernelDesc);

    if (!kernelDesc) {
        std::cerr << "ERROR: impossible to retrieve kernel descriptor " << std::endl;
        return false;
    }

    const CString configFile = CString(Directories::getDataDir() + "/kernel/openvibe.conf");

    Kernel::IKernelContext* ctx = kernelDesc->createKernel("test-kernel", configFile);

    if (!ctx) {
        std::cerr << "ERROR: impossible to create kernel context " << std::endl;
        return false;
    }

    ctx->initialize();
    m_Context = ctx;
    return true;
}

bool ctx::uninitialize()
{
    if (m_Context) {
        Kernel::IKernelDesc* kernelDesc{ nullptr };
        m_KernelLoader.getKernelDesc(kernelDesc);
        kernelDesc->releaseKernel(m_Context);
        m_Context = nullptr;
    }

    m_KernelLoader.uninitialize();
    m_KernelLoader.unload();

    return true;
}
}  // namespace Test
}  // namespace OpenViBE
