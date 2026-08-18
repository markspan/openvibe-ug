#include <fstream>
#include <sstream>
#include <string>
#include <iostream>
#include <cstring>
#include <cerrno>

double threshold = 72;

int main(int argc, char** argv)
{
    if (argc != 2 && argc != 3) {
        std::cout << "Usage: test_accuracy <filename> <threshold>\n";
        return 3;
    }
    if (argc == 3) {
        std::stringstream ss(argv[2]);
        ss >> threshold;
    }

    std::ifstream file(argv[1], std::ios::in);

    if (file.good() && !file.bad() && file.is_open()) // ...
    {
        std::string line;
        while (getline(file, line)) {
            size_t pos;
            if ((pos = line.find("Cross-validation")) != std::string::npos) {
                std::string cutline = line.substr(pos);
                pos                 = cutline.find("is") + 3;//We need to cut the coloration
                cutline             = cutline.substr(pos);

                pos = cutline.find('%');

                cutline = cutline.substr(0, pos);
                std::stringstream ss(cutline);

                double percentage = 0.0;
                ss >> percentage;

                if (percentage < threshold) {
                    std::cout << "Accuracy too low ( " << percentage << " % )" << std::endl;
                    return 1;
                }
                std::cout << "Test ok ( " << percentage << " % )" << std::endl;
                return 0;
            }
        }
        std::cout << "Error: EOF of log file reached without finding the cross-validation accuracy string.\n";
        return 4;
    }
    std::cout << "Error: Problem opening [" << argv[1] << "]\n";
    std::cerr << "Error: Code is " << strerror(errno) << "\n";
    return 5;
}
