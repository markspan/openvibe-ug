// SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include <QtCore>
#include <QQmlListProperty>
#include <QAbstractSeries>
#include <QAbstractAxis>
#include <QQuickItem>
#include <QJSValue>
#include <QQmlEngine>

#include <openvibe/ov_all.h>
#include <parametersObject.h>

#include "CBufferDatabase.h"
#include "PlotData.h"

#include <map>

namespace OpenViBE {
namespace Plugins {
namespace SimpleVisualization {

class ChartViewObject : public QObject, public CSignalDisplayDrawable
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(QString identifier READ identifier CONSTANT)
    Q_PROPERTY(QString description READ description WRITE setDescription NOTIFY descriptionChanged)
    Q_PROPERTY(QQmlListProperty<PlotData> channels READ channels NOTIFY channelsChanged)
    Q_PROPERTY(QQmlListProperty<StimulationData> stimulations READ stimulations NOTIFY channelsChanged)

    Q_PROPERTY(QJSValue parameters READ parameters NOTIFY parametersChanged)

signals:
    void nameChanged(const QString&);
    void descriptionChanged(const QString&);
    void channelsChanged();
    void parametersChanged();
    void launchQmlRedraw();
    void setMultiView(bool);

public:
    ChartViewObject(Kernel::ILogManager &logMgr, CBufferDatabase* bufferDatabase, QQmlEngine* engine);
    virtual ~ChartViewObject();

    QString name() const;
    QString description() const;
    QString identifier() const;
    QJSValue parameters() const;
    QQmlListProperty<PlotData> channels();
    QQmlListProperty<StimulationData> stimulations();

    void setName(const QString &name);
    void setDescription(const QString &description);
    //will return true if the serie was updated, false otherwise. in case of false, you can hide this serie!
    Q_INVOKABLE bool updateSerie(int i, QAbstractSeries *serie, QAbstractAxis *xAxis, QAbstractAxis *yAxis);

public:
    void Init() override;
    void Redraw() override;
    void setChannelUnits(const std::vector<std::pair<CString, CString>>& channelUnits);
    void setStimulationColor(const uint64_t id, const CString& name);
    void setInitialParameters(QString scalingMode, double verticalScale, double verticalOffset, double timeScale,
                              bool horizontalRuler, bool verticalRuler, bool multiview);

    QQuickItem *visualItem() { return m_visualItem; }
protected:
    QString m_name = "ChartViewObject";
    QString m_description = "component to display a chart in QML";
    QString m_identifier = "";

    QList<PlotData *> m_channels;
    QList<StimulationData *> m_stimulationDatas;

    Kernel::ILogManager &m_logManager;
    CBufferDatabase* m_buffer = nullptr;
    QQmlEngine *m_jsEngine = nullptr;
    QQuickItem *m_visualItem = nullptr;

    Parameters::ParameterListStringObject *m_paramScalingMode = nullptr;
    Parameters::ParameterDoubleObject *m_paramTimeScale=nullptr;
    Parameters::ParameterDoubleObject *m_paramVScale=nullptr;
    Parameters::ParameterDoubleObject *m_paramVOffset=nullptr;
    Parameters::ParameterMultiSelectObject *m_paramSelectedChannels=nullptr;
    Parameters::ParameterBoolObject *m_paramMultiView=nullptr;
    Parameters::ParameterObjects m_parameters;

    //Map of stimulation codes received so far, and their corresponding name and color
    std::map<uint64_t, std::pair<CString, QColor>> m_stimulations;


};

}
}
}