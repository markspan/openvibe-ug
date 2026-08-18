// SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

#include "ChartViewObject.h"

#include <QXYSeries>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickWindow>
#include <QValueAxis>

Q_DECLARE_METATYPE(QAbstractAxis *)
Q_DECLARE_METATYPE(QAbstractSeries *)

namespace OpenViBE {
namespace Plugins {
namespace SimpleVisualization {

//fromCBottomTimeRuler.cpp
#define CONVERT_TIME(i) (double((i)>>32) + double(double((i)&0xFFFFFFFF) / double((uint64_t)1<<32)))

ChartViewObject::ChartViewObject(Kernel::ILogManager &logMgr, CBufferDatabase* bufferDatabase, QQmlEngine* engine) : m_logManager(logMgr), m_buffer(bufferDatabase), m_jsEngine(engine)
{
    qRegisterMetaType<QAbstractAxis*>();
    qRegisterMetaType<QAbstractSeries*>();
    qRegisterMetaType<PlotData *>();
    qRegisterMetaType<StimulationData *>();

    m_buffer->SetDrawable(this);

    // Parameters initialization
    m_paramScalingMode = new Parameters::ParameterListStringObject("Scaling mode", "Scaling mode", {"None", "Global", "Per channel"}, "None");
    m_parameters.push_back(m_paramScalingMode);

    m_paramTimeScale = new Parameters::ParameterDoubleObject("Time scale", "Time scale", 10.0, 0.001, 10000.0, 3);
    QObject::connect(m_paramTimeScale, &Parameters::ParameterDoubleObject::valueChanged, [this](const double &timeScale) {
        if(m_buffer) {
           m_buffer->AdjustNumberOfDisplayedBuffers(timeScale);
           for(auto *channel: m_channels) {
               channel->m_maxPoints = m_buffer->m_NBufferToDisplay * m_buffer->m_DimSizes[1];
           }
        }
    });
    m_parameters.push_back(m_paramTimeScale);

    m_paramVScale = new Parameters::ParameterDoubleObject("Vertical scale", "Vertical scale (only used if Scaling Mode is None)", 1.0, 0.01, 10000.0, 2);
    m_parameters.push_back(m_paramVScale);

    m_paramVOffset = new Parameters::ParameterDoubleObject("Vertical offset", "Vertical offset (only used if Scaling Mode is None))", 0.0, -1000.0, 1000.0, 2);
    m_parameters.push_back(m_paramVOffset);

    m_paramSelectedChannels = new Parameters::ParameterMultiSelectObject("Selected channels", "Selected channels", {});
    m_parameters.push_back(m_paramSelectedChannels);

    //m_parameters.push_back(new ParameterBoolObject("Show horizontal ruler", "Display or not the horizontal ruler", true));
    //m_parameters.push_back(new ParameterBoolObject("Show vertical ruler", "Display or not the vertical ruler", true));
    m_paramMultiView = new Parameters::ParameterBoolObject("Show multiple views", "Display or not multiple views", false);
    m_parameters.push_back(m_paramMultiView);
    QObject::connect(m_paramMultiView, &Parameters::ParameterBoolObject::valueChanged, [this](const bool &multiView) {
        this->setMultiView(multiView);
    });

    QQmlContext *context = new QQmlContext(m_jsEngine->rootContext());

    context->setContextProperty("dataSource", this); //*m_channels);
#if defined TARGET_OS_Windows
    context->setContextProperty("TARGET_OS_Windows", QVariant(true));
#else
    context->setContextProperty("TARGET_OS_Windows", QVariant(false));
#endif

    QQmlComponent component(m_jsEngine, QUrl("qrc:/qml/OVChart.qml"));

    QObject *object = component.create(context);
    if (!object) {
        m_logManager << Kernel::LogLevel_Error << "Failed to create QML object from OVChart.qml\n";
        return;
    }

    m_visualItem = qobject_cast<QQuickItem *>(object);
    if (m_visualItem) {
        context->setParent(m_visualItem);
    } else {
        m_logManager << Kernel::LogLevel_Error << "Failed to cast QQuickItem from OVChart.qml\n";
        delete context;
    }
}

ChartViewObject::~ChartViewObject()
{
    // Clean up the visual item before anything else
    if (m_visualItem) {
        m_visualItem->deleteLater();
        m_visualItem = nullptr;
    }

    for (PlotData *channel : m_channels) {
        delete channel;
    }
    m_channels.clear();

    for (StimulationData *stimulation : m_stimulationDatas) {
        delete stimulation;
    }
    m_stimulationDatas.clear();

    for (Parameters::ParameterObject *param : m_parameters) {
        delete param;
    }
    m_parameters.clear();
    m_paramScalingMode = nullptr;
    m_paramVScale = nullptr;
    m_paramVOffset = nullptr;

    m_jsEngine = nullptr;
}

QQmlListProperty<PlotData> ChartViewObject::channels()
{
    return QQmlListProperty<PlotData>(this, &m_channels);
}

QQmlListProperty<StimulationData> ChartViewObject::stimulations()
{
    return QQmlListProperty<StimulationData>(this, &m_stimulationDatas);
}

QString ChartViewObject::name() const
{
    return m_name;
}

QString ChartViewObject::description() const
{
    return m_description;
}

QString ChartViewObject::identifier() const
{
    return m_identifier;
}

QJSValue ChartViewObject::parameters() const
{
    int nbParams = m_parameters.size();
    QJSValue jsParams = m_jsEngine->newArray(nbParams);
    for (int i = 0; i < nbParams; ++i) {
        jsParams.setProperty(i, m_jsEngine->newQObject(m_parameters[i]));
    }
    
    return jsParams;
}

void ChartViewObject::setName(const QString &name)
{
    if (m_name != name) {
        m_name = name;
        emit nameChanged(m_name);
    }
}

void ChartViewObject::setDescription(const QString &description)
{
    if (m_description != description) {
        m_description = description;
        emit descriptionChanged(m_description);
    }
}

bool ChartViewObject::updateSerie(int i, QAbstractSeries *serie, QAbstractAxis *xAxis, QAbstractAxis *yAxis)
{
    if(!serie) {
        m_logManager << Kernel::LogLevel_Warning << "serie " << i << " is null!  \n";
        return false;
    }

    if(!m_paramSelectedChannels->value().contains(serie->name())) {
        return false;
    }

    QXYSeries *xySeries = static_cast<QXYSeries *>(serie);
    if (xySeries) {
        xySeries->replace(m_channels[i]->points());
    }

    qreal min_x = 1e9, max_x = -1e9;
    if (xAxis) {
        auto *xAxisCasted = static_cast<QValueAxis *>(xAxis);
        min_x = xAxisCasted->min();
        max_x = xAxisCasted->max();
        
        qreal start = m_channels[i]->points().first().x();
        if(min_x < start) {
            min_x = start;
        }

        qreal end = m_channels[i]->points().last().x();
        if(end > max_x) {
            max_x = end;
        }
        xAxis->setRange(min_x, max_x);
    }

    qreal min_y = 1e9, max_y = -1e9;
    if (yAxis) {
        auto *yAxisCasted = static_cast<QValueAxis *>(yAxis);

        min_y = yAxisCasted->min();
        max_y = yAxisCasted->max();

        qreal new_min = 1e9, new_max = -1e9;
        for (const QPointF &point : m_channels[i]->points()) {
            new_min = qMin(new_min, point.y());
            new_max = qMax(new_max, point.y());
        }
        if(new_min < min_y) {
            min_y = new_min;
        }
        if(new_max > max_y) {
            max_y = new_max;
        }

        yAxis->setRange(min_y, max_y);
    }

    return true;
}

void ChartViewObject::Init()
{
    std::vector<std::string> channelName = m_buffer->m_DimLabels[0];

    //create channels
    QStringList availableChannels;
    const size_t nChannel  = m_buffer->GetChannelCount();
    for (size_t i = 0; i < nChannel; ++i) {
        if (channelName[i].empty()) { 
            channelName[i] = "Channel " + std::to_string(i);
        }
        PlotData *data = new PlotData(i, QString::fromStdString(channelName[i]));
        data->moveToThread(m_jsEngine->thread());
        //data->m_points.reserve(m_buffer->m_DimSizes[1]);
        m_channels.append(data);
        availableChannels.append(QString::fromStdString(channelName[i]));
    }

    m_paramSelectedChannels->setAvailableValues(availableChannels);
    m_paramSelectedChannels->setValue(availableChannels);
    emit channelsChanged();
}

void ChartViewObject::Redraw()
{
    //nothing to redraw if no data was received
    if (!m_buffer->HasFirstBuffer()) {
        return;
    }

    double *newBuffer = m_buffer->m_SampleBuffers.back();
    uint64_t startTimes = m_buffer->m_StartTime.back(); 
    uint64_t endTime = m_buffer->m_EndTime.back();

    //for each channel
    uint64_t currentSample = 0;
    uint64_t time = startTimes;
    uint64_t timeStep = (endTime - startTimes) / m_buffer->m_DimSizes[1];

    double offset = 0, coeff = 1;
    if(m_paramScalingMode->value() == "None") {
        offset = m_paramVOffset->value();
        coeff = m_paramVScale->value();
    }

    for(auto *channel: m_channels) {
        for (uint64_t i = 0; i < m_buffer->m_DimSizes[1]; ++i) {
            //add the point to the channel
            double time_ms = CONVERT_TIME(time);
            channel->addPoint(QPointF(time_ms, coeff*newBuffer[currentSample] + offset));
            time += timeStep;
            currentSample++;
        }
    }

    for (const auto &stim : m_buffer->m_Stimulations) {
        if(stim.first >= startTimes && stim.first <= endTime) {
            size_t j = 0;
            while (j < m_buffer->m_NBufferToDisplay && stim.first > m_buffer->m_EndTime[j]) {
                j++;
            }

            if (j < m_buffer->m_NBufferToDisplay) {
                double time_ms = CONVERT_TIME(startTimes + j*timeStep);
                StimulationData *stimulation = new StimulationData(time_ms,
                                            m_stimulations[stim.first].first.toASCIIString(),
                                            m_stimulations[stim.first].second);
                stimulation->moveToThread(m_jsEngine->thread());
                m_stimulationDatas.append(stimulation);
            }
        }
    }
    emit launchQmlRedraw();
}

void ChartViewObject::setChannelUnits(const std::vector<std::pair<CString, CString>>& channelUnits)
{
    if(static_cast<long unsigned int>(m_channels.size()) != channelUnits.size()){
        m_logManager << Kernel::LogLevel_Warning << "Channel count mismatch!  channels :" << static_cast<size_t>(m_channels.size()) << " channelUnits : " << channelUnits.size() << "\n";
        return;
    }
    for(auto *channel : m_channels){
        channel->setName(channelUnits[channel->id()].first.toASCIIString());
        channel->setUnit(channelUnits[channel->id()].second.toASCIIString());
    }
}

void ChartViewObject::setStimulationColor(const uint64_t id, const CString& name)
{
    if (m_stimulations.find(id) == m_stimulations.end()) {
        //only the lower 32 bits of the stimulation code are currently used to compute the color
        const auto code = uint32_t(id);

        //go through the lower 32 bits to compute RGB components. Bit positions are
        //inverted so that close code values result in very different colors.
        uint32_t red = 0;
        uint32_t green = 0;
        uint32_t blue = 0;

        for (uint32_t i = 0; i < 11; ++i) {
            red |= ((code >> (3 * i)) & 0x1) << (10 - i);
            green |= ((code >> (3 * i + 1)) & 0x1) << (10 - i);
            if (i < 10) { blue |= ((code >> (3 * i + 2)) & 0x1) << (9 - i); }   //only 10 bits for blue component
        }

        //convert to 16 bits per channel
        red   = (red * 65535) / 0x7FF; //red coded on 11 bits
        green = (green * 65535) / 0x7FF; //green coded on 11 bits
        blue  = (blue * 65535) / 0x3FF; //blue coded on 10 bits

        //convert 16-bit values to 8-bit values
        QColor color;
        color.setRed(red / 256);
        color.setGreen(green / 256);
        color.setBlue(blue / 256);

        //store stimulation color in map
        m_stimulations[id].first  = name;
        m_stimulations[id].second = color;

        QString stimName = name.toASCIIString();
        for(auto *channel : m_channels){
            if(channel->name() == stimName){
                channel->setColoring(color);
            }
        }
    }
}


void ChartViewObject::setInitialParameters(QString scalingMode, double verticalScale, double verticalOffset, double timeScale,
                                           bool horizontalRuler, bool verticalRuler, bool multiview)
{
    if(scalingMode == "Per channel") {
        m_logManager << Kernel::LogLevel_Warning << "Per channel scaling mode not implemented yet will set to None";
        scalingMode = "None";
    }

    m_paramScalingMode->setValue(scalingMode);
    m_paramTimeScale->setValue(timeScale);
    m_paramVScale->setValue(verticalScale);
    m_paramVOffset->setValue(verticalOffset);
    //display or not xAxis
    //horizontalRuler

    //display or not yAxis
    //verticalRuler

    //display or not multiple views
    //multiview
}

}
}
}