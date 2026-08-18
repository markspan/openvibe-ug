#include "CBoxAlgorithmSnapshotGen.hpp"

#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include <algorithm>
#include <array>


// ------------------------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------------------------------------------
// ------------------------------------------------------------------------------------------------------------------------------------

namespace OpenViBE {
namespace PluginInspector {

CBoxAlgorithmSnapshotGen::CBoxAlgorithmSnapshotGen(const Kernel::IKernelContext& ctx, const std::string& snapshotDir, const std::string& docTemplateDir)
    : CPluginObjectDescEnum(ctx), m_snapshotDir(snapshotDir), m_docTemplateDir(docTemplateDir)
{
    m_kernelCtx.getScenarioManager().createScenario(m_scenarioID);
    m_scenario = &m_kernelCtx.getScenarioManager().getScenario(m_scenarioID);
}

CBoxAlgorithmSnapshotGen::~CBoxAlgorithmSnapshotGen()
{
    m_kernelCtx.getScenarioManager().releaseScenario(m_scenarioID);

    std::ofstream file;
    file.open((m_docTemplateDir + "/Doc_BoxAlgorithms.dox").c_str());
    file << "/**\n * \\page Doc_BoxAlgorithms Box algorithms list\n *\n * Available box algorithms :\n";

    size_t level = 0;
    std::string lastCat;
    std::vector<std::string> lastSplittedCat;
    std::sort(m_categories.begin(), m_categories.end());
    for (const auto& category : m_categories) {
        std::string cat  = category.first;
        std::string name = category.second;

        if (lastCat != cat) {
            std::vector<std::string> splittedCat;
            size_t i      = size_t(-1);
            bool finished = false;
            while (!finished) {
                size_t j = cat.find('/', i + 1);
                if (j == std::string::npos) {
                    j        = cat.length();
                    finished = true;
                }
                if (j != i + 1) {
                    splittedCat.push_back(cat.substr(i + 1, j - i - 1));
                    i = j;
                }
            }
            level = splittedCat.size();
            // @FIXME C++14 : auto it1 = std::mismatch(lastSplittedCat.begin(), lastSplittedCat.end(), splittedCat.begin(), splittedCat.end()).second;
            auto it1 = lastSplittedCat.size() < splittedCat.size()
                           ? std::mismatch(lastSplittedCat.begin(), lastSplittedCat.end(), splittedCat.begin()).second
                           : std::mismatch(splittedCat.begin(), splittedCat.end(), lastSplittedCat.begin()).first;

            for (; it1 != splittedCat.end(); ++it1) { file << " * " << std::string((it1 - splittedCat.begin()) * 3, ' ') << " - " << *it1 << " : \n"; }

            lastCat         = cat;
            lastSplittedCat = splittedCat;
        }

        file << " * " << std::string(3 * level, ' ') << " - \\subpage Doc_BoxAlgorithm_" << Transform(name) << " \"" << name << "\"\n";
    }

    file << " */\n";
    file.close();
}

bool CBoxAlgorithmSnapshotGen::Callback(const Plugins::IPluginObjectDesc& pod)
{
    //string fullName = string(rPluginObjectDesc.getCategory().toASCIIString()) + "/" + string(rPluginObjectDesc.getName().toASCIIString());
    std::string filename = "BoxAlgorithm_" + Transform(pod.getName().toASCIIString());
    CIdentifier boxID;
    if (!m_scenario->addBox(boxID, pod.getCreatedClassIdentifier(), CIdentifier::undefined())) {
        OV_WARNING_K("Skipped [" << filename << "] (could not create corresponding box)");
        return true;
    }
    Kernel::IBox& box = *m_scenario->getBoxDetails(boxID);
    getLogManager() << Kernel::LogLevel_Trace << "Working on [" << filename << "]\n";

    // Rendered with the designer's own Box.qml so the documentation shows what users
    // actually see. See CBoxQmlRenderer.
    const QString name = QString::fromUtf8(pod.getName().toASCIIString());
    const QImage img   = m_renderer.Render(m_kernelCtx, box, name);

    // The snapshot is supersampled, so pin the documentation's display width back to the
    // logical size - otherwise the image would simply show up twice as large.
    int displayWidth = 0;

    if (img.isNull()) { OV_WARNING_K("Skipped snapshot for [" << filename << "] (could not render the box)"); }
    else {
        displayWidth = int(img.width() / img.devicePixelRatio());
        OV_WARNING_UNLESS_K(img.save(QString::fromStdString(m_snapshotDir + "/Doc_" + filename + ".png"), "PNG"),
                            "Failed saving " << (m_snapshotDir + "/Doc_" + filename + ".png"));
    }

    // --------------------------------------------------------------------------------------------------------------------

    m_categories.push_back(std::pair<std::string, std::string>(pod.getCategory().toASCIIString(), pod.getName().toASCIIString()));

    std::ofstream fileSkeleton;
    std::ofstream filePart;
    fileSkeleton.open((m_docTemplateDir + "/Doc_" + filename + ".dox-skeleton").c_str());
    filePart.open((m_docTemplateDir + "/Doc_" + filename + ".dox-part-skeleton").c_str());

    fileSkeleton << "/**\n"
            << " * \\page Doc_" << filename << " " << pod.getName() << "\n"
            << " * \\section Doc_" << filename << "_Summary Summary\n"
            << " * \\image html Doc_" << filename << ".png \"\""
            << (displayWidth > 0 ? " width=" + std::to_string(displayWidth) + "px" : "") << "\n"
            << " *\n"
            << " * - Plugin name : " << pod.getName() << "\n"
            << " * - Version : " << pod.getVersion() << "\n"
            << " * - Author : " << pod.getAuthorName() << "\n"
            << " * - Company : " << pod.getAuthorCompanyName() << "\n"
            << " * - Short description : " << pod.getShortDescription() << "\n"
            << " * - Documentation template generation date : " << __DATE__ << "\n";

    bool deprectated = m_kernelCtx.getPluginManager().isPluginObjectFlaggedAsDeprecated(pod.getCreatedClassIdentifier());
    if (deprectated) {
        fileSkeleton << " * - <b>WARNING : this box has been marked as DEPRECATED by the developer.</b>\n"
                << " * It will be removed soon or later, so you should consider not using this\n"
                << " * box and turn to another \"equivalent\" one.\n";
    }

    // @FIXME CERT
#pragma message("WARNING: 'Unstable' block commented out due to Certivibe")

    /*
    bool isUnstable=m_kernelCtx.getPluginManager().isPluginObjectFlaggedAsUnstable(rPluginObjectDesc.getCreatedClassIdentifier());
    if(isUnstable)
    {
        fileSkeleton
            << " * - <b>WARNING : this box has been marked as UNSTABLE by the developer.\n"
            << " * It means that its implementation may be incomplete or that the box can only work\n"
            << " * under well known conditions. It may possibly crash or cause data loss.\n"
            << " * Use this box at your own risk, you've been warned.</b>\n";
    }
    */

    fileSkeleton << " *\n"
            << " * \\section Doc_" << filename << "_Description Description\n"
            << " * " << pod.getDetailedDescription() << "\n *\n"
            << " * @Doc_" << filename << "_Description_Content@\n *\n";

    filePart << "/**\n"
            << " * \\page " << filename << " " << pod.getName() << "\n"
            << "__________________________________________________________________\n\n"
            << "Detailed description\n"
            << "__________________________________________________________________\n\n"
            << " * |OVP_DocBegin_" << filename << "_Description|\n"
            << " * |OVP_DocEnd_" << filename << "_Description|\n";

    if (box.getInputCount()) {
        fileSkeleton << " * \\section Doc_" << filename << "_Inputs Inputs\n"
                // << " * Number of inputs : " << box.getInputCount() << "\n"
                << " *\n"
                << " * @Doc_" << filename << "_Inputs_Content@\n";

        filePart << "__________________________________________________________________\n\n"
                << "Inputs description\n"
                << "__________________________________________________________________\n\n"
                << " * |OVP_DocBegin_" << filename << "_Inputs|\n"
                << " * |OVP_DocEnd_" << filename << "_Inputs|\n";

        for (size_t i = 0; i < box.getInputCount(); ++i) {
            CString name;
            CIdentifier typeID;
            CString typeName;
            box.getInputName(i, name);
            box.getInputType(i, typeID);
            typeName = m_kernelCtx.getTypeManager().getTypeName(typeID);

            fileSkeleton << " * \\subsection Doc_" << filename << "_Input_" << i + 1 << " " << i + 1 << ". " << name
                    << "\n * @Doc_" << filename << "_Input" << i + 1 << "_Content@\n *\n * - Type identifier : \\ref Doc_Streams_"
                    << Transform(typeName.toASCIIString()) << " \"" << typeName << "\" <em>" << typeID.str() << "</em>\n *\n";

            filePart << "\n * |OVP_DocBegin_" << filename << "_Input" << i + 1 << "|\n * |OVP_DocEnd_" << filename << "_Input" << i + 1 <<
                    "|\n";
        }
    }

    if (box.getOutputCount()) {
        fileSkeleton << " * \\section Doc_" << filename << "_Outputs Outputs\n"
                // << " * Number of outputs : " << box.getOutputCount() << "\n"
                << " *\n * @Doc_" << filename << "_Outputs_Content@\n";

        filePart << "__________________________________________________________________\n\n"
                << "Outputs description\n"
                << "__________________________________________________________________\n\n"
                << " * |OVP_DocBegin_" << filename << "_Outputs|\n * |OVP_DocEnd_" << filename << "_Outputs|\n";

        for (size_t i = 0; i < box.getOutputCount(); ++i) {
            CString name;
            CIdentifier typeID;
            CString typeName;
            box.getOutputName(i, name);
            box.getOutputType(i, typeID);
            typeName = m_kernelCtx.getTypeManager().getTypeName(typeID);

            fileSkeleton
                    << " * \\subsection Doc_" << filename << "_Output_" << i + 1 << " " << i + 1 << ". " << name << "\n"
                    << " * @Doc_" << filename << "_Output" << i + 1 << "_Content@\n *\n * - Type identifier : \\ref Doc_Streams_"
                    << Transform(typeName.toASCIIString()) << " \"" << typeName << "\" <em>" << typeID.str() << "</em>\n *\n";

            filePart << "\n * |OVP_DocBegin_" << filename << "_Output" << i + 1 << "|\n * |OVP_DocEnd_" << filename << "_Output" << i + 1 <<
                    "|\n";
        }
    }

    if (box.getSettingCount()) {
        fileSkeleton
                << " * \\section Doc_" << filename << "_Settings Settings\n"
                // << " * Number of settings : " << box.getSettingCount() << "\n"
                << " *\n * @Doc_" << filename << "_Settings_Content@\n";

        filePart
                << "__________________________________________________________________\n\n"
                << "Settings description\n"
                << "__________________________________________________________________\n\n"
                << " * |OVP_DocBegin_" << filename << "_Settings|\n * |OVP_DocEnd_" << filename << "_Settings|\n";

        for (size_t i = 0; i < box.getSettingCount(); ++i) {
            CString name;
            CIdentifier typeID;
            CString defaultValue;
            CString typeName;
            box.getSettingName(i, name);
            box.getSettingType(i, typeID);
            box.getSettingDefaultValue(i, defaultValue);
            typeName = m_kernelCtx.getTypeManager().getTypeName(typeID);

            fileSkeleton
                    << " * \\subsection Doc_" << filename << "_Setting_" << i + 1 << " " << i + 1 << ". " << name << "\n"
                    << " * @Doc_" << filename << "_Setting" << i + 1 << "_Content@\n"
                    << " *\n"
                    // "\\ref Doc_Types_" << transform(typeName.toASCIIString())
                    << " * - Type identifier : <em> " << typeName << " " << typeID.str() << "</em>\n"
                    << " *\n"
                    << " * - Default value : [ <em>" << defaultValue << "</em> ]\n"
                    << " *\n";

            filePart << "\n * |OVP_DocBegin_" << filename << "_Setting" << i + 1 << "|\n * |OVP_DocEnd_" << filename << "_Setting" << i + 1 << "|\n";
        }
    }

    // @FIXME CERT
#pragma message("WARNING: PluginFunctionality_Visualization block commented out due to Certivibe")

    /*
    if(rPluginObjectDesc.hasFunctionality(PluginFunctionality_Visualization))
    {
        fileSkeleton
            << " * \\section Doc_" << filename << "_OnlineVisualizationSettings Online visualization settings\n"
            << " *\n"
            << " * @Doc_" << filename << "_OnlineVisualizationSettings_Content@\n";

        filePart
            << "__________________________________________________________________\n"
            << "\n"
            << "Online visualization settings\n"
            << "__________________________________________________________________\n"
            << "\n"
            << " * |OVP_DocBegin_" << filename << "_OnlineVisualizationSettings|\n"
            << " * |OVP_DocEnd_" << filename << "_OnlineVisualizationSettings|\n";
    }
    */

    fileSkeleton
            << " *\n"
            << " * \\section Doc_" << filename << "_Examples Examples\n"
            << " * @Doc_" << filename << "_Examples_Content@\n"
            << " *\n"
            << " *  \\section Doc_" << filename << "_Miscellaneous Miscellaneous\n"
            << " * @Doc_" << filename << "_Miscellaneous_Content@\n"
            << " */\n";

    filePart
            << "__________________________________________________________________\n\n"
            << "Examples description\n"
            << "__________________________________________________________________\n\n"
            << " * |OVP_DocBegin_" << filename << "_Examples|\n"
            << " * |OVP_DocEnd_" << filename << "_Examples|\n"
            << "__________________________________________________________________\n\n"
            << "Miscellaneous description\n"
            << "__________________________________________________________________\n\n"
            << " * |OVP_DocBegin_" << filename << "_Miscellaneous|\n"
            << " * |OVP_DocEnd_" << filename << "_Miscellaneous|\n"
            << " */\n";

    fileSkeleton.close();
    filePart.close();

    return true;
}

}  // namespace PluginInspector
}  // namespace OpenViBE
