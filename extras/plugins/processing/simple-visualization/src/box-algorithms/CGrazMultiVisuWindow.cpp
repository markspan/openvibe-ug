// SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

#include "CGrazMultiVisuWindow.h"

#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickView>
#include <QQmlComponent>

#include <openvibe/ov_all.h>
#include <toolkit/ovtk_all.h>

namespace OpenViBE {
namespace Plugins {
namespace SimpleVisualization {

CGrazMultiVisuWindow::CGrazMultiVisuWindow(CGrazMultiVisualization* parent, QQmlEngine* engine): m_parent(parent), m_jsEngine(engine)
{

    //TODO create parameters

    QQmlContext *context = new QQmlContext(m_jsEngine->rootContext());

    context->setContextProperty("dataSource", this); //*m_channels);
#if defined TARGET_OS_Windows
    context->setContextProperty("TARGET_OS_Windows", QVariant(true));
#else
    context->setContextProperty("TARGET_OS_Windows", QVariant(false));
#endif

    QQmlComponent component(m_jsEngine, QUrl("qrc:/qml/CGrazMultiVisuWindow.qml"));
    QObject *object = component.create(context);
    if (!object) {
        m_parent->getLogManager() << Kernel::LogLevel_Error << "Failed to create QML object from CGrazMultiVisuWindow.qml\n";
        return;
    }

    m_visualItem = qobject_cast<QQuickItem *>(object);
    if (m_visualItem) {
        context->setParent(m_visualItem);
    } else {
        m_parent->getLogManager() << Kernel::LogLevel_Error << "Failed to cast QQuickItem from CGrazMultiVisuWindow.qml\n";
        delete context;
    }
}

CGrazMultiVisuWindow::~CGrazMultiVisuWindow()
{
    if (m_visualItem) {
        m_visualItem->deleteLater();
        m_visualItem = nullptr;
    }
    m_jsEngine = nullptr;
}

//void CGrazMultiVisuWindow::flushQueue()
//{
//    m_parent->FlushQueue();
//}

int CGrazMultiVisuWindow::getAccuracy(uint i, uint j)
{
    const double* buffer = m_parent->m_oConfusion->getBuffer();
    if(i> m_parent->m_nbModality || j> m_parent->m_nbModality) {
        return 0;
    }

    return int(buffer[j * m_parent->m_nbModality + i]);
}

QString CGrazMultiVisuWindow::barImg()
{
    CString ovBarImg = Directories::getDataDir() + "/plugins/simple-visualization/graz/bar.png";
    QString res(ovBarImg.toASCIIString());
    res = QUrl::fromLocalFile(res).toString();
    return res;
}

QList<double> CGrazMultiVisuWindow::barScales()
{
    m_barScales.clear();
    for(const auto& s : m_parent->m_barScales) {
        m_barScales.append(s);
    }
    return m_barScales;
}

bool CGrazMultiVisuWindow::delayFeedback()
{
    return m_parent->m_delayFeedback;
}

bool CGrazMultiVisuWindow::showAccuracy()
{
    return m_parent->m_showAccuracy;
}

bool CGrazMultiVisuWindow::showInstruction()
{
    return m_parent->m_showInstruction;
}

QString CGrazMultiVisuWindow::state()
{
    switch(m_parent->m_state) {
        case CGrazMultiVisualization::EStates::Idle: return "Idle";
        case CGrazMultiVisualization::EStates::Cross: return "Cross";
        case CGrazMultiVisualization::EStates::Black: return "Black";
        case CGrazMultiVisualization::EStates::Instruction: return "Instruction";
        case CGrazMultiVisualization::EStates::Feedback: return "Feedback";
        default: return "Unknown";
    }
}

int CGrazMultiVisuWindow::feedbackMode()
{
    return m_parent->m_feedbackMode;
}

int CGrazMultiVisuWindow::modality()
{
    return m_parent->m_modality;
}

int CGrazMultiVisuWindow::nbModality()
{
    return m_parent->m_nbModality;
}

int CGrazMultiVisuWindow::vote()
{
    return m_parent->m_vote;
}

QStringList CGrazMultiVisuWindow::imgsPaths()
{
    return m_imgsPaths;
}

void CGrazMultiVisuWindow::setImgsPaths(const std::vector<std::string>& paths)
{
    m_imgsPaths.clear();
    for(const auto& p : paths) {
        QString str = QString::fromStdString(p);
        m_imgsPaths.append(QUrl::fromLocalFile(str).toString());
    }
    emit imgsPathsChanged();
}

}
}
}
    