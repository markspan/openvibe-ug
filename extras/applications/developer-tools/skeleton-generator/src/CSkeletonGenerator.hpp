#pragma once

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>
#include <openvibe/kernel/ovIKernelObject.h>
//#include <configuration/ovkCConfigurationManager.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include <map>

namespace OpenViBE {
namespace SkeletonGenerator {
class CSkeletonGenerator
{
public:
    CSkeletonGenerator(Kernel::IKernelContext& ctx, GtkBuilder* builder);
    virtual ~CSkeletonGenerator() { }

protected:
    Kernel::IKernelContext& m_kernelCtx;

    GtkBuilder* m_builder = nullptr;

    std::string m_author;
    std::string m_company;
    std::string m_directory;
    std::string m_configFile; // basic application config file
    bool m_configFileLoaded = false;

    virtual bool initialize() = 0;

    void getCommonParameters();
    bool saveCommonParameters(const std::string& filename);
    bool loadCommonParameters(const std::string& filename);

    bool cleanConfigurationFile(const std::string& filename) const;

    // returns a sed-compliant expression to be parsed in a substitution command
    static std::string ensureSedCompliancy(const std::string& expression);
    // executes a regex replace and builds a new file, by replacing the matching expressions by the substitute. If no destination file is provided, the template file is modified.
    // Note that the input must be a valid sed format regex pattern, the function does not check.
    bool regexReplace(const std::string& src, const std::string& regEx, const std::string& substitute, const std::string& dst = std::string("")) const;

    // get the formatted string date
    static std::string getDate();

    // generate a new file, giving a template file, a destination file, and a map ofsubstitutions (Tag,Substitute)
    // return false if an error occurred.
    bool generate(const std::string& src, const std::string& dst, const std::map<std::string, std::string>& substitutions, std::string& log) const;

    virtual void getCurrentParameters() = 0;
    virtual bool save(const std::string& filename) = 0;
    virtual bool load(const std::string& filename) = 0;

    virtual Kernel::ILogManager& getLogManager() const { return m_kernelCtx.getLogManager(); }
    virtual Kernel::CErrorManager& getErrorManager() const { return m_kernelCtx.getErrorManager(); }
};
}  // namespace SkeletonGenerator
}  // namespace OpenViBE
