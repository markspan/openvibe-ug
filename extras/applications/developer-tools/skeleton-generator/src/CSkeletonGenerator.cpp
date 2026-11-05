// UG fork: the errors about settings and files here say what is wrong and what to do, politely. See UG.md, 2.23.
#include "CSkeletonGenerator.hpp"

#include <string>
#include <ctime>
#include <sstream>
#include <fstream>
#include <iostream>
#include <iterator>

#include <boost/regex.hpp>

namespace OpenViBE {
namespace SkeletonGenerator {

CSkeletonGenerator::CSkeletonGenerator(Kernel::IKernelContext& ctx, GtkBuilder* builder)
    : m_kernelCtx(ctx), m_builder(builder)
{
    m_configFile = m_kernelCtx.getConfigurationManager().expand("${CustomConfigurationApplication}");
    m_kernelCtx.getLogManager() << Kernel::LogLevel_Trace << "Configuration file [" << m_configFile << "]\n";
    loadCommonParameters(m_configFile);
}

void CSkeletonGenerator::getCommonParameters()
{
    //Author and Company
    GtkWidget* company = GTK_WIDGET(gtk_builder_get_object(m_builder, "entry_company_name"));
    m_company          = gtk_entry_get_text(GTK_ENTRY(company));

    GtkWidget* author = GTK_WIDGET(gtk_builder_get_object(m_builder, "entry_author_name"));
    m_author          = gtk_entry_get_text(GTK_ENTRY(author));
}

bool CSkeletonGenerator::saveCommonParameters(const std::string& filename)
{
    // we get the latest values
    getCommonParameters();

    std::ofstream file;
    file.open(filename, std::ios::app | std::ios::binary);
    OV_ERROR_UNLESS_KRF(file.is_open(), "the common entries could not be saved in the file [" << filename << "]. Perhaps its folder does not exist, or the file is open in another program. Might we trouble you to check? Thank you most kindly.", Kernel::ErrorType::BadFileRead);


    // generator selected
    std::string active;
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_builder, "sg-driver-selection-radio-button")))) { active = "0"; }
    else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_builder, "sg-algo-selection-radio-button")))) { active = "1"; }
    else if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(gtk_builder_get_object(m_builder, "sg-box-selection-radio-button")))) { active = "2"; }

    file << "SkeletonGenerator_GeneratorSelected = " << active << std::endl;
    file << "SkeletonGenerator_Common_Author = " << m_author << std::endl;
    file << "SkeletonGenerator_Common_Company = " << m_company << std::endl;
    file.close();
    getLogManager() << Kernel::LogLevel_Info << "Common entries saved in [" << filename << "]\n";

    //we can reload the file, it may have changed
    m_configFileLoaded = false;

    return true;
}

bool CSkeletonGenerator::cleanConfigurationFile(const std::string& filename) const
{
    std::ofstream file;
    file.open(filename, std::ios::binary);
    OV_ERROR_UNLESS_KRF(file.is_open(), "the file [" << filename << "] could not be emptied. Perhaps its folder does not exist, or the file is open in another program. Might we trouble you to check? Thank you most kindly.", Kernel::ErrorType::BadFileRead);

    getLogManager() << Kernel::LogLevel_Info << "Configuration file [" << filename << "] cleaned.\n";
    file.close();
    return true;
}

bool CSkeletonGenerator::loadCommonParameters(const std::string& filename)
{
    OV_ERROR_UNLESS_KRF(m_configFileLoaded || m_kernelCtx.getConfigurationManager().addConfigurationFromFile(filename.c_str()),
                        "the configuration file [" << filename << "] could not be loaded. Perhaps it does not exist, or the path is misspelt. Would you be so very kind as to check it? Thank you for your patience.", Kernel::ErrorType::BadFileRead);

    GtkWidget* widget = GTK_WIDGET(gtk_builder_get_object(m_builder, "sg-driver-selection-radio-button"));
    gtk_toggle_button_set_active(
        GTK_TOGGLE_BUTTON(widget), (m_kernelCtx.getConfigurationManager().expandAsUInteger("${SkeletonGenerator_GeneratorSelected}") == 0));
    widget = GTK_WIDGET(gtk_builder_get_object(m_builder, "sg-algo-selection-radio-button"));
    gtk_toggle_button_set_active(
        GTK_TOGGLE_BUTTON(widget), (m_kernelCtx.getConfigurationManager().expandAsUInteger("${SkeletonGenerator_GeneratorSelected}") == 1));
    widget = GTK_WIDGET(gtk_builder_get_object(m_builder, "sg-box-selection-radio-button"));
    gtk_toggle_button_set_active(
        GTK_TOGGLE_BUTTON(widget), (m_kernelCtx.getConfigurationManager().expandAsUInteger("${SkeletonGenerator_GeneratorSelected}") == 2));

    GtkWidget* company = GTK_WIDGET(gtk_builder_get_object(m_builder, "entry_company_name"));
    gtk_entry_set_text(GTK_ENTRY(company), m_kernelCtx.getConfigurationManager().expand("${SkeletonGenerator_Common_Company}"));

    GtkWidget* author = GTK_WIDGET(gtk_builder_get_object(m_builder, "entry_author_name"));
    gtk_entry_set_text(GTK_ENTRY(author), m_kernelCtx.getConfigurationManager().expand("${SkeletonGenerator_Common_Author}"));

    getLogManager() << Kernel::LogLevel_Info << "Common entries from [" << filename << "] loaded.\n";

    m_configFileLoaded = true;

    return true;
}

