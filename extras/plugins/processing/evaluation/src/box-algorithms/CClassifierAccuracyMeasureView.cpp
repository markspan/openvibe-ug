// SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

#include "CClassifierAccuracyMeasureView.hpp"

#include <QQmlEngine>
#include <QQmlContext>
#include <QQuickItem>
#include <QQmlComponent>
#include <QQuickWindow>

namespace OpenViBE {
namespace Plugins {
namespace Evaluation {

class ProgressBarData : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString label READ label WRITE setLabel NOTIFY labelChanged)
    Q_PROPERTY(int score READ score WRITE setScore NOTIFY scoreChanged)
    Q_PROPERTY(int nstimulation READ nstimulation WRITE setNstimulation NOTIFY nstimulationChanged)

signals:
    void labelChanged();
    void scoreChanged();
    void nstimulationChanged();

public:
    ProgressBarData() = default;
    ProgressBarData(const ProgressBarData&) = default;
    ProgressBarData& operator=(const ProgressBarData&) = default;
    ~ProgressBarData() = default;

    QString label() const { return m_label; }
    void setLabel(const QString& label) {
        if(label != m_label) {
            m_label = label;
            emit labelChanged(); 
        }
    }

    int score() const { return m_score; }
    void setScore(int score) {
        if(score != m_score) {
            m_score = score;
            emit scoreChanged();
        }
    }

    int nstimulation() const { return m_nstimulation; }
    void setNstimulation(int nstimulation) {
        if(m_nstimulation != nstimulation) {
            m_nstimulation = nstimulation;
            emit nstimulationChanged();
        }
    }

protected:
    QString m_label;
    int m_score = 0;
    int m_nstimulation = 0;
};

ClassifierAccuracyMeasureView::ClassifierAccuracyMeasureView(Kernel::ILogManager &logMgr, QQmlEngine* engine) : m_logManager(logMgr), m_jsEngine(engine)
{
    qRegisterMetaType<ProgressBarData*>();

    bool openGLSupported = QQuickWindow::graphicsApi() == QSGRendererInterface::OpenGLRhi;
    if (!openGLSupported) {
        m_logManager << Kernel::LogLevel_Warning << "OpenGL is not set as the graphics backend, so AbstractSeries.useOpenGL will not work.";
        m_logManager << Kernel::LogLevel_Warning << "Set QSG_RHI_BACKEND=opengl environment variable to force the OpenGL backend to be used.";
    }
    
    QQmlContext *context = new QQmlContext(m_jsEngine->rootContext());

    context->setContextProperty("dataSourceAccuracy", this);
    context->setContextProperty("dataSourceROC", nullptr);

    context->setContextProperty("openGLSupported", openGLSupported);
#if defined TARGET_OS_Windows
    context->setContextProperty("TARGET_OS_Windows", QVariant(true));
#else
    context->setContextProperty("TARGET_OS_Windows", QVariant(false));
#endif

    QQmlComponent component(m_jsEngine, QUrl("qrc:/qml/ClassifierAccuracyMeasure.qml"));

    QObject *object = component.create(context);
    if (!object) {
        m_logManager << Kernel::LogLevel_Error << "Failed to create QML object from ClassifierAccuracyMeasure.qml\n";
        return;
    }

    m_visualItem = qobject_cast<QQuickItem *>(object);
    if (m_visualItem) {
        context->setParent(m_visualItem);
    } else {
        m_logManager << Kernel::LogLevel_Error << "Failed to cast QQuickItem from ClassifierAccuracyMeasure.qml\n";
        delete context;
    }
}

ClassifierAccuracyMeasureView::~ClassifierAccuracyMeasureView()
{
    qDeleteAll(m_progressBars);
    m_progressBars.clear();

    if (m_visualItem) {
        m_visualItem->deleteLater();
        m_visualItem = nullptr;
    }
    m_jsEngine = nullptr;
}

void ClassifierAccuracyMeasureView::addProgressBar(const QString& label)
{
    ProgressBarData* progressBar = new ProgressBarData();
    progressBar->setLabel(label);
    m_progressBars.append(progressBar);
    emit progressBarsChanged();
}

void ClassifierAccuracyMeasureView::removeProgressBar(const QString& label)
{
    for(auto it = m_progressBars.begin(); it != m_progressBars.end(); ++it) {
        if((*it)->label() == label) {
            ProgressBarData *progressBar = m_progressBars.takeAt(it - m_progressBars.begin());
            delete progressBar;
            emit progressBarsChanged();
            return;
        }
    }
}

uint ClassifierAccuracyMeasureView::nbProgressBars() const
{
    return m_progressBars.size();
}

void ClassifierAccuracyMeasureView::resetScores()
{
    for(auto *progress : m_progressBars) {
        progress->setScore(0);
        progress->setNstimulation(0);
    }
}

QQmlListProperty<ProgressBarData> ClassifierAccuracyMeasureView::progressBars()
{
    return QQmlListProperty<ProgressBarData>(this, &m_progressBars);
}

void ClassifierAccuracyMeasureView::incrementScore(int idx)
{
    if(idx < m_progressBars.size()) {
        m_progressBars[idx]->setScore(m_progressBars[idx]->score() + 1);
    } else {
        m_logManager << Kernel::LogLevel_Error << "in ClassifierAccuracyMeasureView: Invalid index "
                     << idx << " for incrementScore. There is " << uint(m_progressBars.size()) << " progressBars \n";
    }
}

void ClassifierAccuracyMeasureView::incrementNStimulation(int idx)
{
    if(idx < m_progressBars.size()) {
        m_progressBars[idx]->setNstimulation(m_progressBars[idx]->nstimulation() + 1);
    } else {
        m_logManager << Kernel::LogLevel_Error << "in ClassifierAccuracyMeasureView: Invalid index "
                     << idx << " for incrementNStimulation. There is " << uint(m_progressBars.size()) << " progressBars \n";
    }
}

#include "CClassifierAccuracyMeasureView.moc"

}
}
}