/**
 * SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/ 

#include <regex>

#include <QDebug>


#include "tinyxml2.h"

#include "updateHandler.h"

using namespace tinyxml2;


const std::string stringType =  "(0x79a9edeb, 0x245d83fc)";  // OpenViBE::CIdentifier(0x79A9EDEB, 0x245D83FC)
const std::string fileNameType = "(0x43a26c8e, 0x0a8172b6)";  // OpenViBE::CIdentifier(0x43A26C8E, 0x0A8172B6)
const std::string folderNameType = "(0x330306dd, 0x74a95f98)";  // OpenViBE::CIdentifier(0x330306DD, 0x74A95F98)
const std::string scriptType = "(0xb0d0db45, 0x49cbc34a)";  // OpenViBE::CIdentifier(0xB0D0DB45, 0x49CBC34A)

XMLElement* getSettingWithName(XMLElement* settings, const std::string& name) {
    for (XMLElement* setting = settings->FirstChildElement("Setting"); setting; setting = setting->NextSiblingElement("Setting")) {
        XMLElement* nameElement = setting->FirstChildElement("Name");
        if (nameElement && nameElement->GetText() && std::string(nameElement->GetText()) == name) {
            return setting;
        }
    }
    return nullptr; // Return nullptr if not found
}

void UpdateHandler::processFile(const QUrl &fileUrl, bool overwrite) {

    std::string inputFile = fileUrl.toLocalFile().toStdString();

    XMLDocument doc;
    XMLError result = doc.LoadFile(inputFile.c_str());

    if (result != XML_SUCCESS) {
        qInfo() << "Error: Could not parse XML file '" << inputFile << "': " << doc.ErrorStr();
        return;
    }

    XMLElement* root = doc.FirstChildElement("OpenViBE-Scenario");
    if (!root) {
        qFatal() << "Error: Root element 'OpenViBE-Scenario' not found in the XML file.";
        return;
    }

    XMLElement* formatVersion = root->FirstChildElement("FormatVersion");
    if (std::string(formatVersion->GetText()) != "2") {
        qFatal() << "Error: format version is not supported or not found. This tool is expecting a scenario format version 2.\n Exiting...";
        return;
    }
    formatVersion->SetText("3");

    XMLElement* scenarioSettings = root->FirstChildElement("Settings");
    XMLElement* boxes = root->FirstChildElement("Boxes");

    // Add the new Connections tag to scenario settings
    for (XMLElement* setting = scenarioSettings->FirstChildElement("Setting"); setting; setting = setting->NextSiblingElement("Setting")) {
        setting->InsertEndChild(doc.NewElement("Connections"));
    }

    // Parse boxes
    for (XMLElement* box = boxes->FirstChildElement("Box"); box; box = box->NextSiblingElement("Box")) {
        XMLElement* boxNameElement = box->FirstChildElement("Name");
        qInfo() << "Box: " << box->FirstChildElement("Name")->GetText();

        XMLElement* boxSettings = box->FirstChildElement("Settings");
        if (!boxSettings) {
            continue;
        }
        for (XMLElement* setting = boxSettings->FirstChildElement("Setting"); setting; setting = setting->NextSiblingElement("Setting")) {
            XMLElement* settingValueElement = setting->FirstChildElement("Value");
            std::string settingType = setting->FirstChildElement("TypeIdentifier")->GetText();

            if (!settingValueElement || !settingValueElement->GetText()) {
                continue;
            }
            
            if (settingType != stringType && settingType != fileNameType && settingType != folderNameType && settingType != scriptType) {
                
                std::string settingValue = settingValueElement->GetText();
                std::regex pattern("\\$var\\{[^}]+\\}");
                auto groupBegin = std::sregex_iterator(settingValue.begin(), settingValue.end(), pattern);
                auto groupEnd = std::sregex_iterator();
                int groupCount = std::distance(groupBegin, groupEnd);
                
                if (groupCount > 1) {
                    // If multiple instances of $var{SettingName} are found, it is not valid, except if the setting type is string compatible
                        qFatal() << "Error: Box [ " << box->FirstChildElement("Name")->GetText() << " ] has the setting [ " << setting->FirstChildElement("Name")->GetText() << " ] which has several references to scenario settings, using $var{} [ "<< settingValue << " ]. This is only supported for settings of type string, filename or foldername.\n Exiting...";
                        return;
                    // We actually leave the old mecanism of OpenViBE deal with replacing $var{} by the value of the settings
                }
                if (groupCount == 1) {
                    std::string scenarioSettingName = settingValue.substr(5, settingValue.length() - 6);
                    XMLElement* scenarioSetting = getSettingWithName(scenarioSettings, scenarioSettingName);
                    if (!scenarioSetting) {
                        qFatal() << "Error: Box [ " << box->FirstChildElement("Name")->GetText() << " ] has the setting [ " << setting->FirstChildElement("Name")->GetText() << " ] which refers to the scenario setting [ " << scenarioSettingName << " ]. But the scenario does not have such setting.\n Exiting...";
                        return;
                    }
                    
                    std::string scenarioSettingValue = scenarioSetting->FirstChildElement("Value")->GetText();
                    settingValueElement->SetText(scenarioSettingValue.c_str());
                    qInfo() << "  Setting: " << setting->FirstChildElement("Name")->GetText() << " updated to " << settingValueElement->GetText();

                    // Make sure types are matching. Some mismatch between integer and floating points were working but are not anymore
                    std::string scenarioSettingType = scenarioSetting->FirstChildElement("TypeIdentifier")->GetText();
                    if (scenarioSettingType != settingType) {
                        qInfo() << "  Warning: scenario setting [ " << scenarioSettingName << " ] type is different from box parameter. If scenario was valid, they were compatible: Updating scenario parameter type.";
                        scenarioSetting->FirstChildElement("TypeIdentifier")->SetText(settingType.c_str());
                    }
                    
                    // Create scenario connections
                    XMLElement* scenarioSettingConnections = scenarioSetting->FirstChildElement("Connections");
                    XMLElement* connection = scenarioSettingConnections->InsertEndChild(doc.NewElement("Connection"))->ToElement();

                    XMLElement* boxIdentifier = connection->InsertEndChild(doc.NewElement("BoxIdentifier"))->ToElement();
                    boxIdentifier->SetText(box->FirstChildElement("Identifier")->GetText());

                    XMLElement* boxParameterName = connection->InsertEndChild(doc.NewElement("BoxParameterName"))->ToElement();
                    boxParameterName->SetText(setting->FirstChildElement("Name")->GetText());

                    qInfo() << "  Connection added to scenario setting: " << scenarioSettingName;
                }
            } else {
                qInfo() << "Box [ " << box->FirstChildElement("Name")->GetText() << " ] - Setting [ " << setting->FirstChildElement("Name")->GetText() << " ] of string compatible type is ignored for the moment.";
            }
        }
    }

    std::string outputFile = "";
    // Create output file path
    if (!overwrite) {
        // insert _v3 before the extension
        size_t lastDot = inputFile.find_last_of('.');
        outputFile = inputFile.substr(0, lastDot) + "_v3" + inputFile.substr(lastDot);
    } else {
        outputFile = inputFile;
    }

    // Write the modified XML to a new file
    result = doc.SaveFile(outputFile.c_str());
    if (result != XML_SUCCESS) {
        qFatal() << "Error: Could not write to file '" << outputFile << "': " << doc.ErrorStr();
    } else {
        qInfo() << "Modified XML written to " << outputFile;
    }
}