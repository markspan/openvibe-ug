#include "ovbt_sg_defines.h"

#include <fstream>

bool CCppCodeGenerator::openFile(const char* filename)
{
    m_file.open(filename, std::ios::out | std::ios::trunc);
    if (!m_file.is_open()) { return false; }
    m_file << "#include \"toolkit/ovtk_all.h\"" << std::endl << std::endl;

    m_file << "using namespace OpenViBE;" << std::endl;
    m_file << "using namespace /*OpenViBE::*/Kernel;" << std::endl;
    m_file << "using namespace /*OpenViBE::*/Toolkit;" << std::endl << std::endl << std::endl;

    m_file << "bool Toolkit::initializeStimulationList(const Kernel::IKernelContext& ctx)" << std::endl;
    m_file << "{" << std::endl;
    m_file << "\tITypeManager& typeManager=ctx.getTypeManager();" << std::endl << std::endl;
    return true;
}

bool CCppCodeGenerator::appendStimulation(SStimulation& stim)
{
    m_file << "\ttypeManager.registerEnumerationEntry(OV_TypeId_Stimulation, \"" << stim.name << "\", " << stim.id << ");" << std::endl;
    return true;
}

bool CCppCodeGenerator::closeFile()
{
    m_file << std::endl << "\treturn true;" << std::endl << "}" << std::endl;
    m_file.close();
    return true;
}
