///-------------------------------------------------------------------------------------------------
/// 
/// \file main.cpp
/// \author Thibaut Monseigne / Inria.
/// \version 2.0.
/// \date 19/04/2022.
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

#include <iostream>

#include "CompareCSV.hpp"
#include "CompareXML.hpp"

int main(const int argc, char* argv[])
{
    if (argc != 4) {
        std::cout << "Usage: " << argv[0] << " <data> <reference> <threshold>\n";
        return -1;
    }

    const double threshold = strtod(argv[3], nullptr);

    const std::string file1 = argv[1], file2 = argv[2];
    const size_t pos        = file1.find_last_of('.') + 1;
    const std::string ext   = file1.substr(pos);
    Compare* compare;
    if (ext == "csv") { compare = new CompareCSV(threshold); }
    else if (ext == "xml") { compare = new CompareXML(threshold); }
    else {
        std::cout << "Usage: Files must be csv or xml, not \"" << ext << "\"\n";
        return -1;
    }

    if (!compare->Test(argv[1], argv[2])) {
        std::cout << "Algorithm failed validation test \n" << std::endl;
        return 1;
    }

    std::cout << "Test passed\n" << std::endl;
    return 0;
}
