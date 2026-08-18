///-------------------------------------------------------------------------------------------------
/// 
/// \file CBoxAlgorithmAdditionTest.cpp
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

#include "CBoxAlgorithmAdditionTest.hpp"

#include <cstdlib>
#include <random>

namespace OpenViBE {
namespace Plugins {
namespace Tests {

bool CBoxAlgorithmAdditionTest::initialize()
{
    CString level;
    getStaticBoxContext().getSettingValue(0, level);
    m_logLevel = Kernel::ELogLevel(getTypeManager().getEnumerationEntryValueFromName(OV_TypeId_LogLevel, level));

    m_proxy1 = &getAlgorithmManager().getAlgorithm(getAlgorithmManager().createAlgorithm(Box_AlgorithmAddition));
    m_proxy2 = &getAlgorithmManager().getAlgorithm(getAlgorithmManager().createAlgorithm(Box_AlgorithmAddition));
    m_proxy3 = &getAlgorithmManager().getAlgorithm(getAlgorithmManager().createAlgorithm(Box_AlgorithmAddition));

    m_proxy1->initialize();
    m_proxy2->initialize();
    m_proxy3->initialize();

    m_proxy1->getInputParameter(CIdentifier(0, 1))->setReferenceTarget(&m_i1);
    m_proxy1->getInputParameter(CIdentifier(0, 2))->setReferenceTarget(&m_i2);
    m_proxy2->getInputParameter(CIdentifier(0, 1))->setReferenceTarget(&m_i3);
    m_proxy2->getInputParameter(CIdentifier(0, 2))->setReferenceTarget(&m_i4);
    m_proxy3->getInputParameter(CIdentifier(0, 1))->setReferenceTarget(m_proxy1->getOutputParameter(CIdentifier(0, 3)));
    m_proxy3->getInputParameter(CIdentifier(0, 2))->setReferenceTarget(m_proxy2->getOutputParameter(CIdentifier(0, 3)));

    return true;
}

bool CBoxAlgorithmAdditionTest::uninitialize()
{
    m_proxy1->uninitialize();
    m_proxy2->uninitialize();
    m_proxy3->uninitialize();

    getAlgorithmManager().releaseAlgorithm(*m_proxy1);
    getAlgorithmManager().releaseAlgorithm(*m_proxy2);
    getAlgorithmManager().releaseAlgorithm(*m_proxy3);

    return true;
}

bool CBoxAlgorithmAdditionTest::processClock(Kernel::CMessageClock& /*msg*/)
{
    std::random_device rd;
    std::default_random_engine rng(rd());
    std::uniform_int_distribution<int64_t> uni; // no const on unix system

    m_i1 = (uni(rng) % 100);
    m_i2 = (uni(rng) % 100) * 100;
    m_i3 = (uni(rng) % 100) * 10000;
    m_i4 = (uni(rng) % 100) * 1000000;

    m_proxy1->process();
    m_proxy2->process();
    m_proxy3->process();

    const Kernel::TParameterHandler<int64_t> parameter11(m_proxy1->getInputParameter(CIdentifier(0, 1))),
                                             parameter12(m_proxy1->getInputParameter(CIdentifier(0, 2))),
                                             parameter13(m_proxy1->getOutputParameter(CIdentifier(0, 3))),
                                             parameter21(m_proxy2->getInputParameter(CIdentifier(0, 1))),
                                             parameter22(m_proxy2->getInputParameter(CIdentifier(0, 2))),
                                             parameter23(m_proxy2->getOutputParameter(CIdentifier(0, 3))),
                                             parameter31(m_proxy3->getInputParameter(CIdentifier(0, 1))),
                                             parameter32(m_proxy3->getInputParameter(CIdentifier(0, 2))),
                                             parameter33(m_proxy3->getOutputParameter(CIdentifier(0, 3)));

    getLogManager() << m_logLevel << "paramater_1_1 = " << parameter11 << "\n";
    getLogManager() << m_logLevel << "paramater_1_2 = " << parameter12 << "\n";
    getLogManager() << m_logLevel << "paramater_1_3 = " << parameter13 << "\n";
    getLogManager() << m_logLevel << "paramater_2_1 = " << parameter21 << "\n";
    getLogManager() << m_logLevel << "paramater_2_2 = " << parameter22 << "\n";
    getLogManager() << m_logLevel << "paramater_2_3 = " << parameter23 << "\n";
    getLogManager() << m_logLevel << "paramater_3_1 = " << parameter31 << "\n";
    getLogManager() << m_logLevel << "paramater_3_2 = " << parameter32 << "\n";
    getLogManager() << m_logLevel << "paramater_3_3 = " << parameter33 << "\n";
    getLogManager() << m_logLevel << "------------------\n";

    return true;
}

bool CBoxAlgorithmAdditionTest::process() { return true; }
}  // namespace Tests
}  // namespace Plugins
}  // namespace OpenViBE
