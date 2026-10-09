// SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

#include "CImageDisplayObject.h"

#include <QQuickItem>
#include <QQuickWindow>
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

    QQmlComponent component(m_jsEngine, QUrl("qrc:/qml/OVDisplayCueImage.qml"));

    QObject *object = component.createWithInitialProperties({{"scaleToFit", QVariant(m_scaleToFit)}}, context);
    if (!object)
    {
        for (const QQmlError &error : component.errors())
        {
            m_logManager << Kernel::LogLevel_Error << "Cue image QML: " << error.toString().toUtf8().constData() << "\n";
        }
        m_logManager << Kernel::LogLevel_Error << "Failed to create QML object from OVDisplayCueImage.qml\n";
        delete context;
        return;
    }
    m_visualItem = qobject_cast<QQuickItem *>(object);
    if (m_visualItem)
    {
        context->setParent(m_visualItem);
    }
    else
    {
        m_logManager << Kernel::LogLevel_Error << "Failed to cast QQuickItem from OVDisplayCueImage.qml\n";
        delete object;
        delete context;
    }
}

CImageDisplayObject::~CImageDisplayObject()
{
    if (m_visualItem)
    {
        m_visualItem->deleteLater();
        m_visualItem = nullptr;
    }
    m_jsEngine = nullptr;
}

void CImageDisplayObject::setImage(const std::string& image, std::function<void()> onDisplayed)
{
    if (!m_visualItem)
    {
        m_logManager << Kernel::LogLevel_Error << "Visual Item is not initialized\n";
        return;
    }

    if (image == m_currentImage)
    {
        // Already on screen; nothing will be repainted, so report it right away.
        onDisplayed();
        return;
    }
    m_currentImage = image;

    // This runs on the player thread (processClock). "image" is declared in QML, so reading or
    // writing it goes through the QML engine's JS heap, which belongs to the GUI thread and has no
    // locking: touching it from here races the garbage collector and every binding evaluation, and
    // corrupts the heap for the whole engine. Post the write to the item's own thread instead.
    //  With the item as context object, Qt drops the call if the item is destroyed before the event is delivered.
    const QString url = image.empty() ? QString() : QUrl::fromLocalFile(QString::fromStdString(image)).toString();
    QMetaObject::invokeMethod(m_visualItem, [item = QPointer<QQuickItem>(m_visualItem), url, onDisplayed = std::move(onDisplayed)]()
    {
        if (!item) { return; }
        item->setProperty("image", url);

        // The caller wants to know when the image is actually on screen, not when the property is
        // set: that is the first frame swapped after this write. A hidden or not-yet-shown window
        // never swaps, so report immediately in that case rather than never.
        QQuickWindow* window = item->window();
        if (window == nullptr || !window->isExposed())
        {
            onDisplayed();
            return;
        }
        // frameSwapped is emitted from the render thread; the callback contract allows that. The item
        // as context object drops the connection if it goes away before the frame does.
        QObject::connect(window, &QQuickWindow::frameSwapped, item, onDisplayed,
                         Qt::ConnectionType(Qt::DirectConnection | Qt::SingleShotConnection));
    }, Qt::QueuedConnection);
}

}  // namespace SimpleVisualization
}  // namespace Plugins
}  // namespace OpenViBE
