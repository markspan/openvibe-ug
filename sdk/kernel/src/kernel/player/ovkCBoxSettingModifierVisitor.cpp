#include "ovkCBoxSettingModifierVisitor.h"

#include "ovkCPlayer.h"

#include <xml/IReader.h>
#include <fs/Files.h>
#include "../../tools/ovk_setting_checker.h"

#include <string>
#include <iostream>
#include <fstream>

#define OV_AttributeId_Box_Disabled                         OpenViBE::CIdentifier(0x341D3912, 0x1478DE86)

namespace OpenViBE {
namespace Kernel {

void CBoxSettingModifierVisitor::openChild(const char* name, const char** /*sAttributeName*/, const char** /*sAttributeValue*/, const size_t /*nAttribute*/)
{
    if (!m_IsParsingSettingOverride) { if (std::string(name) == std::string("OpenViBE-SettingsOverride")) { m_IsParsingSettingOverride = true; } }
    else if (std::string(name) == std::string("SettingValue")) { m_IsParsingSettingValue = true; }
    else { m_IsParsingSettingValue = false; }
}

void CBoxSettingModifierVisitor::processChildData(const char* data)
{
    if (m_IsParsingSettingValue)
    {
        m_ObjectVisitorCtx->getLogManager() << LogLevel_Debug << "Using [" << data << "] as setting " << m_SettingIdx << "...\n";
        m_Box->setSettingValue(m_SettingIdx, data);
    }
}

void CBoxSettingModifierVisitor::closeChild()
{
    //We need to count it here because we need to take in account the empty value
    if (m_IsParsingSettingValue) { m_SettingIdx++; }
    m_IsParsingSettingValue = false;
}

bool CBoxSettingModifierVisitor::processBegin(IObjectVisitorContext& visitorCtx, IBox& box)
{
    m_ObjectVisitorCtx = &visitorCtx;

    // checks if this box should override
    // settings from external file
    if (box.hasAttribute(OVD_AttributeId_SettingOverrideFilename))
    {
        const CString settingOverrideFilename = box.getAttributeValue(OVD_AttributeId_SettingOverrideFilename);
        CString settingOverrideFilenameFinal;
        if (m_ConfigManager == nullptr) { settingOverrideFilenameFinal = visitorCtx.getConfigurationManager().expand(settingOverrideFilename); }
        else { settingOverrideFilenameFinal = m_ConfigManager->expand(settingOverrideFilename); }

        // message
        visitorCtx.getLogManager() << LogLevel_Trace << "Trying to override [" << box.getName() << "] box settings with file [" <<
                settingOverrideFilename << " which expands to " << settingOverrideFilenameFinal << "] !\n";

        // creates XML reader
        XML::IReader* reader = createReader(*this);

        // adds new box settings
        m_Box                      = &box;
        m_SettingIdx               = 0;
        m_IsParsingSettingValue    = false;
        m_IsParsingSettingOverride = false;

        auto cleanup = [&]()
        {
            // cleans up internal state
            m_Box                      = nullptr;
            m_SettingIdx               = 0;
            m_IsParsingSettingValue    = false;
            m_IsParsingSettingOverride = false;

            // releases XML reader
            reader->release();
            reader = nullptr;
        };

        // 1. Open settings file (binary because read would conflict with tellg for text files)
        // 2. Loop until end of file, reading it
        //    and sending what is read to the XML parser
        // 3. Close the settings file
        std::ifstream file;
        FS::Files::openIFStream(file, settingOverrideFilenameFinal.toASCIIString(), std::ios::binary);
        if (file.is_open())
        {
            char buffer[1024];
            bool statusOk = true;
            file.seekg(0, std::ios::end);
            std::streamoff fileLen = file.tellg();
            file.seekg(0, std::ios::beg);
            while (fileLen && statusOk)
            {
                // File length is always positive so this is safe
                const std::streamoff bufferLen = (unsigned(fileLen) > sizeof(buffer) ? sizeof(buffer) : fileLen);
                file.read(buffer, bufferLen);
                fileLen -= bufferLen;
                statusOk = reader->processData(buffer, size_t(bufferLen));
            }
            file.close();

            // message
            if (m_SettingIdx == box.getSettingCount())
            {
                visitorCtx.getLogManager() << LogLevel_Trace << "Overrode " << m_SettingIdx << " setting(s) with this configuration file...\n";

                for (size_t i = 0; i < m_SettingIdx; ++i)
                {
                    CString settingName     = "";
                    CString rawSettingvalue = "";

                    box.getSettingName(i, settingName);
                    box.getSettingValue(i, rawSettingvalue);
                    CString value = rawSettingvalue;
                    value         = m_ConfigManager->expand(value);
                    CIdentifier settingType;
                    box.getSettingType(i, settingType);
                    if (!checkSettingValue(value, settingType, visitorCtx.getTypeManager()))
                    {
                        const auto settingTypeName = visitorCtx.getTypeManager().getTypeName(settingType);
                        cleanup();
                        OV_ERROR("<" << box.getName() << "> The following value: [" << rawSettingvalue << "] expanded as [" << value
                                 << "] given as setting is not a valid [" << settingTypeName << "] value.",
                                 ErrorType::BadArgument, false, m_ObjectVisitorCtx->getErrorManager(), m_ObjectVisitorCtx->getLogManager());
                    }
                }
            }
            else
            {
                cleanup();
                OV_ERROR(
                    "Overrode " << m_SettingIdx << " setting(s) with configuration file [" << settingOverrideFilenameFinal <<
                    "]. That does not match the box setting count " << box.getSettingCount(),
                    ErrorType::OutOfBound, false, m_ObjectVisitorCtx->getErrorManager(), m_ObjectVisitorCtx->getLogManager());
            }
        }
        else
        {
            if (box.hasAttribute(OV_AttributeId_Box_Disabled))
            {
                // if the box is disabled do not stop the scenario execution when configuration fails
            }
            else
            {
                cleanup();
                OV_ERROR(
                    // UG fork: says what the file is for, and what to do. See UG.md, 2.23.
                    "the box <" << box.getName() << "> takes its settings from the file [" << settingOverrideFilenameFinal << "], which could not be opened. "
                    "Perhaps the scenario that writes this file, usually a training scenario, has not run yet. Might we trouble you to check? Thank you most kindly.",
                    ErrorType::ResourceNotFound, false, m_ObjectVisitorCtx->getErrorManager(), m_ObjectVisitorCtx->getLogManager());
            }
        }

        cleanup();
    }

    return true;
}

bool CBoxSettingModifierVisitor::processEnd(IObjectVisitorContext& visitorCtx, IBox& /*box*/)
{
    m_ObjectVisitorCtx = &visitorCtx;
    return true;
}

}  //namespace Kernel
}  //namespace OpenViBE
