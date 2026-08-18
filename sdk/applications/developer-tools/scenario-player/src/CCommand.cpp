///-------------------------------------------------------------------------------------------------
/// 
/// \file CCommand.cpp
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

#include "CKernelFacade.hpp"
#include "CCommand.hpp"

namespace OpenViBE {
EPlayerReturnCodes SInitCmd::Execute(CKernelFacade& kernelFacade) const
{
    std::cout << "About to execute:" << std::endl << *this << std::endl;
    return kernelFacade.Initialize();
}

void SInitCmd::doPrint(std::ostream& os) const
{
    os << "command name: InitCommand" << std::endl;
    os << "Benchmark: " << (this->benchmark ? std::to_string(this->benchmark.get()) : "not set") << std::endl;
}

EPlayerReturnCodes SLoadKernelCmd::Execute(CKernelFacade& kernelFacade) const
{
    std::cout << "About to execute:" << std::endl << *this << std::endl;
    // default config file is an empty one so it is not problem to give it directly as param
    return kernelFacade.LoadKernel(*this);
}

void SLoadKernelCmd::doPrint(std::ostream& os) const
{
    os << "command name: LoadKernelCommand" << std::endl;
    os << "ConfigurationFile: " << (this->configFile ? this->configFile.get() : "not set") << std::endl;
}

EPlayerReturnCodes SLoadScenarioCmd::Execute(CKernelFacade& kernelFacade) const
{
    std::cout << "About to execute:" << std::endl << *this << std::endl;

    if (!this->scenarioName || !this->scenarioFile) {
        std::cerr << "Missing required arguments for command" << std::endl;
        return EPlayerReturnCodes::MissingMandatoryArgument;
    }

    return kernelFacade.LoadScenario(*this);
}

void SLoadScenarioCmd::doPrint(std::ostream& os) const
{
    os << "command name: LoadScenarioCommand" << std::endl;
    os << "ScenarioName: " << (this->scenarioName ? this->scenarioName.get() : "not set") << std::endl;
    os << "ScenarioFile: " << (this->scenarioFile ? this->scenarioFile.get() : "not set") << std::endl;
}

EPlayerReturnCodes SUpdateScenarioCmd::Execute(CKernelFacade& kernelFacade) const
{
    std::cout << "About to execute:" << std::endl << *this << std::endl;

    if (!this->scenarioName || !this->scenarioFile) {
        std::cerr << "Missing required arguments for command" << std::endl;
        return EPlayerReturnCodes::MissingMandatoryArgument;
    }

    return kernelFacade.UpdateScenario(*this);
}

void SUpdateScenarioCmd::doPrint(std::ostream& os) const
{
    os << "command name: UpdateScenarioCommand" << std::endl;
    os << "ScenarioName: " << (this->scenarioName ? this->scenarioName.get() : "not set") << std::endl;
    os << "ScenarioFile: " << (this->scenarioFile ? this->scenarioFile.get() : "not set") << std::endl;
}

EPlayerReturnCodes SResetCmd::Execute(CKernelFacade& /*kernelFacade*/) const
{
    // to be implemented
    std::cout << "About to execute:" << std::endl << *this << std::endl << "Not implemented yet" << std::endl;
    return EPlayerReturnCodes::Success;
}

void SResetCmd::doPrint(std::ostream& os) const { os << "command name: ResetCommand" << std::endl; }

EPlayerReturnCodes SRunScenarioCmd::Execute(CKernelFacade& kernelFacade) const
{
    std::cout << "About to execute:" << std::endl << *this << std::endl;
    if (!this->scenarioList) {
        std::cerr << "Missing required arguments for command: ScenarioList" << std::endl;
        return EPlayerReturnCodes::MissingMandatoryArgument;
    }
    return kernelFacade.RunScenarioList(*this);
}

void SRunScenarioCmd::doPrint(std::ostream& os) const
{
    os << "command name: SRunScenarioCmd" << std::endl;

    os << "ScenarioList:";
    if (this->scenarioList) { for (auto& scenario : this->scenarioList.get()) { os << " " << scenario; } }
    else { os << " not set"; }
    os << std::endl;

    os << "PlayMode: ";
    if (this->playMode) {
        const std::string modeAsString = (this->playMode == EPlayerPlayMode::Fastfoward) ? "fastforward" : "standard";
        os << modeAsString;
    }
    else { os << "not set"; }
    os << std::endl;

    os << "MaximumExecutionTime: " << (this->maximumExecutionTime ? std::to_string(this->maximumExecutionTime.get()) : "not set") << std::endl;

    os << "TokenList:";
    if (this->tokenList) { for (auto& token : this->tokenList.get()) { os << " (" << token.first << "," << token.second << ")"; } }
    else { os << " not set"; }
    os << std::endl;
}

EPlayerReturnCodes SSetupScenarioCmd::Execute(CKernelFacade& kernelFacade) const
{
    std::cout << "About to execute:" << std::endl << *this << std::endl;
    if (!this->scenarioName) {
        std::cerr << "Missing required arguments for command" << std::endl;
        return EPlayerReturnCodes::MissingMandatoryArgument;
    }
    return kernelFacade.SetupScenario(*this);
}

void SSetupScenarioCmd::doPrint(std::ostream& os) const
{
    os << "command name: SSetupScenarioCmd" << std::endl;
    os << "ScenarioName: " << (this->scenarioName ? this->scenarioName.get() : "not set") << std::endl;
    os << "TokenList:";
    if (this->tokenList) { for (auto& token : this->tokenList.get()) { os << " (" << token.first << "," << token.second << ")"; } }
    else { os << " not set"; }
    os << std::endl;
}
}   // namespace OpenViBE
