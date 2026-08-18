///-------------------------------------------------------------------------------------------------
/// 
/// \file main.cpp
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

#include <string>
#include <vector>
#include <memory>

#include "defines.hpp"
#include "CKernelFacade.hpp"
#include "CCommand.hpp"
#include "CCommandLineOptionParser.hpp"
#include "CCommandFileParser.hpp"

namespace OpenViBE {
void initializeParser(ProgramOptionParser& parser)
{
    const std::string desc = R"d(Usage: program options

Program can be run in express mode to directly execute a scenario
Program can be run in command mode to execute list of commands from a file

)d";
    parser.SetGlobalDesc(desc);

    parser.AddSimpleOption("help", { "h", "Help" });
    parser.AddSimpleOption("version", { "v", "Program version" });

    parser.AddValueOption<SProgramOptionsTraits::String>("mode", { "m", "Execution mode: 'x' for express, 'c' for command [mandatory]" });

    // express mode options
    parser.AddValueOption<SProgramOptionsTraits::String>("config-file", { "", "Path to configuration file (express mode only)" });
    parser.AddValueOption<SProgramOptionsTraits::String>("scenario-file", { "", "Path to scenario file (express mode only) [mandatory]" });
    parser.AddValueOption<SProgramOptionsTraits::String>("updated-scenario-file", { "", "Enable update process instead of playing scenario. Path to the updated scenario file (express mode only)." });
    parser.AddValueOption<SProgramOptionsTraits::String>("play-mode", { "", "Play mode: std for standard and ff for fast-foward (express mode only) [default=std]" });
    parser.AddValueOption<SProgramOptionsTraits::Float>("max-time", { "", "Scenarios playing execution time limit (express mode only)" });
    parser.AddValueOption<SProgramOptionsTraits::TokenPairList>("dg", { "", "Global user-defined token: -dg=\"(token:value)\" (express mode only)" });
    parser.AddValueOption<SProgramOptionsTraits::TokenPairList>("ds", { "", "Scenario user-defined token: -ds=\"(token:value)\" (express mode only)" });

    // command mode options
    parser.AddValueOption<SProgramOptionsTraits::String>("command-file", { "", "Path to command file (command mode only) [mandatory]" });
}

}  // namespace OpenViBE

int main(int argc, char** argv)
{
    OpenViBE::ProgramOptionParser optionParser;
    initializeParser(optionParser);

    if (!optionParser.Parse(argc, argv)) {
        std::cerr << "ERROR: Failed to parse arguments" << std::endl;
        return int(OpenViBE::EPlayerReturnCodes::InvalidArg);
    }
    if (optionParser.HasOption("help")) {
        optionParser.PrintOptionsDesc();
        return int(OpenViBE::EPlayerReturnCodes::Success);
    }
    if (optionParser.HasOption("version")) {
        // PROJECT_VERSION is added to definition from cmake
        std::cout << "version: " << PROJECT_VERSION << std::endl;
        return int(OpenViBE::EPlayerReturnCodes::Success);
    }
    if (optionParser.HasOption("mode") || optionParser.HasOption("updated-scenario-file")) {
        // command parser type is selected from mode
        std::unique_ptr<OpenViBE::ICommandParser> commandParser{ nullptr };
        const auto mode = optionParser.GetOptionValue<OpenViBE::SProgramOptionsTraits::String>("mode");

        if (mode == "c") {
            // check for the mandatory commad file
            if (optionParser.HasOption("command-file")) {
                commandParser.reset(new OpenViBE::CommandFileParser(optionParser.GetOptionValue<std::string>("command-file")));
            }
            else {
                std::cerr << "ERROR: mandatory option 'command-file' not set" << std::endl;
                return int(OpenViBE::EPlayerReturnCodes::MissingMandatoryArgument);
            }
        }
        else if ((mode == "x") || optionParser.HasOption("updated-scenario-file")) { commandParser.reset(new OpenViBE::CommandLineOptionParser(optionParser)); }
        else {
            std::cerr << "ERROR: unknown mode set" << std::endl;
            std::cerr << "Mode must be 'x' or 'c'" << std::endl;
            return int(OpenViBE::EPlayerReturnCodes::InvalidArg);
        }

        commandParser->Initialize();

        try {
            auto returnCode = commandParser->Parse();

            if (returnCode == OpenViBE::EPlayerReturnCodes::Success) {
                OpenViBE::CKernelFacade kernel;

                for (const auto& cmd : commandParser->GetCommandList()) {
                    returnCode = cmd->Execute(kernel);
                    if (returnCode != OpenViBE::EPlayerReturnCodes::Success) { return int(returnCode); }
                }
            }
            else { return int(returnCode); }
        }
        catch (const std::exception& e) {
            std::cerr << "ERROR: received unexpected exception: " << e.what() << std::endl;
            return int(OpenViBE::EPlayerReturnCodes::UnkownFailure);
        }
    }
    else {
        std::cerr << "ERROR: mandatory option 'mode' not set" << std::endl;
        return int(OpenViBE::EPlayerReturnCodes::MissingMandatoryArgument);
    }

    return int(OpenViBE::EPlayerReturnCodes::Success);
}
