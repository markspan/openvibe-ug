///-------------------------------------------------------------------------------------------------
/// 
/// \file VisualizationTools.cpp
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

#include "VisualizationTools.hpp"

namespace OpenViBE {
namespace AdvancedVisualization {

std::string trim(const std::string& value)
{
    if (value.length() == 0) { return ""; }
    size_t i = 0;
    size_t j = value.length() - 1;
    while (i < value.length() && value[i] == ' ') { i++; }
    while (j > i && value[j] == ' ') { j--; }
    return value.substr(i, j - i + 1);
}

CRendererContext& getContext()
{
    static CRendererContext* ctx = new CRendererContext();
    return *ctx;
}

}  // namespace AdvancedVisualization
}  // namespace OpenViBE
