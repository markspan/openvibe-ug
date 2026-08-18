// SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

#include "CGrazVisuWindow.h"

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

CGrazVisuWindow::CGrazVisuWindow(CGrazVisualization* parent, QQmlEngine* engine): m_parent(parent), m_jsEngine(engine)
{

    //TODO create parameters

    QQmlContext *context = new QQmlContext(m_jsEngine->rootContext());


    context->setContextProperty("dataSource", this); //*m_channels);
#if defined TARGET_OS_Windows
    context->setContextProperty("TARGET_OS_Windows", QVariant(true));
#else
    context->setContextProperty("TARGET_OS_Windows", QVariant(false));
#endif

    QQmlComponent component(m_jsEngine, QUrl("qrc:/qml/CGrazVisuWindow.qml"));

    QObject *object = component.create(context);
    if (!object) {
        m_parent->getLogManager() << Kernel::LogLevel_Error << "Failed to create QML object from CGrazVisuWindow.qml\n";
        return;
    }
    
    m_visualItem = qobject_cast<QQuickItem *>(object);
    if (m_visualItem) {
        context->setParent(m_visualItem);
    } else {
        m_parent->getLogManager() << Kernel::LogLevel_Error << "Failed to cast QQuickItem from CGrazVisuWindow.qml\n";
        delete context;
    }
}

CGrazVisuWindow::~CGrazVisuWindow()
{
    if (m_visualItem) {
        m_visualItem->deleteLater();
        m_visualItem = nullptr;
    }
    m_jsEngine = nullptr;
}

int CGrazVisuWindow::getAccuracy(uint i, uint j)
{
    const double* buffer = m_parent->m_oConfusion->getBuffer();
    return int(buffer[j * 2 + i]);
}

QString CGrazVisuWindow::barImg()
{
    CString ovBarImg = Directories::getDataDir() + "/plugins/simple-visualization/openvibe-simple-visualization-GrazVisualization-bar.png"; //graz/bar.png";
    QString res(ovBarImg.toASCIIString());
    res = QUrl::fromLocalFile(res).toString();
    return res;
}

double CGrazVisuWindow::barScale()
{
    return m_parent->m_BarScale;
}

bool CGrazVisuWindow::delayFeedback()
{
    return m_parent->m_DelayFeedback;
}

bool CGrazVisuWindow::positiveFeedbackOnly()
{
    return m_parent->m_PositiveFeedbackOnly;
}

bool CGrazVisuWindow::showAccuracy()
{
    return m_parent->m_ShowAccuracy;
}

bool CGrazVisuWindow::showInstruction()
{
    return m_parent->m_ShowInstruction;
}

QString CGrazVisuWindow::state()
{
    switch(m_parent->m_CurrentState) {
        case CGrazVisualization::EStates::Idle: return "Idle";
        case CGrazVisualization::EStates::Reference: return "Reference";
        case CGrazVisualization::EStates::Cue: return "Cue";
        case CGrazVisualization::EStates::ContinousFeedback: return "ContinousFeedback";
        default: return "Unknown";
    }
}

QString CGrazVisuWindow::direction()
{
    switch(m_parent->m_CurrentDirection) {
        case CGrazVisualization::EArrowDirections::Left: return "Left";
        case CGrazVisualization::EArrowDirections::Right: return "Right";
        case CGrazVisualization::EArrowDirections::Up: return "Up";
        case CGrazVisualization::EArrowDirections::Down: return "Down";
        default: return "None";
    }
}

bool CGrazVisuWindow::showFeedback()
{
    return m_parent->m_ShowFeedback;
}

QStringList CGrazVisuWindow::imgsPaths()
{
    return m_imgsPaths;
}

void CGrazVisuWindow::setImgsPaths(const std::vector<std::string>& paths)
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
    