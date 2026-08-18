///-------------------------------------------------------------------------------------------------
/// 
/// \file ovtTestFixtureCommon.cpp
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

#include <toolkit/ovtk_all.h>

#include "ovtTestFixtureCommon.h"

namespace OpenViBE {
namespace Test {
void SKernelFixture::setUp()
{
    const CString kernelFile = Directories::getLib("kernel");
    CString error;

    if (!m_kernelLoader.load(kernelFile, &error)) {
        std::cerr << "ERROR: impossible to load kernel from file located at: " << kernelFile << std::endl;
        std::cerr << "ERROR: kernel error: " << error << std::endl;
        return;
    }

    m_kernelLoader.initialize();

    Kernel::IKernelDesc* kernelDesc = nullptr;
    m_kernelLoader.getKernelDesc(kernelDesc);

    if (!kernelDesc) {
        std::cerr << "ERROR: impossible to retrieve kernel descriptor " << std::endl;
        return;
    }

    CString configFile;

    if (!m_configFile.empty()) { configFile = m_configFile.c_str(); }
    else { configFile = CString(Directories::getDataDir() + "/kernel/openvibe.conf"); }


    Kernel::IKernelContext* ctx = kernelDesc->createKernel("test-kernel", configFile);

    if (!ctx) {
        std::cerr << "ERROR: impossible to create kernel context " << std::endl;
        return;
    }

    ctx->initialize();
    Toolkit::initialize(*ctx);
    context = ctx;
}

void SKernelFixture::tearDown()
{
    if (context) {
        Toolkit::uninitialize(*context);
        Kernel::IKernelDesc* kernelDesc = nullptr;
        m_kernelLoader.getKernelDesc(kernelDesc);
        kernelDesc->releaseKernel(context);
        context = nullptr;
    }

    m_kernelLoader.uninitialize();
    m_kernelLoader.unload();
}
}  // namespace Test
}  // namespace OpenViBE
