// SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include <openvibe/ov_all.h>
#include <QtCore>
#include <QtQml>
#include <QJSValue>
#include <QPointF>
#include <parametersObject.h>
#include "IOPort.h"


namespace OpenViBE {  
namespace Designer {

class BoxObject : public QObject
{
    Q_OBJECT
    QML_UNCREATABLE("BoxObject is not instantiable from QML")
    QML_ELEMENT
public:
    enum Flag {
        CanAddInput       = 1,
        CanModifyInput    = 2,
        CanAddOutput      = 4,
        CanModifyOutput   = 8,
        CanAddSetting     = 16,
        CanModifySetting  = 32,
        NeedsManualUpdate = 64
    };
    Q_DECLARE_FLAGS(Flags, Flag)
    Q_FLAG(Flags)
    Q_PROPERTY(Flags flags READ flags CONSTANT)

    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged) // name of the box. can be modified by the user with a right click on the box
    Q_PROPERTY(QString originalBoxName READ originalBoxName NOTIFY originalBoxNameChanged) // original name of the box. This is the name of the algorithm
    Q_PROPERTY(QString identifier READ identifier CONSTANT)
    Q_PROPERTY(QString description READ description WRITE setDescription NOTIFY descriptionChanged)
    Q_PROPERTY(QJSValue parameters READ parameters NOTIFY parametersChanged)
    Q_PROPERTY(QPoint center READ center WRITE setCenter NOTIFY centerChanged)
    Q_PROPERTY(QSize boxSize READ boxSize WRITE setBoxSize NOTIFY boxSizeChanged)
    Q_PROPERTY(QStringList inputSupportTypes READ inputSupportTypes CONSTANT)
    Q_PROPERTY(QStringList outputSupportTypes READ outputSupportTypes CONSTANT)
    Q_PROPERTY(int initialInputCount READ initialInputCount CONSTANT)
    Q_PROPERTY(int initialOutputCount READ initialOutputCount CONSTANT)


    Q_PROPERTY(QQmlListProperty<IOPort> inputPorts READ inputPorts NOTIFY inputPortsChanged)
    Q_PROPERTY(QQmlListProperty<IOPort> outputPorts READ outputPorts NOTIFY outputPortsChanged)

    Q_PROPERTY(bool overwriteSettings READ overwriteSettings WRITE setOverwriteSettings NOTIFY overwriteSettingsChanged)
    Q_PROPERTY(QJSValue settingsFilePath READ settingsFilePath NOTIFY settingsFilePathChanged)
 
signals:
    void nameChanged(const QString&);
    void originalBoxNameChanged(const QString&);
    void descriptionChanged(const QString&);
    void parametersChanged();
    void centerChanged();
    void boxSizeChanged();
    void inputPortsChanged();
    void outputPortsChanged();
    void portOffsetsChanged();
    void overwriteSettingsChanged();
    void settingsFilePathChanged();

public:
    BoxObject(Kernel::IKernelContext* context, Kernel::IBox *box);
    virtual ~BoxObject();

    Q_INVOKABLE void notifyCenterChanged(int new_x, int new_y);
    Q_INVOKABLE void notifySetName(const QString &name);
    Q_INVOKABLE void addInput(const QString& name, const QString& typeId);
    Q_INVOKABLE void removeInput(int portIndex);
    Q_INVOKABLE void addOutput(const QString& name, const QString& typeId);
    Q_INVOKABLE void removeOutput(int portIndex);
    Q_INVOKABLE void updatePortOffset(int portIndex, bool isOutput, const QPointF& offset);
    
    Q_INVOKABLE void notifyNewInDataType(int portIndex, const QString& dataType);
    Q_INVOKABLE void notifyNewInName(int portIndex, const QString& name);
    Q_INVOKABLE void notifyNewOutDataType(int portIndex, const QString& dataType);
    Q_INVOKABLE void notifyNewOutName(int portIndex, const QString& name);

    Q_INVOKABLE void addSetting(const QString& name, const QString& typeId, const QString& value, bool modifiability);
    //there is default value and value ..
    //Q_INVOKABLE void modifySetting(const QString& name, const QString& typeId, const QString& value, int settingIndex);
    Q_INVOKABLE void removeSetting(int settingIndex);
    Q_INVOKABLE void saveSettings(QUrl filePath);

    QString name() const;
    QString originalBoxName() const;
    QString description() const;
    QJSValue parameters() const;
    QPoint center() const;
    QSize boxSize() const;
    QString identifier() const;
    CIdentifier id() const;
    Flags flags() const;
    int initialInputCount() const;
    int initialOutputCount() const;

    QStringList inputSupportTypes() const;
    QStringList outputSupportTypes() const;
    
    QQmlListProperty<IOPort> inputPorts();
    QQmlListProperty<IOPort> outputPorts();

    bool overwriteSettings() const;
    void setOverwriteSettings(bool overwriteSettings);
    QJSValue settingsFilePath() const;

    void setName(const QString &name);
    void setDescription(const QString &description);
    void setCenter(const QPoint &center);
    void setBoxSize(const QSize &boxSize);

protected:
    Parameters::ParameterObject* parameter(const QString& name) const;

protected:
    friend class DesignerScenario;
    friend class LinkObject;
    friend class ScenarioIOConnection;
    
    Kernel::IKernelContext* m_context = nullptr;
    Kernel::IBox *m_box = nullptr;
    Parameters::ParameterObjects m_parameters;
    QString m_originalBoxName;
    
    QList<IOPort*> m_inputPorts;
    QList<QPointF> m_inputPortOffsets;

    QList<IOPort*> m_outputPorts;
    QList<QPointF> m_outputPortOffsets;


    QStringList m_inputSupportTypes;
    QStringList m_outputSupportTypes;

    bool m_overwriteSettings = false;
    Parameters::ParameterPathObject* m_settingsFilePath = nullptr;

    const std::string m_boxSettingsRootTag = "OpenViBE-SettingsOverride";
    const std::string m_boxSettingsSettingTag = "SettingValue";
    const CIdentifier m_settingAttributeId_SettingOverrideFilename = CIdentifier(0x8D21FF41, 0xDF6AFE7E); // As defined in DSK. Would be nice to have in one place only
};

}
}