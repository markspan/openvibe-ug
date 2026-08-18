#include <fstream>
#include <sstream>
#include <string>
#include <iostream>
#include <cmath>
#include <cfloat> // DBL_EPSILON

int main(int argc, char** argv)
{
    if (argc != 2) {
        std::cout << "Usage: test_evaluation <filename>\n";
        return 3;
    }

    std::ifstream file(argv[1], std::ios::in);

    if (file.good() && !file.bad() && file.is_open()) // ...
    {
        std::string line;
        while (getline(file, line)) {
            if (line.find("Final value of Kappa") != std::string::npos) {
                std::cout << "Found kappa line " << line << std::endl;

                const size_t pos          = line.rfind(' ');
                const std::string cutline = line.substr(pos);

                std::stringstream kappa(cutline);

                double coefficient = 0;
                kappa >> coefficient;

                if (std::fabs(coefficient - 0.840677) > DBL_EPSILON) {
                    std::cout << "Wrong Kappa coefficient. Found " << coefficient << " instead of 0.840677" << std::endl;
                    return 1;
                }
                std::cout << "Test ok" << std::endl;
                return 0;
            }
        }
    }
    std::cout << "Error: Problem opening [" << argv[1] << "]\n";

    return 2;
}
