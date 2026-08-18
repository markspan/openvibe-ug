#include "ovbt_sg_defines.h"

#include <iostream>
#include <fstream>
#include <string>
#include <vector>

enum class EGenerationTypes { CPP, MATLAB, PYTHON, LUA, UNKNOWN };

EGenerationTypes parse_argument(std::string option)
{
    if (option == "--cpp") { return EGenerationTypes::CPP; }
    if (option == "--matlab") { return EGenerationTypes::MATLAB; }
    if (option == "--python") { return EGenerationTypes::PYTHON; }
    if (option == "--lua") { return EGenerationTypes::LUA; }
    return EGenerationTypes::UNKNOWN;
}

int generate_generator_list(std::vector<CFileGeneratorBase*>& list, EGenerationTypes type, int argc, char** argv)
{
    switch (type) {
        case EGenerationTypes::CPP:
        {
            if (argc < 4) { return -1; }
            CFileGeneratorBase* gen = new CCppDefineGenerator();
            if (!gen->openFile(argv[3])) {
                std::cerr << "Unable to open " << argv[3] << std::endl;
                return -1;
            }
            list.push_back(gen);

            gen = new CCppCodeGenerator();
            if (!gen->openFile(argv[4])) {
                std::cerr << "Unable to open " << argv[4] << std::endl;
                return -1;
            }
            list.push_back(gen);
            return 0;
        }

        case EGenerationTypes::MATLAB:
        {
            CFileGeneratorBase* gen = new CMatlabGenerator();
            if (!gen->openFile(argv[3])) {
                std::cerr << "Unable to open " << argv[3] << std::endl;
                return -1;
            }
            list.push_back(gen);
            return 0;
        }
        case EGenerationTypes::PYTHON:
        case EGenerationTypes::LUA:
        case EGenerationTypes::UNKNOWN: default:
        {
            std::cerr << "Unhandle type. Fatal error" << std::endl;
            return -1;
        }
    }
}

int main(int argc, char** argv)
{
    if (argc < 3) { return -1; }
    EGenerationTypes type = parse_argument(argv[1]);

    std::vector<SStimulation> stimulations;
    std::vector<CFileGeneratorBase*> generators;

    std::ifstream file(argv[2]);
    std::string name, id, hexaCode;
    while (file >> name >> id >> hexaCode) {
        SStimulation temp = { name, id, hexaCode };
        stimulations.push_back(temp);
    }

    if (generate_generator_list(generators, type, argc, argv)) { return -1; }

    //Now we generate all files that needs to be done
    for (auto& s : stimulations) { for (auto& g : generators) { g->appendStimulation(s); } }
    for (auto& g : generators) { g->closeFile(); }

    return 0;
}
