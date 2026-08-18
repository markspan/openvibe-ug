///-------------------------------------------------------------------------------------------------
/// 
/// \file CCommandFileParser.hpp
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
#include <string>
#include <functional>
#include <fstream>
#include <vector>
#include <map>

namespace OpenViBE {
/**
* \brief Parser implementation that parses command a file
* \ingroup ScenarioPlayer
*
* The current implementation retrieves the list of commands from a file.
* 
* Input file format requirements:
* - Command names are defined within brackets [InitCommand] on a single line
* - Each command parameter is defined on a single line below the command name:
*  + Single value parameters are defined as: parameterName: value
*  + Simple list parameters are defined as: parameterName: {val1,val2,...}
*  + Token list parameters are defined as: parameterName: { (token1:val1),(token2:val2) } (no ',' authorized in (token, value) pair)
*  .
* .
*
* \todo The implementation uses a lot of string utility functions that need more testing
*
* \note Use of regex would simplify the implementation but boost::regex is not header-only and std::regex not implemented in gcc 4.8
*
*/
class CommandFileParser final : public ICommandParser
{
public:
    /**
    *
    * \brief Constructor
    * \param[in] file path to the command file
    *
    */
    explicit CommandFileParser(const std::string& file) : m_cmdFile(file) { }

    void Initialize() override;
    void Uninitialize() override;

    std::vector<std::shared_ptr<SCommand>> GetCommandList() const override { return m_cmdList; }

    EPlayerReturnCodes Parse() override;

private:
    using CallbackType = std::function<EPlayerReturnCodes(std::vector<std::string>)>;
    using Token = std::pair<std::string, std::string>;

    static std::string trim(const std::string& str);
    static std::pair<std::string, std::string> tokenize(const std::string& str);
    static std::vector<std::string> split(const std::string& str, char delimiter);
    static bool toBool(const std::string& str);
    static std::vector<std::string> toList(const std::string& str);
    static std::vector<Token> toTokenList(const std::string& str);

    EPlayerReturnCodes flush(const std::string& tag, const std::vector<std::string>& content);

    EPlayerReturnCodes initCommandCb(const std::vector<std::string>& content);
    EPlayerReturnCodes resetCommandCb(const std::vector<std::string>& content);
    EPlayerReturnCodes loadKernelCommandCb(const std::vector<std::string>& content);
    EPlayerReturnCodes loadScenarioCommandCb(const std::vector<std::string>& content);
    EPlayerReturnCodes setupScenarioCommandCb(const std::vector<std::string>& content);
    EPlayerReturnCodes runScenarioCommandCb(const std::vector<std::string>& content);

    std::string m_cmdFile;
    std::vector<std::shared_ptr<SCommand>> m_cmdList;
    std::map<std::string, CallbackType> m_callbacks;
};
}  // namespace OpenViBE
