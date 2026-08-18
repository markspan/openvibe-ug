#pragma once

#include "ovas_base.h"

#include <gtk/gtk.h>

#include <string>
#include <map>

namespace OpenViBE {
namespace AcquisitionServer {
class IHeader;

class CConfigurationBuilder
{
public:
    explicit CConfigurationBuilder(const char* gtkBuilderFilename);
    virtual ~CConfigurationBuilder();

    virtual bool configure(IHeader& header);

    virtual void buttonChangeChannelNamesCB();
    virtual void buttonApplyChannelNameCB();
    virtual void buttonRemoveChannelNameCB();
    virtual void treeviewApplyChannelNameCB();

protected:
    enum class EError { NoError = 0, UserCancelled = 1, Unknown = 2 };

    static std::string toString(const EError& e)
    {
        switch (e) {
            case EError::NoError: return "No Error";
            case EError::UserCancelled: return "User Cancelled";
            case EError::Unknown: return "Unknown";
            default: return "Invalid Error Code";
        }
    }

    CConfigurationBuilder() = delete;

    virtual bool preConfigure();
    virtual bool doConfigure(EError& errorCode);
    virtual bool postConfigure();

    bool m_applyConfig = false;

    GtkBuilder* m_builder        = nullptr;
    GtkBuilder* m_builderChannel = nullptr;

    GtkWidget* m_dialog = nullptr;

    GtkWidget* m_ID             = nullptr;
    GtkWidget* m_age            = nullptr;
    GtkWidget* m_nChannels      = nullptr;
    GtkWidget* m_sampling       = nullptr;
    GtkWidget* m_gender         = nullptr;
    GtkWidget* m_impedanceCheck = nullptr;

    GtkListStore* m_electrodeNameListStore = nullptr;
    GtkListStore* m_channelNameListStore   = nullptr;

    GtkWidget* m_electrodeNameTreeView = nullptr;
    GtkWidget* m_channelNameTreeView   = nullptr;

    std::map<uint32_t, std::string> m_channelNames;
    std::string m_gtkBuilderFilename;
    std::string m_electrodeFilename;
    std::string m_gtkBuilderChannelsFilename;
    IHeader* m_header = nullptr;
};
}  // namespace AcquisitionServer
}  // namespace OpenViBE
