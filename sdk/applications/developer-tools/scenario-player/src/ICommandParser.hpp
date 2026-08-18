///-------------------------------------------------------------------------------------------------
/// 
/// \file ICommandParser.hpp
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
#include <vector>
#include <memory>

namespace OpenViBE {
struct SCommand;

/**
* \brief Base abstract class for command parser
* \ingroup ScenarioPlayer
*
* Command parsers aim at parsing a list of commands from a specific input.
*/
class ICommandParser
{
public:
    virtual ~ICommandParser() = default;
    ICommandParser()          = default;

    /**
    * \brief Initialize parser
    */
    virtual void Initialize() = 0;

    /**
    * \brief Unitialize parser
    */
    virtual void Uninitialize() = 0;

    /**
    * \brief Retrieve the list of commands
    * \pre This method should be called after the parse() method
    */
    virtual std::vector<std::shared_ptr<SCommand>> GetCommandList() const = 0;


    /**
    * \brief Retrieve the list of commands
    * \pre This method should be called after the initialize() method
    */
    virtual EPlayerReturnCodes Parse() = 0;

private:
    // disable copy and assignment because it is not meant to used
    // as a value class
    ICommandParser(const ICommandParser&)            = delete;
    ICommandParser& operator=(const ICommandParser&) = delete;
};
}   // namespace OpenViBE
