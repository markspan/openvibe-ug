#include "ovbt_sg_defines.h"

#include <fstream>

bool CCppDefineGenerator::openFile(const char* filename)
{
    m_file.open(filename, std::ios::out | std::ios::trunc);
    if (!m_file.is_open()) { return false; }
    m_file << "#pragma once" << std::endl << std::endl;

    return true;
}

bool CCppDefineGenerator::appendStimulation(SStimulation& stim)
{
    m_file << "#define " << stim.id << "  " << stim.hexa << std::endl;
    return true;
}

bool CCppDefineGenerator::closeFile()
{
    m_file << std::endl;
    m_file.close();
    return true;
}
