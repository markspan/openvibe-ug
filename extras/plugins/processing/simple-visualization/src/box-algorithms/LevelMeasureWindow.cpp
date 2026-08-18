// SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

#include "LevelMeasureWindow.h"

#include <QQuickView>
#include <QQmlContext>

namespace OpenViBE {
namespace Plugins {
namespace SimpleVisualization {

LevelMeasureWindow::LevelMeasureWindow(QQmlEngine *engine, Kernel::ILogManager& logManager) : m_jsEngine(engine), m_logManager(logManager)
{
    qRegisterMetaType<ProgressBar *>();

    bool openGLSupported = QQuickWindow::graphicsApi() == QSGRendererInterface::OpenGLRhi;
    
    QQmlContext *context = new QQmlContext(m_jsEngine->rootContext());
    
    context->setContextProperty("dataSource", this);
    context->setContextProperty("openGLSupported", openGLSupported);
#if defined TARGET_OS_Windows
    context->setContextProperty("TARGET_OS_Windows", QVariant(true));
#else
    context->setContextProperty("TARGET_OS_Windows", QVariant(false));
#endif

    QQmlComponent component(m_jsEngine, QUrl("qrc:/qml/LevelMeasureWindow.qml"));
    QObject *object = component.create(context);
    if (!object) {
        m_logManager << Kernel::LogLevel_Error << "Failed to create QML object from LevelMeasureWindow.qml\n";
        return;
    }
    m_visualItem = qobject_cast<QQuickItem *>(object);
    if (m_visualItem) {
        context->setParent(m_visualItem);
    } else {
        m_logManager << Kernel::LogLevel_Error << "Failed to cast QQuickItem from LevelMeasureWindow.qml\n";
        delete context;
    }
}

LevelMeasureWindow::~LevelMeasureWindow()
{
    // Clean up the visual item before anything else
    if (m_visualItem) {
        m_visualItem->deleteLater();
        m_visualItem = nullptr;
    }
    m_jsEngine = nullptr;
    m_progressBars.clear();
}


QQmlListProperty<ProgressBar>  LevelMeasureWindow::progressBars() {
    return QQmlListProperty<ProgressBar>(this, &m_progressBars);
}

void LevelMeasureWindow::resetScore() 
{
    for (auto* bar : m_progressBars) {
        bar->reset();
    }
}
    
int LevelMeasureWindow::threshold() const
{
    return m_threshold;
}

void LevelMeasureWindow::setThreshold(int threshold)
{
    if (m_threshold != threshold) {
        m_threshold = threshold;
        emit thresholdChanged();
    }
}

int LevelMeasureWindow::nbRows() const 
{
    return m_nbRows;
}

void LevelMeasureWindow::setNbRows(int nbRows) 
{
    if (m_nbRows != nbRows) {
        m_nbRows = nbRows;
        emit nbRowsChanged();
    }
}   

int LevelMeasureWindow::nbCols() const 
{
    return m_nbCols;
}

void LevelMeasureWindow::setNbCols(int nbCols) 
{
    if (m_nbCols != nbCols) {
        m_nbCols = nbCols;
        emit nbColsChanged();
    }
}

void LevelMeasureWindow::reset()
{
    for(auto& bar : m_progressBars) {
        delete bar;
    }
    m_progressBars.clear();

    int size = m_nbRows * m_nbCols;

    for(int i = 0; i < size; ++i) {
        auto* bar = new ProgressBar();
        bar->moveToThread(QCoreApplication::instance()->thread());
        m_progressBars.push_back(bar);
    }
    
    emit progressBarsChanged();
}

void LevelMeasureWindow::refresh(CMatrix* matrix) 
{
    double *buffer = matrix->getBuffer();

    for(size_t i = 0; i < matrix->getBufferElementCount(); ++i) {
        int percent = 100*(*buffer);
        if (percent > 100) { 
            percent = 100;
        } else if (percent < 0) {
            percent = 0; 
        }

        if (percent > m_threshold && !m_progressBars[i]->m_lastWasOverThreshold) {
            m_progressBars[i]->m_score++;
            m_progressBars[i]->m_lastWasOverThreshold = true;
        }
        if (percent <= m_threshold) { 
            m_progressBars[i]->m_lastWasOverThreshold = false;
        }

        m_progressBars[i]->setPercent(percent);
        m_progressBars[i]->scoreChanged(); //update the text of the progress bar
        
        buffer++;
    }

    emit progressBarsChanged();
}

}
}
}
