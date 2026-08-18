// SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include <QtCore>
#include <QColor>
#include <QAbstractSeries>
#include <QAbstractAxis>
#include <QQmlListProperty>
#include <QJSValue>


#include <openvibe/ov_all.h>
#include <parametersObject.h>

class QQmlEngine;
class QQuickItem;


namespace OpenViBE {
namespace Plugins {
namespace Evaluation {

class Curve;

class CROCCurveView : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString name READ name WRITE setName NOTIFY nameChanged)
    Q_PROPERTY(QString description READ description WRITE setDescription NOTIFY descriptionChanged)
    Q_PROPERTY(QQmlListProperty<Curve> curves READ curves NOTIFY curvesChanged)
    Q_PROPERTY(QJSValue parameters READ parameters NOTIFY parametersChanged)


signals:
    void curvesChanged();
    void descriptionChanged(const QString&);
    void launchQmlRedraw();
    void nameChanged(const QString&);
    void parametersChanged();

public:
    CROCCurveView(Kernel::ILogManager &logMgr, QQmlEngine* engine);
    virtual ~CROCCurveView();

    std::vector<std::string> curveNames() const;
    void addCurve(const std::string& name);
    void addCurve(const QString& name);
    bool removeCurve(const std::string& name);
    bool removeCurve(const QString& name);

    bool addPoint(const std::string& name, double x, double y);

    QString name() const;
    void setName(const QString &name);

    QString description() const;
    void setDescription(const QString &description);
    //will return true if the serie was updated, false otherwise. in case of false, you can hide this serie!
    Q_INVOKABLE bool updateSerie(int i, QAbstractSeries *serie, QAbstractAxis *xAxis, QAbstractAxis *yAxis);

    QQmlListProperty<Curve> curves();
    QJSValue parameters() const;

    QQuickItem* visualItem() { return m_visualItem; }

protected:
    QList<Curve *> m_curves;
    QString m_name = "ROC curve";
    QString m_description = "ROC curve description";

    QQmlEngine *m_jsEngine = nullptr;
    QQuickItem *m_visualItem = nullptr;
    Kernel::ILogManager &m_logManager;
    Parameters::ParameterObjects m_parameters;

};

}
}
}