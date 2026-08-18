///-------------------------------------------------------------------------------------------------
/// 
/// \file CAlgorithmAddition.cpp
/// \author Yann Renard (Inria)
/// \version 1.0.
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

#include "CAlgorithmAddition.hpp"

namespace OpenViBE {
namespace Plugins {
namespace Tests {

bool CAlgorithmAddition::initialize()
{
    m_parameter1.initialize(getInputParameter(CIdentifier(0, 1)));
    m_parameter2.initialize(getInputParameter(CIdentifier(0, 2)));
    m_parameter3.initialize(getOutputParameter(CIdentifier(0, 3)));

    return true;
}

bool CAlgorithmAddition::uninitialize()
{
    m_parameter3.uninitialize();
    m_parameter2.uninitialize();
    m_parameter1.uninitialize();

    return true;
}

bool CAlgorithmAddition::process()
{
    m_parameter3 = m_parameter1 + m_parameter2;

    return true;
}
}  // namespace Tests
}  // namespace Plugins
}  // namespace OpenViBE
