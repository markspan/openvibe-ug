// SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include <openvibe/ov_all.h>
#include <QtCore>
#include <QQmlEngine>
#include <QtQml>
#include <QJSValue>
#include <QQuickItem>

namespace OpenViBE {
namespace Plugins {
namespace SimpleVisualization {

class ProgressBar : public QObject {
    Q_OBJECT
    Q_PROPERTY(int percent READ percent WRITE setPercent NOTIFY percentChanged)
    Q_PROPERTY(int score READ score WRITE setScore NOTIFY scoreChanged)

signals:
    void percentChanged();
    void scoreChanged();

public:
    ProgressBar(QObject *parent=nullptr) : QObject(parent), m_percent(0.0) {};
    ~ProgressBar() = default;

    int percent() const { return m_percent; }
    void setPercent(int percent) {
        if (m_percent != percent) {
            m_percent = percent;
            emit percentChanged();
        }
    }

    int score() const { return m_score; }
    void setScore(int score) {
        if (m_score != score) {
            m_score = score;
            emit scoreChanged();
        }
    }

    void reset() {
        m_percent = 0.0;
        m_lastWasOverThreshold = false;
        m_score = 0;
        emit percentChanged();
        emit scoreChanged();
    }

protected:
    friend class LevelMeasureWindow;
    int m_percent=0; // Current value of the progress bar (from 0 to 100)
    bool m_lastWasOverThreshold = false; // Flag to check if the last value was over the threshold
    int m_score = 0; // Score for the progress bar, used to track the level measure
};

class LevelMeasureWindow : public QObject {
    Q_OBJECT
    Q_PROPERTY(QQmlListProperty<ProgressBar> progressBars READ progressBars NOTIFY progressBarsChanged)
    Q_PROPERTY(int threshold READ threshold WRITE setThreshold NOTIFY thresholdChanged)
    Q_PROPERTY(int nbRows READ nbRows WRITE setNbRows NOTIFY nbRowsChanged)
    Q_PROPERTY(int nbCols READ nbCols WRITE setNbCols NOTIFY nbColsChanged)

signals:
    void progressBarsChanged();
    void thresholdChanged();
    void nbRowsChanged();
    void nbColsChanged();

public:
    LevelMeasureWindow(QQmlEngine* engine, Kernel::ILogManager& logManager);
    ~LevelMeasureWindow();

    Q_INVOKABLE void resetScore();
    
    int nbRows() const;
    void setNbRows(int nbRows);

    int nbCols() const;
    void setNbCols(int nbCols);

    int threshold() const;
    void setThreshold(int threshold);

    QQmlListProperty<ProgressBar> progressBars();

    QQuickItem* visualItem() const { return m_visualItem; }

    void reset();
    void refresh(CMatrix* matrix);

protected:
    QList<ProgressBar *> m_progressBars; // List of progress bars
    QQmlEngine *m_jsEngine = nullptr; // JavaScript engine for QML
    QQuickItem *m_visualItem = nullptr;
    Kernel::ILogManager& m_logManager;

    int m_threshold = 10; // Threshold for the level measure
    int m_nbRows = 1;
    int m_nbCols = 1;
};

}
}
}
