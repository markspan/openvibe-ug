#include "CBoxQmlRenderer.hpp"
#include "SnapshotPort.h"

#include <cmath>
#include <iostream>

#include <QCoreApplication>
#include <QQuickItem>
#include <QQuickRenderTarget>
#include <QVariantMap>

#include <rhi/qrhi.h>

namespace OpenViBE {
namespace PluginInspector {

namespace {
/// Margin kept around the box. Also has to leave room for the drop shadow.
constexpr int PADDING = 16;

/// Supersampling factor. The box is laid out at its normal logical size, but rasterized
/// at this multiple, so glyphs and the rounded corners keep their detail when the docs
/// are read on a high-DPI screen. The .dox files pin the display size back to logical.
constexpr int SCALE = 2;

/// Builds the port objects Box.qml's Repeaters iterate over.
QList<QObject*> buildPorts(const Kernel::IKernelContext& ctx, const Kernel::IBox& box, const bool outputs, QObject* owner)
{
    QList<QObject*> ports;
    const size_t count = outputs ? box.getOutputCount() : box.getInputCount();

    for (size_t i = 0; i < count; ++i) {
        CString name;
        CIdentifier typeID;

        if (outputs) {
            box.getOutputName(i, name);
            box.getOutputType(i, typeID);
        }
        else {
            box.getInputName(i, name);
            box.getInputType(i, typeID);
        }

        // The human readable type name is what StreamColorMapper.getStreamColor() switches on.
        const CString typeName = ctx.getTypeManager().getTypeName(typeID);
        ports.push_back(new SnapshotPort(QString::fromUtf8(name.toASCIIString()), QString::fromUtf8(typeName.toASCIIString()), owner));
    }

    return ports;
}
}  // namespace

CBoxQmlRenderer::CBoxQmlRenderer()
{
    m_window = std::make_unique<QQuickWindow>(&m_renderControl);
    m_window->setColor(Qt::white);

    if (!m_renderControl.initialize()) {
        std::cerr << "[  ERR  ] CBoxQmlRenderer: could not initialize the Qt Quick scenegraph. "
                     "Box snapshots need the accelerated (default) adaptation - check that a "
                     "graphics backend is available." << std::endl;
        return;
    }

    m_component = std::make_unique<QQmlComponent>(&m_engine);
    // Load through the module so Box.qml's own relative import and the
    // StreamColorMapper singleton resolve.
    m_component->loadFromModule("OVVisualization", "Box");

    if (m_component->isError()) {
        for (const auto& error : m_component->errors()) {
            std::cerr << "[  ERR  ] CBoxQmlRenderer: " << error.toString().toStdString() << std::endl;
        }
        return;
    }

    m_valid = true;
}

CBoxQmlRenderer::~CBoxQmlRenderer()
{
    releaseRenderTarget();
    m_renderControl.invalidate();
}

void CBoxQmlRenderer::releaseRenderTarget()
{
    delete m_renderTarget;
    m_renderTarget = nullptr;
    delete m_passDescriptor;
    m_passDescriptor = nullptr;
    delete m_texture;
    m_texture = nullptr;
}

bool CBoxQmlRenderer::ensureRenderTarget(const QSize& size)
{
    if (m_texture && m_texture->pixelSize() == size) { return true; }

    releaseRenderTarget();

    QRhi* rhi = m_renderControl.rhi();
    if (!rhi) { return false; }

    m_texture = rhi->newTexture(QRhiTexture::RGBA8, size, 1, QRhiTexture::RenderTarget | QRhiTexture::UsedAsTransferSource);
    if (!m_texture->create()) { return false; }

    m_renderTarget   = rhi->newTextureRenderTarget({ { m_texture } });
    m_passDescriptor = m_renderTarget->newCompatibleRenderPassDescriptor();
    m_renderTarget->setRenderPassDescriptor(m_passDescriptor);
    if (!m_renderTarget->create()) { return false; }

    // The texture is SCALE times the logical size; telling Qt Quick about it makes the
    // scene render at that density instead of just being stretched.
    QQuickRenderTarget target = QQuickRenderTarget::fromRhiRenderTarget(m_renderTarget);
    target.setDevicePixelRatio(SCALE);
    m_window->setRenderTarget(target);
    return true;
}

QImage CBoxQmlRenderer::Render(const Kernel::IKernelContext& ctx, const Kernel::IBox& box, const QString& displayName)
{
    if (!m_valid) { return {}; }

    // Owns the port objects for the duration of this render.
    QObject portOwner;

    QVariantMap properties;
    properties["index"]              = 0;
    properties["name"]               = displayName;
    properties["originalBoxName"]    = QString();  // empty suppresses the italic second line
    properties["identifier"]         = QString::fromUtf8(box.getIdentifier().str().c_str());
    properties["parameters"]         = QVariantList();
    properties["inputSupportTypes"]  = QStringList();
    properties["outputSupportTypes"] = QStringList();
    properties["flags"]              = 0;
    properties["initialInputCount"]  = int(box.getInputCount());
    properties["initialOutputCount"] = int(box.getOutputCount());
    properties["inputPorts"]         = QVariant::fromValue(buildPorts(ctx, box, false, &portOwner));
    properties["outputPorts"]        = QVariant::fromValue(buildPorts(ctx, box, true, &portOwner));

    // Box.qml derives its own width/height from its content and uses boxSize only for
    // `x: center.x - boxSize.width/2`, so a zero center and size park it at the origin.
    properties["center"]  = QPoint(0, 0);
    properties["boxSize"] = QSize(0, 0);

    // Stand-in for Designer::BoxObject. Its invokables are optional calls on the QML side.
    properties["model"] = QVariantMap{ { "modelData", QVariantMap() } };

    std::unique_ptr<QObject> object(m_component->createWithInitialProperties(properties, m_engine.rootContext()));

    if (!object) {
        for (const auto& error : m_component->errors()) {
            std::cerr << "[  ERR  ] CBoxQmlRenderer: " << error.toString().toStdString() << std::endl;
        }
        return {};
    }

    auto* item = qobject_cast<QQuickItem*>(object.get());
    if (!item) {
        std::cerr << "[  ERR  ] CBoxQmlRenderer: Box.qml did not produce a QQuickItem" << std::endl;
        return {};
    }

    item->setParentItem(m_window->contentItem());

    // Box.qml defers its layout with Qt.callLater and we never run an event loop,
    // so without pumping events here the size never settles.
    QCoreApplication::processEvents();

    const int w = int(std::ceil(item->width()));
    const int h = int(std::ceil(item->height()));

    if (w <= 0 || h <= 0) {
        std::cerr << "[  ERR  ] CBoxQmlRenderer: no size for " << displayName.toStdString() << std::endl;
        return {};
    }

    // The window keeps the logical geometry; only the texture is supersampled.
    const QSize target(w + 2 * PADDING, h + 2 * PADDING);
    item->setPosition(QPointF(PADDING, PADDING));
    m_window->setGeometry(0, 0, target.width(), target.height());

    if (!ensureRenderTarget(target * SCALE)) {
        std::cerr << "[  ERR  ] CBoxQmlRenderer: could not create the render target for " << displayName.toStdString() << std::endl;
        return {};
    }

    m_renderControl.polishItems();
    m_renderControl.beginFrame();
    m_renderControl.sync();
    m_renderControl.render();

    // Read the texture back within the same frame; endFrame() flushes and waits.
    QRhiReadbackResult readback;
    QRhiResourceUpdateBatch* batch = m_renderControl.rhi()->nextResourceUpdateBatch();
    batch->readBackTexture(QRhiReadbackDescription(m_texture), &readback);
    m_renderControl.commandBuffer()->resourceUpdate(batch);
    m_renderControl.endFrame();

    item->setParentItem(nullptr);

    if (readback.data.isEmpty()) {
        std::cerr << "[  ERR  ] CBoxQmlRenderer: empty readback for " << displayName.toStdString() << std::endl;
        return {};
    }

    const QImage wrapped(reinterpret_cast<const uchar*>(readback.data.constData()), readback.pixelSize.width(), readback.pixelSize.height(),
                         QImage::Format_RGBA8888_Premultiplied);

    // Detach from the readback buffer, and flip if the backend renders bottom-up.
    QImage image = m_renderControl.rhi()->isYUpInFramebuffer() ? wrapped.mirrored() : wrapped.copy();

    image = image.convertToFormat(QImage::Format_ARGB32);
    // Lets callers recover the logical size for the documentation's display width.
    image.setDevicePixelRatio(SCALE);

    return image;
}

}  // namespace PluginInspector
}  // namespace OpenViBE
