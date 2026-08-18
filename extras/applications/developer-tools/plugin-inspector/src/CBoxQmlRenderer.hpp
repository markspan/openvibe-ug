#pragma once

#include "base.hpp"

#include <memory>

#include <QQmlComponent>
#include <QQmlEngine>
#include <QQuickRenderControl>
#include <QQuickWindow>

QT_BEGIN_NAMESPACE
class QRhiTexture;
class QRhiTextureRenderTarget;
class QRhiRenderPassDescriptor;
QT_END_NAMESPACE

namespace OpenViBE {
namespace PluginInspector {

/**
 * @brief Renders a box offscreen using the designer's own Box.qml.
 *
 * Keeps the docs and the designer on a single definition of "what a box looks like":
 * rather than redrawing the box with QPainter, this instantiates OVVisualization/Box.qml
 * and grabs it.
 *
 * Box.qml normally binds to a Designer::BoxObject, which lives in the designer executable.
 * We hand it a lightweight QVariantMap model instead - the invokables it would otherwise
 * call (updatePortOffset, notifyCenterChanged) are optional calls on the QML side, and they
 * only feed back drag positions that a snapshot does not need.
 *
 * Rendering uses QQuickRenderControl, so no window is ever created. It requires the default
 * (accelerated) scenegraph adaptation: the software backend is rejected outright with
 * "QRhi is only compatible with default adaptation", and it cannot draw the box drop shadow
 * either - which matters, because the designer canvas is white too, so the shadow is the
 * only thing separating a box from its background.
 */
class CBoxQmlRenderer final
{
public:
    CBoxQmlRenderer();
    ~CBoxQmlRenderer();

    CBoxQmlRenderer(const CBoxQmlRenderer&)            = delete;
    CBoxQmlRenderer& operator=(const CBoxQmlRenderer&) = delete;

    /// False when the scenegraph or Box.qml could not be set up; Render() will always fail.
    bool isValid() const { return m_valid; }

    /// Renders one box. Returns a null QImage on failure.
    QImage Render(const Kernel::IKernelContext& ctx, const Kernel::IBox& box, const QString& displayName);

private:
    /// (Re)creates the offscreen texture the scenegraph draws into. Sizes vary per box.
    bool ensureRenderTarget(const QSize& size);
    void releaseRenderTarget();

    QQmlEngine m_engine;
    QQuickRenderControl m_renderControl;
    std::unique_ptr<QQuickWindow> m_window;
    std::unique_ptr<QQmlComponent> m_component;

    QRhiTexture* m_texture                    = nullptr;
    QRhiTextureRenderTarget* m_renderTarget   = nullptr;
    QRhiRenderPassDescriptor* m_passDescriptor = nullptr;

    bool m_valid = false;
};

}  // namespace PluginInspector
}  // namespace OpenViBE
