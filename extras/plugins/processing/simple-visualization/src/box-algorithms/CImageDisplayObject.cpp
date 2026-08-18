// SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

#include "CImageDisplayObject.h"

#include <QQuickItem>
#include <QQmlEngine>
#include <QQmlComponent>
#include <QQmlContext>


namespace OpenViBE {
namespace Plugins {
namespace SimpleVisualization {


CImageDisplayObject::CImageDisplayObject(Kernel::ILogManager& logManager, const bool scaleToFit, QQmlEngine* engine)
    : m_logManager(logManager),
      m_scaleToFit(scaleToFit),
      m_jsEngine(engine)
{
    QQmlContext *context = new QQmlContext(m_jsEngine->rootContext());


    context->setContextProperty("scaleToFit", QVariant(m_scaleToFit));

    QQmlComponent component(m_jsEngine, QUrl("qrc:/qml/OVChart.qml"));

    QObject *object = component.create(context);
    if (!object) {
        m_logManager << Kernel::LogLevel_Error << "Failed to create QML object from OVChart.qml\n";
        return;
    }
    m_visualItem = qobject_cast<QQuickItem *>(object);
    if (m_visualItem) {
        context->setParent(m_visualItem);
    } else {
        m_logManager << Kernel::LogLevel_Error << "Failed to cast QQuickItem from OVChart.qml\n";
        delete context;
    }
}

CImageDisplayObject::~CImageDisplayObject()
{
    if (m_visualItem) {
        m_visualItem->deleteLater();
        m_visualItem = nullptr;
    }
    m_jsEngine = nullptr;
}

void CImageDisplayObject::setImage(const std::string& image)
{
    if (!m_visualItem) {
        m_logManager << Kernel::LogLevel_Error << "Visual Item is not initialized\n";
        return;
    }

    if ( image != m_visualItem->property("image").toString().toStdString() ) {
        if (image.empty()) {
            m_visualItem->setProperty("image", "");
        } else {
            m_visualItem->setProperty("image", QUrl::fromLocalFile(image.c_str()).toString());
        }
    }
}

}  // namespace SimpleVisualization
}  // namespace Plugins
}  // namespace OpenViBE