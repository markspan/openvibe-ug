/*********************************************************************
* Software License Agreement (AGPL-3 License)
*
* OpenViBE SDK Test Software
* Based on OpenViBE V1.1.0, Copyright (C) Inria, 2006-2015
* Copyright (C) Inria, 2015-2017,V1.0
*
* This program is free software: you can redistribute it and/or modify
* it under the terms of the GNU Affero General Public License version 3,
* as published by the Free Software Foundation.
*
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU Affero General Public License for more details.
*
* You should have received a copy of the GNU Affero General Public License
* along with this program.
* If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#include <string>

#include <openvibe/ov_all.h>

namespace OpenViBE
{
    namespace Test
    {
        class ctx final
        {
        public:
            ctx() = default;
            ~ctx() = default;

            bool initialize();
            bool uninitialize();

            Kernel::IKernelContext* operator->() { return m_Context; }

        private:
            Kernel::IKernelContext* m_Context{nullptr};
            CKernelLoader m_KernelLoader;
        };
    }  // namespace Test
}  // namespace OpenViBE
