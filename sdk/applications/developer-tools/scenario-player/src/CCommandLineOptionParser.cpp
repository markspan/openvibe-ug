///-------------------------------------------------------------------------------------------------
/// 
/// \file CCommandLineOptionParser.cpp
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

#include "CCommand.hpp"
#include "CCommandLineOptionParser.hpp"

namespace OpenViBE {
CommandLineOptionParser::CommandLineOptionParser(ProgramOptionParser& parser) : m_parser(parser) { }

void CommandLineOptionParser::Initialize() { }  // nothing to do
void CommandLineOptionParser::Uninitialize() { m_cmdList.clear(); }

std::vector<std::shared_ptr<SCommand>> CommandLineOptionParser::GetCommandList() const { return m_cmdList; }

EPlayerReturnCodes CommandLineOptionParser::Parse()
{
    // parsing consists of building a straightfoward command workflow according to command-line
    // options.

    // commands have to be pushed in the right order

    // choose a dumb name for the scenario
    std::string scenarioName = "express-scenario";

    // an init command is always needed
    m_cmdList.push_back(std::make_shared<SInitCmd>());

    // workflow must at least load the kernel
    const std::shared_ptr<SLoadKernelCmd> kernelCmd = std::make_shared<SLoadKernelCmd>();

    if (m_parser.HasOption("config-file")) { kernelCmd->configFile = m_parser.GetOptionValue<std::string>("config-file"); } // optional

    m_cmdList.push_back(kernelCmd);

    // scenario loading is a mandatory step
    const std::shared_ptr<SLoadScenarioCmd> scenarioCmd = std::make_shared<SLoadScenarioCmd>();

    if (m_parser.HasOption("scenario-file")) { // mandatory option
        scenarioCmd->scenarioFile = m_parser.GetOptionValue<std::string>("scenario-file");
        // set dumb name as it used to recognize scenario in the application
        scenarioCmd->scenarioName = scenarioName;
    }
    else {
        std::cerr << "ERROR: mandatory option 'scenario-file' not set" << std::endl;
        return EPlayerReturnCodes::MissingMandatoryArgument;
    }

    m_cmdList.push_back(scenarioCmd);

    // scenario update option
    const std::shared_ptr<SUpdateScenarioCmd> updateScenarioCmd = std::make_shared<SUpdateScenarioCmd>();

    if (m_parser.HasOption("updated-scenario-file")) {
        // do not play scenario, just update it.
        updateScenarioCmd->scenarioFile = m_parser.GetOptionValue<std::string>("updated-scenario-file");

        // set dumb name as it used to recognize scenario in the application
        updateScenarioCmd->scenarioName = scenarioName;

        m_cmdList.push_back(updateScenarioCmd);
    }
    else {
        // check if some scenario setup information has been set
        if (m_parser.HasOption("ds")) {
            const std::shared_ptr<SSetupScenarioCmd> setupCmd = std::make_shared<SSetupScenarioCmd>();
            setupCmd->scenarioName                            = scenarioName;
            setupCmd->tokenList                               = m_parser.GetOptionValue<std::vector<SSetupScenarioCmd::Token>>("ds");

            m_cmdList.push_back(setupCmd);
        }

        // last command in the workflow is the run command
        const std::shared_ptr<SRunScenarioCmd> runCmd = std::make_shared<SRunScenarioCmd>();
        runCmd->scenarioList                          = std::vector<std::string>{ scenarioName };

        if (m_parser.HasOption("play-mode")) {
            const auto playMode = m_parser.GetOptionValue<std::string>("play-mode");

            if (playMode != "ff" && playMode != "std") {
                std::cerr << "ERROR: option 'play-mode' must be ff or std" << std::endl;
                return EPlayerReturnCodes::BadArg;
            }

            // permissive code here
            // any other entry than ff leads to standard mode...
            runCmd->playMode = ((playMode == "ff") ? EPlayerPlayMode::Fastfoward : EPlayerPlayMode::Standard);
        }

        if (m_parser.HasOption("max-time")) { runCmd->maximumExecutionTime = m_parser.GetOptionValue<double>("max-time"); }
        if (m_parser.HasOption("dg")) { runCmd->tokenList = m_parser.GetOptionValue<std::vector<SSetupScenarioCmd::Token>>("dg"); }

        m_cmdList.push_back(runCmd);
    }

    return EPlayerReturnCodes::Success;
}
}   // namespace OpenViBE
