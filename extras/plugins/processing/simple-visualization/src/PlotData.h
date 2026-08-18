// SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once
#include <QtCore>
#include <QColor>


namespace OpenViBE {
namespace Plugins {
namespace SimpleVisualization {

static const QColor baseColors[15] = {
    QColor(255, 0, 0),    // Red
    QColor(0, 0, 255),    // Blue
    QColor(255, 255, 0),  // Yellow
    QColor(0, 255, 255),  // Cyan
    QColor(255, 0, 255),  // Magenta
    QColor(255, 165, 0),  // Orange
    QColor(0, 255, 0),    // Green
    QColor(128, 0, 128),  // Purple
    QColor(255, 192, 203),// Pink
    QColor(165, 42, 42),  // Brown
    QColor(0, 0, 0),      // Black
    QColor(255, 255, 255),// White
    QColor(128, 128, 128),// Gray
    QColor(0, 128, 0),    // Lime
    QColor(0, 0, 128)     // Navy
};

class StimulationData : public QObject
{
    Q_OBJECT
    Q_PROPERTY(uint64_t time READ time CONSTANT)
    Q_PROPERTY(QString name READ name CONSTANT)
    Q_PROPERTY(QColor coloring READ coloring CONSTANT)

public:
    StimulationData(uint64_t time, QString name, QColor color) :
        m_time(time), m_name(name), m_color(color) {}
    ~StimulationData() = default;

    uint64_t time() const { return m_time; }
    QString name() const { return m_name; }
    QColor coloring() const { return m_color; }

protected:
    uint64_t m_time;
    QString m_name;
    QColor m_color;
};

class PlotData : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QList<QPointF> points READ points NOTIFY pointsChanged)
    Q_PROPERTY(QString name READ name NOTIFY nameChanged)
    Q_PROPERTY(QString unit READ unit NOTIFY unitChanged)
    Q_PROPERTY(QColor coloring READ coloring NOTIFY coloringChanged) //color is a qml data type

signals:
    void coloringChanged();
    void nameChanged();
    void pointsChanged();
    void unitChanged();

public:
    PlotData(uint64_t id, const QString& name, int maxPoints = PLOT_MAX_POINTS_DEFAULT) : m_maxPoints(maxPoints), m_id(id), m_name(name) {
        m_points.reserve(m_maxPoints);
        m_color = baseColors[counter % 15];
        counter++;
    }
    ~PlotData() {}

    uint64_t id() const { return m_id; }
    QList<QPointF> points() const { return m_points; }
    QString name() const { return m_name; }
    QString unit() const { return m_unit; }
    QColor coloring() const { return m_color; }

    void addPoint(const QPointF& point) { 
        m_points.append(point);
        if(m_points.size() > m_maxPoints) {
            m_points.removeFirst();
        }
        emit pointsChanged();
    }

    void setName(const QString& name) { m_name = name; emit nameChanged(); }
    void setUnit(const QString& unit) { m_unit = unit; emit unitChanged(); }
    void setColoring(const QColor& color) { m_color = color; emit coloringChanged(); }
    int m_maxPoints = PLOT_MAX_POINTS_DEFAULT;

protected:
    uint64_t m_id;
    QString m_name = "";
    QString m_unit = "Unknown";
    QList<QPointF> m_points;
    QColor m_color;
    static inline int counter = 0;
    static inline const int PLOT_MAX_POINTS_DEFAULT = 1000;

};

} // namespace SimpleQmlVisualization
} // namespace Plugins
} // namespace OpenViBE