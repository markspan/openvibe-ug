///-------------------------------------------------------------------------------------------------
/// 
/// \file CCommandLineOptionParser.hpp
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
#include "ICommandParser.hpp"

namespace OpenViBE {
/**
* \brief Parser implementation that parses command from command-line arguments
* \ingroup ScenarioPlayer
*
* The current implementation retrieves the options from a ProgramOptions parser and
* simply builds the commands from the parsed options.
*/
class CommandLineOptionParser final : public ICommandParser
{
public:
    /**
    * \brief Constructor
    * \param[in] parser Specific instantiation of ProgramOptions parser
    */
    explicit CommandLineOptionParser(ProgramOptionParser& parser);

    void Initialize() override;
    void Uninitialize() override;

    std::vector<std::shared_ptr<SCommand>> GetCommandList() const override;

    EPlayerReturnCodes Parse() override;

private:
    ProgramOptionParser& m_parser;
    std::vector<std::shared_ptr<SCommand>> m_cmdList;
};
}   // namespace OpenViBE
