// SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

#include "CGraphWindow.h"
#include <QAreaSeries>
#include <QLineSeries>
#include <QValueAxis>
#include <QXYSeries>
#include <QQmlContext>
#include <QQuickItem>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QQuickWindow>

Q_DECLARE_METATYPE(QAbstractAxis *)
Q_DECLARE_METATYPE(QAbstractSeries *)


namespace OpenViBE {
namespace Plugins {
namespace SimpleVisualization {

void Curve::setSerieData(QAbstractSeries* serie, QAbstractAxis* xAxis, QAbstractAxis* yAxis)
{
    if(!serie || !xAxis || !yAxis) {
        qWarning() << "Invalid serie or axis " << serie << xAxis << yAxis;
        return;
    }

    auto *xAxisCasted = static_cast<QValueAxis *>(xAxis);
    qreal min_Xaxis = xAxisCasted->min();
    qreal max_Xaxis = xAxisCasted->max();
    qreal xStep = (max_Xaxis - min_Xaxis) / m_data.size();

    QXYSeries *xySeries = static_cast<QXYSeries *>(serie);
    if(xySeries) {
        xySeries->clear();
        for (long unsigned int i = 0; i < m_data.size(); ++i) {
            xySeries->append(min_Xaxis + i*xStep, m_data[i]);
        }
    }

    if (yAxis) {
        auto *yAxisCasted = static_cast<QValueAxis *>(yAxis);

        qreal min_axis = yAxisCasted->min();
        qreal max_axis = yAxisCasted->max();

        qreal new_min = 1e9, new_max = -1e9;
        //get min, max from data
        for (double y : m_data ) {
            new_min = qMin(new_min, y);
            new_max = qMax(new_max, y);
        }
        //update axis range if new data outside current range
        if(new_min < min_axis) {
            min_axis = new_min;
        }
        if(new_max > max_axis) {
            max_axis = new_max;
        }

        yAxis->setRange(min_axis, max_axis);
    }
}

void Curve::setSerieVariance(QAbstractSeries* serie, QAbstractAxis* xAxis, QAbstractAxis* yAxis)
{
    if(!serie || !xAxis || !yAxis) {
        qWarning() << "Invalid serie or axis " << serie << xAxis << yAxis;
        return;
    }

    auto *xAxisCasted = static_cast<QValueAxis *>(xAxis);
    qreal min_Xaxis = xAxisCasted->min();
    qreal max_Xaxis = xAxisCasted->max();
    qreal xStep = (max_Xaxis - min_Xaxis) / m_data.size();

    auto *yAxisCasted = static_cast<QValueAxis *>(yAxis);

    qreal min_axis = yAxisCasted->min();
    qreal max_axis = yAxisCasted->max();

    qreal new_min = 1e9, new_max = -1e9;

    QAreaSeries *areaSeries = static_cast<QAreaSeries *>(serie);
    if(areaSeries) {
        auto *lowerSerie = areaSeries->lowerSeries();
        lowerSerie->clear();
        auto *upperSerie = areaSeries->upperSeries();
        upperSerie->clear();
        for (long unsigned int i = 0; i < m_data.size(); ++i) {
            lowerSerie->append(min_Xaxis + i*xStep, m_data[i] - m_variance[i]);
            upperSerie->append(min_Xaxis + i*xStep, m_data[i] + m_variance[i]);

            new_min = qMin(new_min, m_data[i] - m_variance[i]);
            new_max = qMax(new_max, m_data[i] + m_variance[i]);
    
            if(new_min < min_axis) {
                min_axis = new_min;
            }
            if(new_max > max_axis) {
                max_axis = new_max;
            }
        }
        yAxis->setRange(min_axis, max_axis);
    }
}

void Graph::initCurves(const std::vector<std::string>& lineColor, const std::vector<std::string>& lineText)
{
    Q_ASSERT(lineColor.size() == lineText.size());

    //clear previous curves
    for(auto& curve : m_curves) {
        curve->deleteLater();
    }
    m_curves.clear();

    for (size_t i = 0; i < lineColor.size(); ++i) {
        QString color = lineColor[i].c_str();
        QStringList rgb_list = color.split(",");
        int r = std::min(rgb_list[0].toInt(), 255);
        int g = std::min(rgb_list[1].toInt(), 255);
        int b = std::min(rgb_list[2].toInt(), 255);
        m_curves.append(new Curve(QColor(r,g,b), lineText[i].c_str()));

        //connect curve signals
        connect(m_curves[i], &Curve::dataChanged, [this, i]() { this->launchQmlRedraw(i, true); });
        connect(m_curves[i], &Curve::varianceChanged, [this, i]() { this->launchQmlRedraw(i, false); });
    }
}

void Graph::updateTimes(uint64_t startTime, uint64_t endTime)
{
    if(endTime - startTime < 1) {
        qWarning() << "Invalid time range " << startTime << endTime;
        endTime = startTime + 1;
    }
    m_EndTime = CTime(endTime).toSeconds();
    m_StartTime = CTime(startTime).toSeconds();

    emit timesChanged();
}

void Graph::updateCurves(const double* curve, size_t howMany, size_t curveIndex)
{
    m_curves[curveIndex]->setData(curve, howMany);
}

void Graph::updateVariance(const double* variance, size_t howMany, size_t varianceIndex)
{
    m_curves[varianceIndex]->setVariance(variance, howMany);
}


CGraphWindow::CGraphWindow(Kernel::ILogManager& logManager, QQmlEngine* engine) : m_logManager(logManager), m_jsEngine(engine)
{
    qRegisterMetaType<QAbstractAxis*>();
    qRegisterMetaType<QAbstractSeries*>();
    qRegisterMetaType<Curve *>();

    bool openGLSupported = QQuickWindow::graphicsApi() == QSGRendererInterface::OpenGLRhi;
    if (!openGLSupported) {
        m_logManager << Kernel::LogLevel_Warning << "OpenGL is not set as the graphics backend, so AbstractSeries.useOpenGL will not work.";
        m_logManager << Kernel::LogLevel_Warning << "Set QSG_RHI_BACKEND=opengl environment variable to force the OpenGL backend to be used.";
    }

    QQmlContext *context = new QQmlContext(m_jsEngine->rootContext());
    
    context->setContextProperty("dataSource", this); //*m_channels);
    context->setContextProperty("openGLSupported", openGLSupported);
#if defined TARGET_OS_Windows
    context->setContextProperty("TARGET_OS_Windows", QVariant(true));
#else
    context->setContextProperty("TARGET_OS_Windows", QVariant(false));
#endif

    QQmlComponent component(m_jsEngine, QUrl("qrc:/qml/CGraphWindow.qml"));

    QObject *object = component.create(context);
    if (!object) {
        m_logManager << Kernel::LogLevel_Error << "Failed to create QML object from CGraphWindow.qml\n";
        return;
    }

    m_visualItem = qobject_cast<QQuickItem *>(object);
    if (m_visualItem) {
        context->setParent(m_visualItem);
    } else {
        m_logManager << Kernel::LogLevel_Error << "Failed to cast QQuickItem from CGraphWindow.qml\n";
        delete context;
    }
}


CGraphWindow::~CGraphWindow()
{
    if (m_visualItem) {
        m_visualItem->deleteLater();
        m_visualItem = nullptr;
    }
    m_jsEngine = nullptr;
    m_graphs.clear();
}


uint CGraphWindow::rowCount() const
{
    uint rowCount = 0;
    for (const auto& [key, value] : m_graphs) {
        if (std::get<0>(key) > rowCount) {
            rowCount = std::get<0>(key);
        }
    }
    return rowCount + 1;
}

uint CGraphWindow::colCount() const
{
    uint colCount = 0;
    for (const auto& [key, value] : m_graphs) {
        if (std::get<1>(key) > colCount) {
            colCount = std::get<1>(key);
        }
    }
    return colCount + 1;
}

std::shared_ptr<Graph> CGraphWindow::createGraph(uint rowIndex, uint colIndex)
{
    //check if graph already exists at this position
    if(m_graphs.find(std::make_tuple(rowIndex, colIndex)) != m_graphs.end()) {
        m_logManager << Kernel::LogLevel_Warning << "Graph already exists at position (" << rowIndex << ", " << colIndex << ")\n";
        return nullptr;
    }

    //if no position specified, add at the end
    if(rowIndex == 999) {
        rowIndex = rowCount();
    }
    if(colIndex == 999) {
        colIndex = colCount();
    }

    std::shared_ptr<Graph> graph = std::make_shared<Graph>();
    graph->moveToThread(m_jsEngine->thread());
    m_graphs[std::make_tuple(rowIndex, colIndex)] = graph;
    m_graphsList.push_back(graph.get());
    emit graphsChanged();
    return graph;
}

std::shared_ptr<Graph> CGraphWindow::getGraph(uint rowIndex, uint colIndex)
{
    if(m_graphs.find(std::make_tuple(rowIndex, colIndex)) == m_graphs.end()) {
        m_logManager << Kernel::LogLevel_Warning << "No graph found at position (" << rowIndex << ", " << colIndex << ")\n";
        return nullptr;
    }
    return m_graphs[std::make_tuple(rowIndex, colIndex)];
}

void CGraphWindow::removeGraph(std::shared_ptr<Graph> graph)
{
    for (auto it = m_graphs.begin(); it != m_graphs.end(); ++it) {
        if(it->second == graph) {
            m_graphsList.removeOne(graph.get());
            m_graphs.erase(it);
                qDebug() << Q_FUNC_INFO;
            emit graphsChanged();
            return;
        }
    }
}

void CGraphWindow::removeGraph(uint rowIndex, uint colIndex)
{
    auto it = m_graphs.find(std::make_tuple(rowIndex, colIndex));
    if(it == m_graphs.end()) {
        m_logManager << Kernel::LogLevel_Warning << "No graph found at position (" << rowIndex << ", " << colIndex << ") cannot remove it\n";
        return;
    }
    m_graphsList.removeOne(it->second.get());
    m_graphs.erase(std::make_tuple(rowIndex, colIndex));
    emit graphsChanged();
}


void CGraphWindow::clearGraphs()
{
    m_graphsList.clear();
    m_graphs.clear();
    emit graphsChanged();
}

}
}
}