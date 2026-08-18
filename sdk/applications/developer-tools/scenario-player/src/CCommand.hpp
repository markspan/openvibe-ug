///-------------------------------------------------------------------------------------------------
/// 
/// \file CCommand.hpp
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

#include "ICommand.hpp"
#include <boost/optional.hpp>
#include <string>

namespace OpenViBE {
/**
* \brief Command that drives the initialization of the tool
* \ingroup ScenarioPlayer
*
* InitCommand class contains the following properties:
* - Benchmark: Flag to enable benchmark on execute command (optional).
*/
struct SInitCmd final : SCommand
{
    // List of properties
    boost::optional<bool> benchmark;

    EPlayerReturnCodes Execute(CKernelFacade& kernelFacade) const override;

protected:
    void doPrint(std::ostream& os) const override;
};


/**
* \brief Command that drives kernel loading
* \ingroup ScenarioPlayer
*
* LoadKernelCommand contains the following properties:
* - ConfigurationFile: Path to kernel configuration file (optional)
*/
struct SLoadKernelCmd final : SCommand
{
    // List of properties
    boost::optional<std::string> configFile;

    EPlayerReturnCodes Execute(CKernelFacade& kernelFacade) const override;

protected:
    void doPrint(std::ostream& os) const override;
};

/**
* \brief Command that drives scenario loading
* \ingroup ScenarioPlayer
*
* LoadScenarioCommand contains the following properties:
* - ScenarioFile: Path to xml scenario file (mandatory)
* - ScenarioName: Name of the scenario (mandatory)
*/
struct SLoadScenarioCmd final : SCommand
{
    // List of properties
    boost::optional<std::string> scenarioFile;
    boost::optional<std::string> scenarioName;

    EPlayerReturnCodes Execute(CKernelFacade& kernelFacade) const override;

protected:
    void doPrint(std::ostream& os) const override;
};

/**
* \brief Command that drives scenario update and export
* \ingroup ScenarioPlayer
*
* UpdateScenarioCommand contains the following properties:
* - ScenarioFile: Path to xml scenario file (mandatory)
*/
struct SUpdateScenarioCmd final : SCommand
{
    // List of properties
    boost::optional<std::string> scenarioFile;
    boost::optional<std::string> scenarioName;

    EPlayerReturnCodes Execute(CKernelFacade& kernelFacade) const override;

protected:
    void doPrint(std::ostream& os) const override;
};

/**
* \brief Command that drives tool reset to its initial state
* \ingroup ScenarioPlayer
*/
struct SResetCmd final : SCommand
{
    EPlayerReturnCodes Execute(CKernelFacade& kernelFacade) const override;

protected:
    void doPrint(std::ostream& os) const override;
};

/**
* \brief Command that drives the execution of a list of scenarios
* \ingroup ScenarioPlayer
*
* SRunScenarioCmd contains the following properties:
* - ScenarioList: Names of scenario that must be executed (mandatory)
* - PlayMode: 0 for standard, 1 for fastforward (optional)
* - MaximumExecutionTime: Scenarios playing execution time limit (optional)
* - TokenList: List of global (token,value) pairs (optional)
*/
struct SRunScenarioCmd final : SCommand
{
    using Token = std::pair<std::string, std::string>;

    // List of properties
    boost::optional<std::vector<std::string>> scenarioList;
    boost::optional<EPlayerPlayMode> playMode;
    boost::optional<double> maximumExecutionTime;
    boost::optional<std::vector<Token>> tokenList;

    EPlayerReturnCodes Execute(CKernelFacade& kernelFacade) const override;

protected:
    void doPrint(std::ostream& os) const override;
};

/**
* \brief Command that drives the setup of a scenario
* \ingroup ScenarioPlayer
*
* SSetupScenarioCmd contains the following properties:
* - ScenarioName: name of the scenario to setup (mandatory)
* - TokenList: List of scenario specific tokens (optional)
*
* The token list overwrites the previous token list if the command was already
* called on the same scenario (note that an empty token list is allowed).
*/
struct SSetupScenarioCmd final : SCommand
{
    using Token = std::pair<std::string, std::string>;

    // List of properties
    boost::optional<std::string> scenarioName;
    boost::optional<std::vector<Token>> tokenList;

    EPlayerReturnCodes Execute(CKernelFacade& kernelFacade) const override;

protected:
    void doPrint(std::ostream& os) const override;
};
}   // namespace OpenViBE
