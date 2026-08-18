///-------------------------------------------------------------------------------------------------
/// 
/// \file CKernelFacade.hpp
/// \author Charles Garraud.
/// \version 1.0.
/// \date 25/01/2016.
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

#include "defines.hpp"
#include <memory>

namespace OpenViBE {
struct SInitCmd;
struct SResetCmd;
struct SLoadKernelCmd;
struct SLoadScenarioCmd;
struct SUpdateScenarioCmd;
struct SSetupScenarioCmd;
struct SRunScenarioCmd;

/**
* \brief Wrapper class used to access Kernel features
* \ingroup ScenarioPlayer
*
* This class is one-to-many interface used as a central point
* to access a subset of Kernel features.
*
*/
class CKernelFacade final
{
public:
    CKernelFacade();
    ~CKernelFacade();

    /**
    * \brief Initialize session parameters
    */
    static EPlayerReturnCodes Initialize() { return EPlayerReturnCodes::Success; }

    /**
    * \brief Reset session parameters
    */
    static EPlayerReturnCodes Uninitialize() { return EPlayerReturnCodes::Success; }

    /**
    * \brief Load kernel
    * \param[in] command command containing all mandatory properties
    */
    EPlayerReturnCodes LoadKernel(const SLoadKernelCmd& command) const;

    /**
    * \brief Unload kernel
    */
    EPlayerReturnCodes UnloadKernel() const;

    /**
    * \brief Load scenario
    * \param[in] command command containing all mandatory properties
    */
    EPlayerReturnCodes LoadScenario(const SLoadScenarioCmd& command) const;

    /**
    * \brief Update scenario
    * \param[in] command command containing all mandatory properties
    */
    EPlayerReturnCodes UpdateScenario(const SUpdateScenarioCmd& command) const;

    /**
    * \brief Configure scenario
    * \param[in] command command containing all mandatory properties
    */
    EPlayerReturnCodes SetupScenario(const SSetupScenarioCmd& command) const;

    /**
    * \brief Run one or multiple scenarios
    * \param[in] command command containing all mandatory properties
    */
    EPlayerReturnCodes RunScenarioList(const SRunScenarioCmd& command) const;

private:
    // disable copy and assignment because it is not meant to used
    // as a value class event if it is not inheritable
    CKernelFacade(const CKernelFacade&)            = delete;
    CKernelFacade& operator=(const CKernelFacade&) = delete;

    struct SKernelFacadeImpl;
    std::unique_ptr<SKernelFacadeImpl> m_impl;
};
}   // namespace OpenViBE
