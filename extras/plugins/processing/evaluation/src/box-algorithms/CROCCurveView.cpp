// SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

#include "CROCCurveView.hpp"

#include <QXYSeries>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QQuickItem>
#include <QQuickWindow>
#include <QValueAxis>

Q_DECLARE_METATYPE(QAbstractAxis *)
Q_DECLARE_METATYPE(QAbstractSeries *)

namespace OpenViBE {
namespace Plugins {
namespace Evaluation {

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

class Curve : public QObject {
    Q_OBJECT
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(QList<QPointF> points READ points NOTIFY pointsChanged)
    Q_PROPERTY(QColor coloring READ coloring NOTIFY coloringChanged) //color is a qml data type


signals:
    void pointsChanged();
    void nameChanged();
    void coloringChanged();

public:
    Curve(const QString& name, const QColor& color) : m_name(name), m_color(color) {
        m_points.reserve(CURVE_MAX_POINTS);
    }
    ~Curve() {}

    QString name() const { return m_name; }
    QList<QPointF> points() const { return m_points; }
    QColor coloring() const { return m_color; }
    
    void addPoint(const QPointF& point) {
        m_points.append(point);
        if(m_points.size() > CURVE_MAX_POINTS) {
            m_points.removeFirst();
        }
        emit pointsChanged();
    }

    void setName(const QString& name) {
        if(m_name != name ) {
             m_name = name;
            emit nameChanged();
        }
    }

    void setColor(const QColor& color) {
        m_color = color;
        emit coloringChanged();
    }

protected:
    QString m_name;
    QList<QPointF> m_points;
    QColor m_color;
    static inline const int CURVE_MAX_POINTS = 100;
};

CROCCurveView::CROCCurveView(Kernel::ILogManager &logMgr, QQmlEngine* engine) : m_logManager(logMgr), m_jsEngine(engine)
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

    context->setContextProperty("dataSourceROC", this);
    context->setContextProperty("dataSourceAccuracy", nullptr);

    context->setContextProperty("openGLSupported", openGLSupported);
#if defined TARGET_OS_Windows
    context->setContextProperty("TARGET_OS_Windows", QVariant(true));
#else
    context->setContextProperty("TARGET_OS_Windows", QVariant(false));
#endif

    QQmlComponent component(m_jsEngine, QUrl("qrc:/qml/ROCCurve.qml"));

    QObject *object = component.create(context);
    if (!object) {
        m_logManager << Kernel::LogLevel_Error << "Failed to create QML object from ROCCurve.qml\n";
        return;
    }
    m_visualItem = qobject_cast<QQuickItem *>(object);
    if (m_visualItem) {
        context->setParent(m_visualItem);
    } else {
        m_logManager << Kernel::LogLevel_Error << "Failed to cast QQuickItem from ROCCurve.qml\n";
        delete context;
    }
}

CROCCurveView::~CROCCurveView()
{
    qDeleteAll(m_curves);
    m_curves.clear();

    for (Parameters::ParameterObject *param : m_parameters) {
        delete param;
    }
    m_parameters.clear();

    if (m_visualItem) {
        m_visualItem->deleteLater();
        m_visualItem = nullptr;
    }
    m_jsEngine = nullptr;
    

}

std::vector<std::string> CROCCurveView::curveNames() const
{
    std::vector<std::string> names;
    for(auto *curve : m_curves) {
        names.push_back(curve->name().toStdString());
    }

    return names;
}

void CROCCurveView::addCurve(const std::string& name)
{
    addCurve(QString::fromStdString(name));
}

void CROCCurveView::addCurve(const QString& name)
{
    int idx = m_curves.size();
    Curve *curve = new Curve(name, baseColors[idx % 15]);
    m_curves.append(curve);
    emit curvesChanged();
}

bool CROCCurveView::removeCurve(const std::string& name)
{
    return removeCurve(QString::fromStdString(name));
}

bool CROCCurveView::removeCurve(const QString& name)
{
    for(auto *curve : m_curves) {
        if(curve->name() == name) {
            m_curves.removeOne(curve);
            delete curve;
            emit curvesChanged();
            return true;
        }
    }
    m_logManager << Kernel::LogLevel_Error << "Curve " << name.toStdString() << " not found\n";
    return false;
}

bool CROCCurveView::addPoint(const std::string& name, double x, double y)
{
    QString qname = QString::fromStdString(name);
    for(auto *curve : m_curves) {
        if(curve->name() == qname) {
            curve->addPoint(QPointF(x, y));
            //can trigger a redraw of the curve here if we want
            //emit launchQmlRedraw();
            return true;
        }
    }
    m_logManager << Kernel::LogLevel_Error << "Curve " << name << " not found\n";
    return false;
}

QString CROCCurveView::name() const
{
    return m_name;
}

void CROCCurveView::setName(const QString &name)
{
    if(m_name != name ) {
         m_name = name;
        emit nameChanged(m_name);
    }
}

QString CROCCurveView::description() const
{
    return m_description;
}

void CROCCurveView::setDescription(const QString &description)
{
    if(m_description != description) {
        m_description = description;
        emit descriptionChanged(m_description);
    }
}

QQmlListProperty<Curve> CROCCurveView::curves()
{
    return QQmlListProperty<Curve>(this, &m_curves);
}

QJSValue CROCCurveView::parameters() const
{
    int nbParams = m_parameters.size();
    QJSValue jsParams = m_jsEngine->newArray(nbParams);
    for (int i = 0; i < nbParams; ++i) {
        jsParams.setProperty(i, m_jsEngine->newQObject(m_parameters[i]));
    }
    
    return jsParams;
}

bool CROCCurveView::updateSerie(int i, QAbstractSeries *serie, QAbstractAxis *xAxis, QAbstractAxis *yAxis)
{
    if(!serie) {
        m_logManager << Kernel::LogLevel_Warning << "serie " << i << " is null!  \n";
        return false;
    }

    //if(!m_paramSelectedChannels->value().contains(serie->name())) {
    //    return false;
    //}

    QXYSeries *xySeries = static_cast<QXYSeries *>(serie);
    if (xySeries) {
        xySeries->replace(m_curves[i]->points());
    }

    QList<QPointF> points = m_curves[i]->points();
    double x_min=1000, x_max=-1000, y_min=10000, y_max=-1000;
    for(auto point : points) {
        x_min = qMin(x_min, point.x());
        x_max = qMax(x_max, point.x());
        y_min = qMin(y_min, point.y());
        y_max = qMax(y_max, point.y());
    }
    
    if (xAxis) {
        auto *xAxisCasted = static_cast<QValueAxis *>(xAxis);
        qreal min_axis = xAxisCasted->min();
        qreal max_axis = xAxisCasted->max();

        if(min_axis > x_min) {
            min_axis = x_min;
        }

        if(max_axis < x_max) {
            max_axis = x_max;
        }
        xAxis->setRange(min_axis, max_axis);
    }

    if (yAxis) {
        auto *yAxisCasted = static_cast<QValueAxis *>(yAxis);

        qreal min_axis = yAxisCasted->min();
        qreal max_axis = yAxisCasted->max();

        if(min_axis > y_min) {
            min_axis = y_min;
        }
        if(max_axis< y_max) {
            max_axis = y_max;
        }

        yAxis->setRange(min_axis, max_axis);
    }
    return true;
}

#include "CROCCurveView.moc"

}
}
}