// SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include <QtCore>
#include <string>
#include <vector>
#include <parametersObject.h>

#include <QQmlEngine>
#include <QQuickItem>

#include "CGrazMultiVisualization.h"

namespace OpenViBE {
namespace Plugins {
namespace SimpleVisualization {


class CGrazMultiVisuWindow : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString barImg READ barImg NOTIFY barImgChanged)
    Q_PROPERTY(QList<double> barScales READ barScales NOTIFY barScalesChanged)
    Q_PROPERTY(bool delayFeedback READ delayFeedback NOTIFY delayFeedbackChanged)
    Q_PROPERTY(int feedbackMode READ feedbackMode NOTIFY feedbackModeChanged)
    Q_PROPERTY(QStringList imgsPaths READ imgsPaths NOTIFY imgsPathsChanged)
    Q_PROPERTY(int modality READ modality NOTIFY modalityChanged)
    Q_PROPERTY(int nbModality READ nbModality NOTIFY nbModalityChanged)
    Q_PROPERTY(bool showAccuracy READ showAccuracy NOTIFY showAccuracyChanged)
    Q_PROPERTY(bool showInstruction READ showInstruction NOTIFY showInstructionChanged)
    Q_PROPERTY(QString state READ state NOTIFY stateChanged)
    Q_PROPERTY(int vote READ vote NOTIFY voteChanged)

public:
    CGrazMultiVisuWindow(CGrazMultiVisualization* parent, QQmlEngine* engine);
    ~CGrazMultiVisuWindow();

    //Q_INVOKABLE void flushQueue(); // to be called after redraw
    Q_INVOKABLE int getAccuracy(uint i, uint j);

    QString barImg();
    QList<double> barScales();
    bool delayFeedback();
    int feedbackMode();
    int modality();
    int nbModality();
    bool showAccuracy();
    bool showInstruction();
    QString state();
    int vote();
    QStringList imgsPaths();
    
    void setImgsPaths(const std::vector<std::string>& paths);
    QQuickItem* visualItem() { return m_visualItem; }
    
signals:
    void barImgChanged();
    void barScalesChanged();
    void delayFeedbackChanged();
    void feedbackModeChanged();
    void launchQmlRedraw();
    void imgsPathsChanged();
    void modalityChanged();
    void nbModalityChanged();
    void showAccuracyChanged();
    void showInstructionChanged();
    void stateChanged();
    void voteChanged();

protected:
    CGrazMultiVisualization* m_parent;
    QQmlEngine *m_jsEngine;
    QQuickItem* m_visualItem = nullptr;

    QString m_name;
    QString m_description;
    QStringList m_imgsPaths;
    QList<double> m_barScales; // from 0 to 1 for each bar (depends of the feedback if only positive feedback or not)
};

}
}
}