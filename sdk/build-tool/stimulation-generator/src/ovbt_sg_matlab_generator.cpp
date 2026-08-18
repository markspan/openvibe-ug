#include "ovbt_sg_defines.h"

#include <fstream>

std::string getBrutHexaCode(std::string formatedHexaCode)
{
    std::string res = formatedHexaCode;
    res.erase(res.begin(), res.begin() + 2);
    return res;
}

bool CMatlabGenerator::openFile(const char* filename)
{
    m_file.open(filename, std::ios::out | std::ios::trunc);
    if (!m_file.is_open()) { return false; }
    m_file << "function OV_stimulations()" << std::endl << std::endl;

    m_file << "global OVTK_StimulationId_LabelStart;" << std::endl;
    m_file << "OVTK_StimulationId_LabelStart = uint64(hex2dec('00008100'));" << std::endl << std::endl;
    m_file << "global OVTK_StimulationId_LabelEnd;" << std::endl;
    m_file << "OVTK_StimulationId_LabelEnd = uint64(hex2dec('000081ff'));" << std::endl << std::endl;

    return true;
}

bool CMatlabGenerator::appendStimulation(SStimulation& stim)
{
    m_file << "\tglobal " << stim.id << ";" << std::endl;
    m_file << "\t" << stim.id << " = uint64(hex2dec('" << getBrutHexaCode(stim.hexa) << "'));" << std::endl << std::endl;
    return true;
}

bool CMatlabGenerator::closeFile()
{
    m_file << "end" << std::endl;
    m_file.close();
    return true;
}
