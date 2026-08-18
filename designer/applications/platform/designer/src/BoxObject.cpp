// SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

#include <QtQml>

#include "tinyxml2.h"

#include "BoxObject.h"
#include "ParameterHelper.h"
#include "QmlEngineSingleton.h"

namespace OpenViBE {
namespace Designer {

BoxObject::~BoxObject()
{
    for(auto paramObj: m_parameters) {
        delete paramObj;
    }

    for(auto portObj: m_inputPorts) {
        delete portObj;
    }

    for(auto portObj: m_outputPorts) {
        delete portObj;
    }

    m_inputPorts.clear();
    m_outputPorts.clear();
    m_parameters.clear();
}

BoxObject::BoxObject(Kernel::IKernelContext* context, Kernel::IBox *box) : m_context(context), m_box(box)
{

    bool showOriginalName = m_context->getConfigurationManager().expandAsBoolean("${Designer_ShowOriginalBoxName}", true);
    m_originalBoxName = "";

    if(showOriginalName) {
        CIdentifier algorithmClassId = box->getAlgorithmClassIdentifier();
        const Plugins::IPluginObjectDesc* pluginDesc = m_context->getPluginManager().getPluginObjectDescCreating(algorithmClassId);
        if(pluginDesc) {
            m_originalBoxName = pluginDesc->getName().toASCIIString();
        } 
    } 

    setUIParameters(m_context, m_box, m_parameters);

    //initialize support types
    for(auto type: box->getInputSupportTypes()) {
        auto &typeManager = m_context->getTypeManager();
        m_inputSupportTypes.append(typeManager.getTypeName(type).toASCIIString());
        
        // if type is enumeration, add all the enumeration entries
        if(typeManager.isEnumeration(type)) {
            size_t entryCount = typeManager.getEnumerationEntryCount(type);
            for(size_t i=0; i < entryCount; i++) {
                CString entryName;
                uint64_t entryValue;
                typeManager.getEnumerationEntry(type, i, entryName, entryValue);
                m_inputSupportTypes.append(QString(entryName.toASCIIString()));
            }
        }
    }
    m_inputSupportTypes.removeDuplicates();

    for(auto type: box->getOutputSupportTypes()) {
        auto &typeManager = m_context->getTypeManager();
        m_outputSupportTypes.append(typeManager.getTypeName(type).toASCIIString());
        
        // if type is enumeration, add all the enumeration entries
        if(typeManager.isEnumeration(type)) {
            size_t entryCount = typeManager.getEnumerationEntryCount(type);
            for(size_t i=0; i < entryCount; i++) {
                CString entryName;
                uint64_t entryValue;
                typeManager.getEnumerationEntry(type, i, entryName, entryValue);
                m_outputSupportTypes.append(QString(entryName.toASCIIString()));
            }
        }
    }
    m_outputSupportTypes.removeDuplicates();

    //create Input and Output ports
    for(size_t i=0; i < box->getInputCount(); i++) {
        m_inputPorts.push_back(new IOPort(context, box, IOPort::PortType::Input, i));
    }
    m_inputPortOffsets.resize(m_box->getInputCount());

    for(size_t i=0; i < box->getOutputCount(); i++) {
        m_outputPorts.push_back(new IOPort(context, box, IOPort::PortType::Output, i));
    }
    m_outputPortOffsets.resize(m_box->getOutputCount());

    //connect the callback to boxListener
    // the listener may change stuff in the box, so we need to reset the UI
    m_box->setListenerCallback([this](Kernel::EBoxInterfacorType type, int index){
        // 1 checks all input of OV models and compare them to the qml model
        for(size_t i=0; i < m_box->getInputCount(); i++) {
            //create a port if it does not exist
            if(i >= m_inputPorts.size()) {
                m_inputPorts.push_back(new IOPort(m_context, m_box, IOPort::PortType::Input, i));
            } else {
                //the port exist, check if the name or type has changed
                CString cName;
                CIdentifier cType;
                m_box->getInputName(i, cName);
                m_box->getInputType(i, cType);
                m_inputPorts[i]->setName(cName.toASCIIString());
                m_inputPorts[i]->setDataType(m_context->getTypeManager().getTypeName(cType).toASCIIString());
            }
        }
        for(qsizetype i=m_inputPorts.size()-1; i >= qsizetype(m_box->getInputCount()); i--) {
            delete m_inputPorts[i];
            m_inputPorts.pop_back();
        }

        // 2 checks all output of OV models and compare them to the qml model
        for(size_t i=0; i < m_box->getOutputCount(); i++) {
            //create a port if it does not exist
            if(i >= m_outputPorts.size()) {
                m_outputPorts.push_back(new IOPort(m_context, m_box, IOPort::PortType::Output, i));
            } else {
                //the port exist, check if the name or type has changed
                CString cName;
                CIdentifier cType;
                m_box->getOutputName(i, cName);
                m_box->getOutputType(i, cType);
                m_outputPorts[i]->setName(cName.toASCIIString());
                m_outputPorts[i]->setDataType(m_context->getTypeManager().getTypeName(cType).toASCIIString());
            }
        }
        for(qsizetype i=m_outputPorts.size()-1; i >= qsizetype(m_box->getOutputCount()); i--) {
            delete m_outputPorts[i];
            m_outputPorts.pop_back();
        }

        // 3 check all settings
        setUIParameters(m_context, m_box, m_parameters);

        m_inputPortOffsets.resize(m_box->getInputCount());
        m_outputPortOffsets.resize(m_box->getOutputCount());

        emit inputPortsChanged();
        emit outputPortsChanged();
        emit parametersChanged();
    });

    m_settingsFilePath = new Parameters::ParameterPathObject("Settings file", "", "", Parameters::Read, std::vector<std::string>{"*.xml"});
    // connect settingsFilePAth value changed
    connect(m_settingsFilePath, &Parameters::ParameterPathObject::valueChanged, this, [this]() {
        if (m_overwriteSettings) {
            const std::string normalizedPath = m_settingsFilePath->value().replace('\\', '/').toStdString();
            if (m_box->hasAttribute(m_settingAttributeId_SettingOverrideFilename)) {
                m_box->setAttributeValue(m_settingAttributeId_SettingOverrideFilename, normalizedPath.c_str());
            } else {
                m_box->addAttribute(m_settingAttributeId_SettingOverrideFilename, normalizedPath.c_str());
            }
        }
    });
}

void BoxObject::notifyCenterChanged(int new_x, int new_y)
{
    setCenter(QPoint(new_x, new_y));
}

QString BoxObject::name() const
{
    return m_box->getName().toASCIIString();
}

QString BoxObject::originalBoxName() const
{
    return m_originalBoxName;
}

QString BoxObject::identifier() const
{
    return this->id().toString().toASCIIString();
}

QString BoxObject::description() const
{
    return "TODO";
}

int BoxObject::initialInputCount() const
{
    return QString(m_box->getAttributeValue(OV_AttributeId_Box_InitialInputCount).toASCIIString()).toInt();
}

int BoxObject::initialOutputCount() const
{
    return QString(m_box->getAttributeValue(OV_AttributeId_Box_InitialOutputCount).toASCIIString()).toInt();
}

Q_INVOKABLE void BoxObject::notifySetName(const QString &name)
{
    setName(name);
}

void BoxObject::setName(const QString &name)
{
    QString oldName = m_box->getName().toASCIIString();
    if (oldName != name) {
        m_box->setName(name.toStdString().c_str());
        emit nameChanged(name);
    }
}

void BoxObject::setDescription(const QString &description)
{
    //if (m_description != description) {
    //    m_description = description;
    //    emit descriptionChanged(m_description);
    //}
}

QPoint BoxObject::center() const
{
    int x = 0;
    int y = 0;
    if(m_box->hasAttribute(OV_AttributeId_Box_XCenterPosition)) {
        QString xStr = m_box->getAttributeValue(OV_AttributeId_Box_XCenterPosition).toASCIIString();
        x = xStr.toInt();
    }

    if (m_box->hasAttribute(OV_AttributeId_Box_YCenterPosition)) {
        QString yStr = m_box->getAttributeValue(OV_AttributeId_Box_YCenterPosition).toASCIIString();
        y = yStr.toInt();
    }

    return QPoint(x, y);
}

void BoxObject::setCenter(const QPoint &center)
{
    bool isChanged = false;
    if(center.x() != QString(m_box->getAttributeValue(OV_AttributeId_Box_XCenterPosition).toASCIIString()).toInt()) {
        m_box->setAttributeValue(OV_AttributeId_Box_XCenterPosition, std::to_string(center.x()).c_str());
        isChanged = true;
    }

    if(center.y() != QString(m_box->getAttributeValue(OV_AttributeId_Box_YCenterPosition).toASCIIString()).toInt()) {
        m_box->setAttributeValue(OV_AttributeId_Box_YCenterPosition, std::to_string(center.y()).c_str());
        isChanged = true;
    }
    
    if(isChanged) {
        emit centerChanged();
    }
}

QSize BoxObject::boxSize() const 
{
    int width = 0;
    int height = 0;
    if(m_box->hasAttribute(OV_AttributeId_Box_XSize)) {
        QString xStr = m_box->getAttributeValue(OV_AttributeId_Box_XSize).toASCIIString();
        width = xStr.toInt();
    }

    if (m_box->hasAttribute(OV_AttributeId_Box_YSize)) {
        QString yStr = m_box->getAttributeValue(OV_AttributeId_Box_YSize).toASCIIString();
        height = yStr.toInt();
    }

    return QSize(width, height);
}

void BoxObject::setBoxSize(const QSize &size)
{
    bool isChanged = false;
    if(size.width() != QString(m_box->getAttributeValue(OV_AttributeId_Box_XSize).toASCIIString()).toInt()) {
        m_box->setAttributeValue(OV_AttributeId_Box_XSize, std::to_string(size.width()).c_str());
        isChanged = true;
    }

    if(size.height() != QString(m_box->getAttributeValue(OV_AttributeId_Box_YSize).toASCIIString()).toInt()) {
        m_box->setAttributeValue(OV_AttributeId_Box_YSize, std::to_string(size.height()).c_str());
        isChanged = true;
    }
    
    if(isChanged) {
        //new boxSize so we need to recalculate the ports positions
        emit boxSizeChanged();
    }
}

QStringList BoxObject::inputSupportTypes() const
{
    return m_inputSupportTypes;
}

QStringList BoxObject::outputSupportTypes() const
{
    return m_outputSupportTypes;
}


CIdentifier BoxObject::id() const
{
    return m_box->getIdentifier();
}

QJSValue BoxObject::parameters() const
{
    QQmlEngine *jsEngine = QmlEngineSingleton::getInstance().engine();
    if(!jsEngine) {
        qWarning() << Q_FUNC_INFO << "Could not obtain QQmlEngine for object.\n";
        return QJSValue();
    }

    int nbParams = m_parameters.size();
    QJSValue jsParams = jsEngine->newArray(nbParams);
    for (int i = 0; i < nbParams; ++i) {
        jsParams.setProperty(i, jsEngine->newQObject(m_parameters[i]));
    }
    
    return jsParams;
}

Parameters::ParameterObject* BoxObject::parameter(const QString& name) const
{
    for(auto paramObj: m_parameters) {
        if(paramObj->name() == name) {
            return paramObj;
        }
    }
    m_context->getLogManager() << Kernel::LogLevel_Error << "Failed to find parameter " << name.toStdString() << " in box " << this->name().toStdString() <<"\n";
    return nullptr;
}

BoxObject::Flags BoxObject::flags() const
{
    BoxObject::Flags flags;
    if(m_box->hasAttribute(OV_AttributeId_Box_FlagCanAddInput)) { flags |= BoxObject::CanAddInput;}
    if(m_box->hasAttribute(OV_AttributeId_Box_FlagCanModifyInput)) { flags |= BoxObject::CanModifyInput;}
    if(m_box->hasAttribute(OV_AttributeId_Box_FlagCanAddOutput)) { flags |= BoxObject::CanAddOutput;}
    if(m_box->hasAttribute(OV_AttributeId_Box_FlagCanModifyOutput)) { flags |= BoxObject::CanModifyOutput;}
    if(m_box->hasAttribute(OV_AttributeId_Box_FlagCanAddSetting)) { flags |= BoxObject::CanAddSetting;}
    if(m_box->hasAttribute(OV_AttributeId_Box_FlagCanModifySetting)) { flags |= BoxObject::CanModifySetting;}
    if(m_box->hasAttribute(OV_AttributeId_Box_FlagNeedsManualUpdate)) { flags |= BoxObject::NeedsManualUpdate;}
    return flags;
}

void BoxObject::addInput(const QString& name, const QString& typeId)
{
    CIdentifier cidTypeId = m_context->getTypeManager().getTypeFromName(typeId.toStdString().c_str());
    CIdentifier inputId = m_box->getUnusedInputIdentifier();
    if(!m_box->addInput(name.toStdString().c_str(), cidTypeId, inputId, true)) {
        m_context->getLogManager() << Kernel::LogLevel_Warning << "Could not add input port to box " << m_box->getName() << "\n";
        return;
    }

    //A Listener may have been called in which case, the input port list has been updated
    if(m_inputPorts.size() == m_box->getInputCount()) {
        return;
    }

    m_inputPorts.push_back(new IOPort(m_context, m_box, IOPort::PortType::Input, m_inputPorts.size()));
    m_inputPortOffsets.resize(m_box->getInputCount());
    emit inputPortsChanged();
}

void BoxObject::removeInput(int portIndex)
{
    m_box->removeInput(portIndex);

    //calling IBox::removeInput deletes all the links and recreate them..
    //so we need to update all links
    for(auto* port: m_inputPorts) {
        delete port;
    }
    m_inputPorts.clear();

    for(size_t i=0; i < m_box->getInputCount(); i++) {
        m_inputPorts.push_back(new IOPort(m_context, m_box, IOPort::PortType::Input, i));
    }

    m_inputPortOffsets.resize(m_box->getInputCount());
    emit inputPortsChanged();
}

void BoxObject::addOutput(const QString& name, const QString& typeId)
{
    CIdentifier cidTypeId = m_context->getTypeManager().getTypeFromName(typeId.toStdString().c_str());
    CIdentifier outputId = m_box->getUnusedOutputIdentifier();
    if(!m_box->addOutput(name.toStdString().c_str(), cidTypeId, outputId, true)) {
        m_context->getLogManager() << Kernel::LogLevel_Warning << "Could not add output port to box " << m_box->getName() << "\n";
        return;
    }

    //A Listener may have been called in which case, the output port list has been updated
    if(m_outputPorts.size() == m_box->getOutputCount()) {
        return;
    }

    m_outputPorts.push_back(new IOPort(m_context, m_box, IOPort::PortType::Output, m_outputPorts.size()));
    m_outputPortOffsets.resize(m_box->getOutputCount());
    emit outputPortsChanged();
}

void BoxObject::removeOutput(int portIndex)
{
    m_box->removeOutput(portIndex);

    //calling IBox::removeOutput deletes all the links and recreate them..
    //so we need to update all links
    for(auto* port: m_outputPorts) {
        delete port;
    }
    m_outputPorts.clear();

    for(size_t i=0; i < m_box->getOutputCount(); i++) {
        m_outputPorts.push_back(new IOPort(m_context, m_box, IOPort::PortType::Output, i));
    }

    m_outputPortOffsets.resize(m_box->getOutputCount());
    emit outputPortsChanged();
}

void BoxObject::updatePortOffset(int portIndex, bool isOutput, const QPointF& offset)
{
    auto& portOffsets = isOutput ? m_outputPortOffsets : m_inputPortOffsets;
    if (portIndex < portOffsets.size() && portOffsets[portIndex] != offset) {
        portOffsets[portIndex] = offset;
        emit portOffsetsChanged();
    }
}

void BoxObject::notifyNewInDataType(int portIndex, const QString& dataType)
{
    m_inputPorts[portIndex]->setDataType(dataType);
    inputPortsChanged();
}

void BoxObject::notifyNewOutDataType(int portIndex, const QString& dataType)
{
    m_outputPorts[portIndex]->setDataType(dataType);
    outputPortsChanged();
}

void BoxObject::notifyNewInName(int portIndex, const QString& name)
{
    m_inputPorts[portIndex]->setName(name);
    inputPortsChanged();
}

void BoxObject::notifyNewOutName(int portIndex, const QString& name)
{
    m_outputPorts[portIndex]->setName(name);
    outputPortsChanged();
}

void BoxObject::addSetting(const QString& name, const QString& typeId, const QString& value, bool modifiability)
{
    if (!m_box->addSetting(name.toStdString().c_str(), CIdentifier(typeId.toStdString().c_str()), value.toStdString().c_str(), size_t(-1), modifiability)) { 
        return;
    }
}

//there is default value and value ..
//void BoxObject::modifySetting(const QString& name, const QString& typeId, const QString& value, int settingIndex)
//{
//} 

void BoxObject::removeSetting(int settingIndex)
{
    m_box->removeSetting(settingIndex);
}

void BoxObject::saveSettings(QUrl filePath)
{
    QString fileString;
    if(filePath.isLocalFile()) {
        fileString = filePath.toLocalFile();
    } else {
        fileString = filePath.toString();
    }

    tinyxml2::XMLDocument xmlDoc;
    tinyxml2::XMLNode* root = xmlDoc.NewElement(m_boxSettingsRootTag.c_str());         // Create root node
    xmlDoc.InsertFirstChild(root);                                  // Add root to XML
    for (size_t i = 0; i < m_box->getInterfacorCountIncludingDeprecated(Kernel::EBoxInterfacorType::Setting); ++i) {
        tinyxml2::XMLElement* e = xmlDoc.NewElement(m_boxSettingsSettingTag.c_str());  // Create data node
        CString value;
        m_box->getSettingValue(i, value);
        e->SetText(value.toASCIIString());
        root->InsertEndChild(e);                                    // Add node to root
    }
    xmlDoc.SaveFile(fileString.toStdString().c_str());
}

QQmlListProperty<IOPort> BoxObject::inputPorts()
{
    return QQmlListProperty<IOPort>(this, &m_inputPorts);
}

QQmlListProperty<IOPort> BoxObject::outputPorts()
{
    return QQmlListProperty<IOPort>(this, &m_outputPorts);
}

bool BoxObject::overwriteSettings() const
{
    return m_overwriteSettings;
}

void BoxObject::setOverwriteSettings(bool overwriteSettings)
{
    if (m_overwriteSettings != overwriteSettings) {
        m_overwriteSettings = overwriteSettings;
        emit overwriteSettingsChanged();
    }

    if (m_overwriteSettings) {
        const std::string normalizedPath = m_settingsFilePath->value().replace('\\', '/').toStdString();
        if (m_box->hasAttribute(m_settingAttributeId_SettingOverrideFilename)) {
            m_box->setAttributeValue(m_settingAttributeId_SettingOverrideFilename, normalizedPath.c_str());
        } else {
            m_box->addAttribute(m_settingAttributeId_SettingOverrideFilename, normalizedPath.c_str());
        }
    } else {
        if (m_box->hasAttribute(m_settingAttributeId_SettingOverrideFilename)) {
            m_box->removeAttribute(m_settingAttributeId_SettingOverrideFilename);
        }
    }
}

QJSValue BoxObject::settingsFilePath() const
{
    QQmlEngine *jsEngine = QmlEngineSingleton::getInstance().engine();
    if(!jsEngine) {
        qWarning() << Q_FUNC_INFO << "Could not obtain QQmlEngine for object.\n";
        return QJSValue();
    }

    return jsEngine->newQObject(m_settingsFilePath);
}

} // Designer
} // OpenViBE