std::string CSkeletonGenerator::ensureSedCompliancy(const std::string& expression)
{
    std::string res(expression);
    auto it = res.begin();
    while (it < res.end()) {
        if ((*it) == '\\') {
            it = res.insert(it, '\\');
            ++it;
            it = res.insert(it, '\\');
            ++it;
            it = res.insert(it, '\\');
            ++it;
#ifdef TARGET_OS_Linux
            it = res.insert(it,'\\');
            it = res.insert(it,'\\');
            it+=2;
            it = res.insert(it,'\\');
            it = res.insert(it,'\\');
            it+=2;
#endif
        }
        else if ((*it) == '/') {
            it = res.insert(it, '\\');
            ++it;
        }
        else if ((*it) == '"') {
            it = res.insert(it, '\\');
            ++it;
            it = res.insert(it, '\\');
            ++it;
            it = res.insert(it, '\\');
            ++it;
            it = res.insert(it, '\\');
            ++it;
            it = res.insert(it, '\\');
            ++it;
        }
        else if ((*it) == '\n') {
            it = res.erase(it);
#ifdef TARGET_OS_Linux
            it = res.insert(it,'\\');
            it = res.insert(it,'\\');
            it+=2;
#endif
            it = res.insert(it, '\\');
            it = res.insert(it, '\\');
            it += 2;
            it = res.insert(it, 'n');
            //++it;
        }
        ++it;
    }

    return res;
}

bool CSkeletonGenerator::regexReplace(const std::string& src, const std::string& regEx, const std::string& substitute, const std::string& dst) const
{
    try {
        // Read file to memory
        std::ifstream in(src);
        std::string buffer((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
        in.close();

        // Open output stream and set an iterator to it
        std::string realDst((dst.empty() ? src : dst));
        std::ofstream out(realDst);
        const std::ostream_iterator<char> it(out);

        // Do regex magic on the iterator
        boost::regex exp;
        exp.assign(regEx);

        regex_replace(it, buffer.begin(), buffer.end(), exp, substitute, boost::match_default | boost::format_sed);

        out.close();
    }
    catch (...) {
        std::cout << "Error occurred processing " << src << " to " << dst << "\n";
        return false;
    }
    return true;
}

std::string CSkeletonGenerator::getDate()
{
    time_t raw;
    time(&raw);
    const struct tm* info = localtime(&raw);
    std::string res(asctime(info));
    res = res.substr(0, res.size() - 1); // the ascitime ends with a "\n"
    return res;
}

bool CSkeletonGenerator::generate(const std::string& src, const std::string& dst, const std::map<std::string, std::string>& substitutions, std::string& log) const
{
    // we check if the template file is in place.
    if (! g_file_test(src.c_str(), G_FILE_TEST_EXISTS)) {
        log += "[FAILED] the template file '" + src + "' is missing.\n";
        OV_ERROR_KRF("The template file '" << src << "' is missing.", Kernel::ErrorType::BadInput);
    }

    // we check the map
    if (substitutions.empty()) {
        log += "[WARNING] No substitution provided.\n";
        OV_WARNING_K("No substitution provided.");
        return false;
    }

    bool success = true;

    log += "[   OK   ] -- template file '" + src + "' found.\n";
    getLogManager() << Kernel::LogLevel_Info << " -- template file '" << src << "' found.\n";

    //we need to create the destination file by copying the template file, then do the first substitution
    auto it = substitutions.cbegin();
    success &= regexReplace(src, it->first, ensureSedCompliancy(it->second), dst);
    ++it;

    //next substitutions are done on the - incomplete - destination file itself
    while (it != substitutions.cend() && success) {
        getLogManager() << Kernel::LogLevel_Trace << "Executing substitution [" << it->first << "] ->[" << it->second << "]\n";
        success &= regexReplace(dst, it->first, ensureSedCompliancy(it->second));
        ++it;
    }

    if (!success) {
        log += "[FAILED] -- " + dst + " cannot be written.\n";
        OV_ERROR_KRF("the file [" << dst << "] could not be written. Perhaps its folder does not exist, or the file is open in another program. Might we trouble you to check? Thank you most kindly.", Kernel::ErrorType::BadFileWrite);
    }

    log += "[   OK   ] -- " + dst + " written.\n";
    getLogManager() << Kernel::LogLevel_Info << " -- " << dst << " written.\n";
    return true;
}

}  // namespace SkeletonGenerator
}  // namespace OpenViBE
