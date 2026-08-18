// SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include <QtCore>
#include <QQmlListProperty>

#include <openvibe/ov_all.h>

class QQmlEngine;
class QQuickItem;

namespace OpenViBE {
namespace Plugins {
namespace Evaluation {

class ProgressBarData;

class ClassifierAccuracyMeasureView : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QQmlListProperty<ProgressBarData> progressBars READ progressBars NOTIFY progressBarsChanged)

signals:
    void progressBarsChanged();

public:
    ClassifierAccuracyMeasureView(Kernel::ILogManager &logMgr, QQmlEngine* engine);
    virtual ~ClassifierAccuracyMeasureView();

    void addProgressBar(const QString& label);
    void removeProgressBar(const QString& label);
    uint nbProgressBars() const;

    Q_INVOKABLE void resetScores();

    void incrementScore(int idx);
    void incrementNStimulation(int idx);

    QQmlListProperty<ProgressBarData> progressBars(); 

    QQuickItem* visualItem() { return m_visualItem; }

protected:
    QList<ProgressBarData*> m_progressBars;

    QQmlEngine* m_jsEngine;
    QQuickItem* m_visualItem = nullptr;
    Kernel::ILogManager &m_logManager;
};

}
}
}