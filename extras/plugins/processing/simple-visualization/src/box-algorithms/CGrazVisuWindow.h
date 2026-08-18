// SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include <QtCore>
#include <string>
#include <vector>
#include <parametersObject.h>

#include <QQmlEngine>
#include <QQuickItem>

#include "CGrazVisualization.h"

namespace OpenViBE {
namespace Plugins {
namespace SimpleVisualization {


class CGrazVisuWindow : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString barImg READ barImg NOTIFY barImgChanged)
    Q_PROPERTY(double barScale READ barScale NOTIFY barScaleChanged)
    Q_PROPERTY(bool delayFeedback READ delayFeedback NOTIFY delayFeedbackChanged)
    Q_PROPERTY(bool showFeedback READ showFeedback NOTIFY showFeedbackChanged)
    Q_PROPERTY(bool positiveFeedbackOnly READ positiveFeedbackOnly NOTIFY positiveFeedbackOnlyChanged)
    Q_PROPERTY(QStringList imgsPaths READ imgsPaths NOTIFY imgsPathsChanged)
    Q_PROPERTY(bool showAccuracy READ showAccuracy NOTIFY showAccuracyChanged)
    Q_PROPERTY(bool showInstruction READ showInstruction NOTIFY showInstructionChanged)
    Q_PROPERTY(QString state READ state NOTIFY stateChanged)
    Q_PROPERTY(QString direction READ direction NOTIFY directionChanged)

public:
    CGrazVisuWindow(CGrazVisualization* parent, QQmlEngine* engine);
    ~CGrazVisuWindow();

    Q_INVOKABLE int getAccuracy(uint i, uint j);

    QString barImg();
    double barScale();
    bool delayFeedback();
    bool showFeedback();
    bool positiveFeedbackOnly();
    bool showAccuracy();
    bool showInstruction();
    QString state();
    QString direction();
    QStringList imgsPaths();
    
    void setImgsPaths(const std::vector<std::string>& paths);
    QQuickItem* visualItem() { return m_visualItem; }

signals:
    void barImgChanged();
    void barScaleChanged();
    void delayFeedbackChanged();
    void directionChanged();
    void showFeedbackChanged();
    void launchQmlRedraw();
    void positiveFeedbackOnlyChanged();
    void imgsPathsChanged();
    void showAccuracyChanged();
    void showInstructionChanged();
    void stateChanged();

protected:
    CGrazVisualization* m_parent;
    QQmlEngine *m_jsEngine;
    QQuickItem* m_visualItem = nullptr;

    QString m_name;
    QString m_description;
    QStringList m_imgsPaths;
};

}
}
}