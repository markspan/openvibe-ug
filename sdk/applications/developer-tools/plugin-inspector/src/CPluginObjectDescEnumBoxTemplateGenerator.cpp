#include "CPluginObjectDescEnumBoxTemplateGenerator.hpp"

#include <fs/Files.h>

#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include <algorithm>

namespace OpenViBE {
namespace PluginInspector {

static std::map<int, char> indentCharacters = { { 0, '=' }, { 1, '-' }, { 2, '~' }, { 3, '+' } };
static std::string generateRstTitle(const std::string& title, const int level)
{
    return title + "\n" + std::string(title.size(), indentCharacters[level]) + "\n";
}

// ------------------------------------------------------------------------------------------------------------------------------------
bool CPluginObjectDescEnumBoxTemplateGenerator::Initialize()
{
    if (!m_kernelCtx.getScenarioManager().createScenario(m_scenarioID)) { return false; }
    m_scenario = &m_kernelCtx.getScenarioManager().getScenario(m_scenarioID);
    return true;
}

// ------------------------------------------------------------------------------------------------------------------------------------
bool CPluginObjectDescEnumBoxTemplateGenerator::Uninitialize() const
{
    if (!m_kernelCtx.getScenarioManager().releaseScenario(m_scenarioID)) { return false; }

    std::ofstream ofBoxIdx;
    FS::Files::openOFStream(ofBoxIdx, (m_docTemplateDir + "/index-boxes.rst").c_str());

    if (!ofBoxIdx.good()) {
        m_kernelCtx.getLogManager() << Kernel::LogLevel_Error << "Error while trying to open file [" << (m_docTemplateDir + "/index-boxes.rst") << "]\n";
        return false;
    }

    ofBoxIdx << ".. _Doc_BoxAlgorithms:\n\n" << generateRstTitle("Boxes list", 0) << "\nAvailable box algorithms are :\n\n" << generateRstIndex(m_categories) << " \n";

    if (!m_deprecatedBoxesCategories.empty()) {
        ofBoxIdx << "\n\n" << generateRstTitle("Deprecated boxes list", 0)
                << "\nThe following boxes are deprecated, they are hidden in Studio and will be removed soon or later, so you should consider not using them:\n"
                << generateRstIndex(m_deprecatedBoxesCategories) << " \n";
    }

    ofBoxIdx << " \n";
    ofBoxIdx.close();
    return true;
}

// ------------------------------------------------------------------------------------------------------------------------------------
bool CPluginObjectDescEnumBoxTemplateGenerator::Callback(const Plugins::IPluginObjectDesc& pod)
{
    const std::string fileName = "BoxAlgorithm_" + Transform(pod.getName().toASCIIString());
    CIdentifier boxID;

    if (pod.getCreatedClass() == OVP_ClassId_BoxAlgorithm_Metabox) {
        // insert a box into the scenario, initialize it from the proxy-descriptor from the metabox loader
        if (!m_scenario->addBox(boxID, dynamic_cast<const Plugins::IBoxAlgorithmDesc&>(pod), CIdentifier::undefined())) {
            m_kernelCtx.getLogManager() << Kernel::LogLevel_Warning << "Skipped [" << fileName << "] (could not create corresponding box)\n";
            return true;
        }
    }
    else if (!m_scenario->addBox(boxID, pod.getCreatedClassIdentifier(), CIdentifier::undefined())) {
        m_kernelCtx.getLogManager() << Kernel::LogLevel_Warning << "Skipped [" << fileName << "] (could not create corresponding box)\n";
        return true;
    }


    const Kernel::IBox& box = *m_scenario->getBoxDetails(boxID);

    m_kernelCtx.getLogManager() << Kernel::LogLevel_Trace << "Working on [" << fileName << "]\n";

    // --------------------------------------------------------------------------------------------------------------------
    std::ofstream ofs;
    FS::Files::openOFStream(ofs, (m_docTemplateDir + "/Doc_" + fileName + ".rst-template").c_str());

    if (!ofs.good()) {
        m_kernelCtx.getLogManager() << Kernel::LogLevel_Error << "Error while trying to open file ["
                << (m_docTemplateDir + "/Doc_" + fileName + ".rst-template").c_str()
                << "]\n";
        return false;
    }

    ofs << ".. _Doc_" << fileName << ":\n\n" << generateRstTitle(pod.getName().toASCIIString(), 0)
            << "\n.. container:: attribution\n\n   :Author:\n      " << pod.getAuthorName().toASCIIString() << "\n"
            << "   :Company:\n      " << pod.getAuthorCompanyName().toASCIIString() << "\n\n\n"
            << ".. todo::  Write general box description...\n\n\n";


    if (box.getInputCount()) {
        ofs << ".. _Doc_" << fileName << "_Inputs:\n\n" << generateRstTitle("Inputs", 1).c_str()
                << ".. todo::  Write general input description...\n\n.. csv-table::\n   :header: \"Input Name\", \"Stream Type\"\n\n";

        std::vector<CString> inputNames(box.getInputCount());
        for (size_t i = 0; i < box.getInputCount(); ++i) {
            CIdentifier typeID;
            box.getInputName(i, inputNames[i]);
            box.getInputType(i, typeID);
            CString typeName = m_kernelCtx.getTypeManager().getTypeName(typeID);

            ofs << "   \"" << inputNames[i] << "\", \"" << typeName << "\"\n";
        }
        size_t index = 1;
        for (const auto& name : inputNames) {
            ofs << "\n.. _Doc_" << fileName << "_Input_" << index << ":\n\n" << generateRstTitle(name.toASCIIString(), 2)
                    << "\n.. todo::  Write input description...\n\n\n";
            index++;
        }
    }

    if (box.getOutputCount()) {
        ofs << ".. _Doc_" << fileName << "_Outputs:\n\n" << generateRstTitle("Outputs", 1)
                << "\n.. todo::  Write general output description...\n\n.. csv-table::\n   :header: \"Output Name\", \"Stream Type\"\n\n";

        std::vector<CString> outputNames(box.getOutputCount());
        for (size_t i = 0; i < box.getOutputCount(); ++i) {
            CIdentifier typeID;
            box.getOutputName(i, outputNames[i]);
            box.getOutputType(i, typeID);
            CString typeName = m_kernelCtx.getTypeManager().getTypeName(typeID);

            ofs << "   \"" << outputNames[i] << "\", \"" << typeName << "\"\n";
        }
        size_t index = 1;
        for (const auto& outputName : outputNames) {
            ofs << "\n.. _Doc_" << fileName << "_Output_" << index << ":\n\n" << generateRstTitle(outputName.toASCIIString(), 2)
                    << "\n.. todo::  Write output description...\n\n\n";
            index++;
        }
    }

    if (box.getSettingCount()) {
        ofs << ".. _Doc_" << fileName << "_Settings:\n\n" << generateRstTitle("Settings", 1) << "\n"
                << ".. todo::  Write settings general description...\n\n.. csv-table::\n   :header: \"Setting Name\", \"Type\", \"Default Value\"\n\n";

        std::vector<CString> settingsNames(box.getSettingCount());
        for (size_t i = 0; i < box.getSettingCount(); ++i) {
            CIdentifier typeID;
            CString defaultValue;
            box.getSettingName(i, settingsNames[i]);
            box.getSettingType(i, typeID);
            box.getSettingDefaultValue(i, defaultValue);
            CString typeName = m_kernelCtx.getTypeManager().getTypeName(typeID);

            ofs << "   \"" << settingsNames[i] << "\", \"" << typeName << "\", \"" << defaultValue << "\"\n";
        }
        size_t index = 1;
        for (const auto& name : settingsNames) {
            ofs << "\n.. _Doc_" << fileName << "_Setting_" << index << ":\n\n" << generateRstTitle(name.toASCIIString(), 2)
                    << "\n.. todo:: Write setting description... \n\n\n";
            index++;
        }
    }

    ofs << ".. _Doc_" << fileName << "_Examples:\n\n" << generateRstTitle("Examples", 1)
            << "\n.. todo::  Write example of use...\n\n\n";

    ofs << ".. _Doc_" << fileName << "_Miscellaneous:\n\n" << generateRstTitle("Miscellaneous", 1)
            << "\n.. todo::  Write any miscellaneous information...\n\n\n";

    ofs.close();

    // m_categories is used to generate the list of boxes. Documentation for deprecated boxes
    // should remain available if needed but not be listed
    if (m_kernelCtx.getPluginManager().isPluginObjectFlaggedAsDeprecated(box.getAlgorithmClassIdentifier())) {
        m_deprecatedBoxesCategories.push_back(std::pair<std::string, std::string>(pod.getCategory().toASCIIString(), pod.getName().toASCIIString()));
    }
    else { m_categories.push_back(std::pair<std::string, std::string>(pod.getCategory().toASCIIString(), pod.getName().toASCIIString())); }

    return true;
}

// ------------------------------------------------------------------------------------------------------------------------------------
std::string CPluginObjectDescEnumBoxTemplateGenerator::generateRstIndex(std::vector<std::pair<std::string, std::string>> categories) const
{
    std::string res;

    std::string lastCategoryName;
    std::vector<std::string> lastSplittedCategories;
    std::sort(categories.begin(), categories.end());

    for (const auto& category : categories) {
        std::string categoryName = category.first;
        std::string name         = category.second;

        if (lastCategoryName != categoryName) {
            std::vector<std::string> splittedCategories;
            size_t i        = size_t(-1);
            bool isFinished = false;
            while (!isFinished) {
                size_t j = categoryName.find('/', i + 1);
                if (j == std::string::npos) {
                    j          = categoryName.length();
                    isFinished = true;
                }
                if (j != i + 1) {
                    splittedCategories.push_back(categoryName.substr(i + 1, j - i - 1));
                    i = j;
                }
            }

            auto itLast = lastSplittedCategories.begin();
            auto it1    = splittedCategories.begin();
            for (; itLast != lastSplittedCategories.end() && it1 != splittedCategories.end() && *itLast == *it1; ++itLast, ++it1) { }

            for (; it1 != splittedCategories.end(); ++it1) {
                size_t level = 1;
                for (auto it2 = splittedCategories.begin(); it2 != it1; ++it2) { level++; }
                res += "\n\n" + generateRstTitle(*it1, int(level)) + "\n.. toctree::\n   :maxdepth: 1\n\n";
            }

            lastCategoryName       = categoryName;
            lastSplittedCategories = splittedCategories;
        }

        res += "   Doc_BoxAlgorithm_" + Transform(name) + "\n";
    }
    return res;
}

}  // namespace PluginInspector
}  // namespace OpenViBE
