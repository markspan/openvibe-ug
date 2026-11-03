// SPDX-License-Identifier: AGPL-3.0-only
// UG fork: new file, see speller.md

#include "CP300SpellerView.h"

#include <system/ovCTime.h>

#include <QMetaObject>
#include <QPointer>
#include <QQmlComponent>
#include <QQmlContext>
#include <QQmlEngine>
#include <QQuickItem>
#include <QQuickWindow>
#include <QStringList>
#include <QVariantList>

namespace OpenViBE {
namespace Plugins {
namespace SimpleVisualization {

namespace {

template <typename T, size_t N>
QVariantList toVariantList(const std::array<T, N>& values)
{
    QVariantList list;
    for (const T& value : values) { list.append(QVariant::fromValue(value)); }
    return list;
}

}  // namespace

/// The window the frame tracking is connected to. GUI thread only.
struct CP300SpellerView::SWindowLink
{
    QPointer<QQuickWindow> window;
    QMetaObject::Connection synchronized;
    QMetaObject::Connection swapped;

    void connectTo(QQuickWindow* newWindow, QObject* context, const std::shared_ptr<CP300FrameTracker>& tracker)
    {
        if (window == newWindow) { return; }
        QObject::disconnect(synchronized);
        QObject::disconnect(swapped);
        window = newWindow;
        if (!newWindow) { return; }

        // Both come from the render thread, which is why the connections are direct and the tracker locks.
        synchronized = QObject::connect(newWindow, &QQuickWindow::afterSynchronizing, context, [tracker]() { tracker->onSynchronized(); },
                                        Qt::DirectConnection);
        swapped = QObject::connect(newWindow, &QQuickWindow::frameSwapped, context, [tracker]() { tracker->onSwapped(System::Time::zgetTimeRaw(false)); },
                                   Qt::DirectConnection);
    }
};

CP300SpellerView::CP300SpellerView(Kernel::ILogManager& logManager, QQmlEngine* engine, const SP300SpellerLayout& layout, const SP300SpellerLook& look)
    : m_logManager(logManager), m_tracker(std::make_shared<CP300FrameTracker>()), m_windowLink(std::make_shared<SWindowLink>())
{
    QStringList labels;
    for (const std::string& label : layout.labels) { labels.append(QString::fromStdString(label)); }

    auto* context = new QQmlContext(engine->rootContext());
    QQmlComponent component(engine, QUrl("qrc:/qml/P300SpellerVisualization.qml"));

    QObject* object = component.createWithInitialProperties({{"rows", layout.rows}, {"columns", layout.columns}, {"labels", labels},
                                                             {"backgroundColors", toVariantList(look.background)},
                                                             {"foregroundColors", toVariantList(look.foreground)},
                                                             {"fontSizes", toVariantList(look.fontSize)}}, context);
    if (!object) {
        for (const QQmlError& error : component.errors()) { m_logManager << Kernel::LogLevel_Error << "P300 speller QML: " << error.toString().toUtf8().constData() << "\n"; }
        delete context;
        return;
    }
    m_visualItem = qobject_cast<QQuickItem*>(object);
    if (m_visualItem) { context->setParent(m_visualItem); }
    else {
        m_logManager << Kernel::LogLevel_Error << "P300SpellerVisualization.qml is not an item\n";
        delete object;
        delete context;
    }
}

CP300SpellerView::~CP300SpellerView()
{
    if (m_visualItem) {
        m_visualItem->deleteLater();
        m_visualItem = nullptr;
    }
}

uint64_t CP300SpellerView::show(const CP300SpellerState& state)
{
    const uint64_t update = ++m_lastUpdate;
    if (!m_visualItem) { return update; }

    QVariantList styles;
    for (const int style : state.styles()) { styles.append(style); }
    const QString target = QString::fromStdString(state.targetText());
    const QString result = QString::fromStdString(state.resultMarkup());

    // On the player thread here. The properties are declared in QML, so reading or writing them goes through the JS heap
    // of the engine, which belongs to the GUI thread: post the write there. With the item as context object, Qt drops the
    // call if the item is destroyed first.
    QMetaObject::invokeMethod(m_visualItem, [item = QPointer<QQuickItem>(m_visualItem), tracker = m_tracker, link = m_windowLink, update, styles, target, result]()
    {
        if (!item) { return; }
        QQuickWindow* window = item->window();
        link->connectTo(window, item, tracker);

        item->setProperty("styles", styles);
        item->setProperty("targetText", target);
        item->setProperty("resultText", result);

        // A hidden window renders nothing: there will be no frame to wait for
        if (window == nullptr || !window->isExposed() || !item->isVisible()) { tracker->onDisplayedWithoutFrame(update, System::Time::zgetTimeRaw(false)); }
        else { tracker->onWritten(update); }
    }, Qt::QueuedConnection);

    return update;
}

bool CP300SpellerView::takeDisplayed(uint64_t& update, uint64_t& time) { return m_tracker->takeDisplayed(update, time); }

}  // namespace SimpleVisualization
}  // namespace Plugins
}  // namespace OpenViBE
