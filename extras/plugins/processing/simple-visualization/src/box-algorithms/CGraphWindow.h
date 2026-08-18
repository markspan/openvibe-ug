// SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include <QtCore>
#include <QtQml>
#include <QAbstractSeries>
#include <QAbstractAxis>

#include <memory>

#include <openvibe/ov_all.h>

class QQuickItem;
class QQmlEngine;

namespace OpenViBE {
namespace Plugins {
namespace SimpleVisualization {

class Curve : public QObject{
    Q_OBJECT
    Q_PROPERTY(QColor col READ col WRITE setCol NOTIFY colChanged)
    Q_PROPERTY(QString label READ label WRITE setlabel NOTIFY labelChanged)

signals:
    void colChanged();
    void labelChanged();
    void dataChanged();
    void varianceChanged();

public:
    Curve(QObject *parent=nullptr) : QObject(parent) {};
    Curve(const QColor& color, const QString& text) : m_col(color), m_label(text) {}
    ~Curve() = default;

    QColor col() const { return m_col; }
    void setCol(const QColor& color) { m_col = color; }

    QString label() const { return m_label; }
    void setlabel(const QString& text) { m_label = text; }

    void setData(const double* curve, size_t howMany) {
        m_data = std::vector<double>(curve, curve + howMany);
        emit dataChanged();
    }

    void setVariance(const double* variance, size_t howMany) {
        m_variance = std::vector<double>(variance, variance + howMany);
        emit varianceChanged();
    }

    Q_INVOKABLE void setSerieData(QAbstractSeries* serie, QAbstractAxis* xAxis, QAbstractAxis* yAxis);
    Q_INVOKABLE void setSerieVariance(QAbstractSeries* serie, QAbstractAxis* xAxis, QAbstractAxis* yAxis);

protected:
    QColor m_col;
    QString m_label;
    std::vector<double> m_data;
    std::vector<double> m_variance;
};

class Graph : public QObject {
    Q_OBJECT
    Q_PROPERTY(QQmlListProperty<Curve> curves READ curves NOTIFY curvesChanged)
    Q_PROPERTY(double startTime MEMBER m_StartTime NOTIFY timesChanged)
    Q_PROPERTY(double endTime MEMBER m_EndTime NOTIFY timesChanged)

signals:
    void curvesChanged();
    void timesChanged();
    void launchQmlRedraw(int curveId, bool isData); //redraw a specific plot

public:
    Graph(QObject *parent=nullptr) : QObject(parent) {};
    ~Graph() = default;

    QQmlListProperty<Curve> curves() { return QQmlListProperty<Curve>(this, &m_curves); }

    void initCurves(const std::vector<std::string>& lineColor, const std::vector<std::string>& lineText);
    void updateCurves(const double* curve, size_t howMany, size_t curveIndex);
    void updateVariance(const double* variance, size_t howMany, size_t varianceIndex);
    void updateTimes(uint64_t startTime, uint64_t endTime); 

protected:
    //graph data
    double m_StartTime = 0;
    double m_EndTime = 1;
    QList<Curve *> m_curves;
};

class CGraphWindow : public QObject
{
    Q_OBJECT
    Q_PROPERTY(uint rowCount READ rowCount NOTIFY rowCountChanged)
    Q_PROPERTY(uint colCount READ colCount NOTIFY colCountChanged)
    Q_PROPERTY(QQmlListProperty<Graph> graphs READ graphs NOTIFY graphsChanged)

public:
    CGraphWindow(Kernel::ILogManager& logManager, QQmlEngine* engine);
    virtual ~CGraphWindow();

    uint rowCount() const;
    uint colCount() const;

    std::shared_ptr<Graph> createGraph(uint rowIndex = 999, uint colIndex= 999);
    std::shared_ptr<Graph> getGraph(uint rowIndex, uint colIndex);

    QQmlListProperty<Graph> graphs() { return QQmlListProperty<Graph>(this, &m_graphsList); }

    void removeGraph(std::shared_ptr<Graph> graph);
    void removeGraph(uint rowIndex, uint colIndex);
    void clearGraphs();

    QQuickItem* visualItem() { return m_visualItem; }

signals:
    void rowCountChanged();
    void colCountChanged();
    void graphsChanged();

protected:
    QQmlEngine *m_jsEngine = nullptr;
    QQuickItem *m_visualItem = nullptr;
    QList<Graph *> m_graphsList;
    std::map<std::tuple<uint, uint>, std::shared_ptr<Graph> > m_graphs; //graph with their position (row, column)
    Kernel::ILogManager &m_logManager;
};

}  // namespace SimpleVisualization
}  // namespace Plugins
}  // namespace OpenViBE