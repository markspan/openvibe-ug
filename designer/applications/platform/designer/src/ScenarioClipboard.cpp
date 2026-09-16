// SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

#include "ScenarioClipboard.h"
#include "ScenarioSerializer.h"

#ifdef TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines
#include "ovp_global_defines.h"
#endif // TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines

#include <cmath>
#include <cstdlib>
#include <limits>
#include <map>

#include <QClipboard>
#include <QGuiApplication>
#include <QMimeData>

namespace OpenViBE {
namespace Designer {

namespace {

constexpr int GridSize = 16;

int attributeInt(const Kernel::IAttributable& item, const CIdentifier& attribute)
{
    return item.hasAttribute(attribute) ? std::atoi(item.getAttributeValue(attribute).toASCIIString()) : 0;
}

void setAttributeInt(Kernel::IAttributable& item, const CIdentifier& attribute, int value)
{
    const CString str = std::to_string(value).c_str();
    if (item.hasAttribute(attribute))
    {
        item.setAttributeValue(attribute, str);
    }
    else
    {
        item.addAttribute(attribute, str);
    }
}

int snapToGrid(int value)
{
    return int(std::lround(double(value) / GridSize)) * GridSize;
}

/** RAII owner of a temporary kernel scenario. */
class TemporaryScenario
{
public:
    explicit TemporaryScenario(Kernel::IKernelContext& ctx) : m_ctx(ctx)
    {
        if (!m_ctx.getScenarioManager().createScenario(m_id))
        {
            m_id = CIdentifier::undefined();
        }
    }
    ~TemporaryScenario()
    {
        if (valid())
        {
            m_ctx.getScenarioManager().releaseScenario(m_id);
        }
    }
    bool valid() const { return m_id != CIdentifier::undefined(); }
    Kernel::IScenario& scenario() { return m_ctx.getScenarioManager().getScenario(m_id); }

private:
    Kernel::IKernelContext& m_ctx;
    CIdentifier m_id;
};

}  // namespace

const char* const ScenarioClipboard::MimeType = "application/x-openvibe-scenario+xml";

ScenarioClipboard& ScenarioClipboard::instance()
{
    static ScenarioClipboard clipboard;
    return clipboard;
}

ScenarioClipboard::ScenarioClipboard()
{
    connect(QGuiApplication::clipboard(), &QClipboard::dataChanged, this, &ScenarioClipboard::contentChanged);
}

QByteArray ScenarioClipboard::payload() const
{
    QByteArray payload;
    const QMimeData* mimeData = QGuiApplication::clipboard()->mimeData();
    if (mimeData && mimeData->hasFormat(MimeType))
    {
        payload = mimeData->data(MimeType);
    }
    else if (!mimeData || mimeData->formats().isEmpty())
    {
        // An unreadable/empty system clipboard leaves the in-process copy in charge; anything else
        // copied by the user in the meantime supersedes it.
        payload = m_localPayload;
    }
    return payload;
}

bool ScenarioClipboard::copy(Kernel::IKernelContext& ctx, const ScenarioSerializer& serializer, const Kernel::IScenario& source,
                             const QList<CIdentifier>& boxIds, const QList<CIdentifier>& commentIds)
{
    if (boxIds.isEmpty() && commentIds.isEmpty())
    {
        return false;
    }

    TemporaryScenario temp(ctx);
    if (!temp.valid())
    {
        ctx.getLogManager() << Kernel::LogLevel_Error << "Failed to create the clipboard scenario\n";
        return false;
    }
    Kernel::IScenario& clipboard = temp.scenario();

    // Boxes keep their identifier inside the clipboard so links can be remapped trivially
    std::map<CIdentifier, CIdentifier> mapping;
    for (const CIdentifier& boxId : boxIds)
    {
        if (const Kernel::IBox* box = source.getBoxDetails(boxId))
        {
            CIdentifier newId;
            clipboard.addBox(newId, *box, boxId);
            mapping[boxId] = newId;
        }
    }
    for (const CIdentifier& commentId : commentIds)
    {
        if (const Kernel::IComment* comment = source.getCommentDetails(commentId))
        {
            CIdentifier newId;
            clipboard.addComment(newId, *comment, commentId);
        }
    }
    // Links are copied only when both ends are
    for (const auto& [sourceBoxId, clipboardBoxId] : mapping)
    {
        CIdentifier linkId = source.getNextLinkIdentifierFromBox(CIdentifier::undefined(), sourceBoxId);
        while (linkId != CIdentifier::undefined())
        {
            const Kernel::ILink* link = source.getLinkDetails(linkId);
            const auto target = mapping.find(link->getTargetBoxIdentifier());
            if (target != mapping.end())
            {
                CIdentifier newId;
                clipboard.connect(newId, clipboardBoxId, link->getSourceBoxOutputIndex(), target->second, link->getTargetBoxInputIndex(), CIdentifier::undefined());
            }
            linkId = source.getNextLinkIdentifierFromBox(linkId, sourceBoxId);
        }
    }

    const QByteArray xml = serializer.toXml(clipboard);
    if (xml.isEmpty())
    {
        return false;
    }

    // No text/plain flavour on purpose: a focused text field would otherwise paste the whole XML.
    auto* mimeData = new QMimeData();
    mimeData->setData(MimeType, xml);
    QGuiApplication::clipboard()->setMimeData(mimeData);

    if (QGuiApplication::clipboard()->ownsClipboard())
    {
        m_localPayload.clear();
    }
    else
    {
        ctx.getLogManager() << Kernel::LogLevel_Warning << "The system clipboard is not available, the copied selection can only be pasted in this application\n";
        m_localPayload = xml;
        emit contentChanged();
    }
    return true;
}

bool ScenarioClipboard::hasContent() const
{
    return !payload().isEmpty();
}

ScenarioClipboard::Pasted ScenarioClipboard::paste(Kernel::IKernelContext& ctx, const ScenarioSerializer& serializer, Kernel::IScenario& target, const QPoint& anchor) const
{
    Pasted pasted;
    const QByteArray xml = payload();
    if (xml.isEmpty())
    {
        return pasted;
    }

    TemporaryScenario temp(ctx);
    if (!temp.valid())
    {
        ctx.getLogManager() << Kernel::LogLevel_Error << "Failed to create the paste scenario\n";
        return pasted;
    }
    Kernel::IScenario& clipboard = temp.scenario();
    if (!serializer.fromXml(xml, clipboard))
    {
        return pasted;
    }

    // Fresh identifiers: the clipboard ones may still exist in the target (paste in place)
    std::map<CIdentifier, CIdentifier> mapping;
    int minX = std::numeric_limits<int>::max();
    int minY = std::numeric_limits<int>::max();

    CIdentifier id = clipboard.getNextBoxIdentifier(CIdentifier::undefined());
    while (id != CIdentifier::undefined())
    {
        CIdentifier newId;
        if (target.addBox(newId, *clipboard.getBoxDetails(id), CIdentifier::undefined()))
        {
            mapping[id] = newId;
            pasted.boxes.append(newId);
            const Kernel::IBox* box = target.getBoxDetails(newId);
            minX = std::min(minX, attributeInt(*box, OV_AttributeId_Box_XCenterPosition));
            minY = std::min(minY, attributeInt(*box, OV_AttributeId_Box_YCenterPosition));
        }
        id = clipboard.getNextBoxIdentifier(id);
    }

    id = clipboard.getNextCommentIdentifier(CIdentifier::undefined());
    while (id != CIdentifier::undefined())
    {
        CIdentifier newId;
        if (target.addComment(newId, *clipboard.getCommentDetails(id), CIdentifier::undefined()))
        {
            pasted.comments.append(newId);
            const Kernel::IComment* comment = target.getCommentDetails(newId);
            minX = std::min(minX, attributeInt(*comment, OV_AttributeId_Comment_XCenterPosition));
            minY = std::min(minY, attributeInt(*comment, OV_AttributeId_Comment_YCenterPosition));
        }
        id = clipboard.getNextCommentIdentifier(id);
    }

    id = clipboard.getNextLinkIdentifier(CIdentifier::undefined());
    while (id != CIdentifier::undefined())
    {
        const Kernel::ILink* link = clipboard.getLinkDetails(id);
        const auto source = mapping.find(link->getSourceBoxIdentifier());
        const auto dest = mapping.find(link->getTargetBoxIdentifier());
        if (source != mapping.end() && dest != mapping.end())
        {
            CIdentifier newId;
            target.connect(newId, source->second, link->getSourceBoxOutputIndex(), dest->second, link->getTargetBoxInputIndex(), CIdentifier::undefined());
        }
        id = clipboard.getNextLinkIdentifier(id);
    }

    // Move the group so that its top-left corner lands on the anchor
    if (!pasted.boxes.isEmpty() || !pasted.comments.isEmpty())
    {
        const int dx = anchor.x() - minX;
        const int dy = anchor.y() - minY;
        for (const CIdentifier& boxId : pasted.boxes)
        {
            Kernel::IBox* box = target.getBoxDetails(boxId);
            setAttributeInt(*box, OV_AttributeId_Box_XCenterPosition, snapToGrid(attributeInt(*box, OV_AttributeId_Box_XCenterPosition) + dx));
            setAttributeInt(*box, OV_AttributeId_Box_YCenterPosition, snapToGrid(attributeInt(*box, OV_AttributeId_Box_YCenterPosition) + dy));
        }
        for (const CIdentifier& commentId : pasted.comments)
        {
            Kernel::IComment* comment = target.getCommentDetails(commentId);
            setAttributeInt(*comment, OV_AttributeId_Comment_XCenterPosition, snapToGrid(attributeInt(*comment, OV_AttributeId_Comment_XCenterPosition) + dx));
            setAttributeInt(*comment, OV_AttributeId_Comment_YCenterPosition, snapToGrid(attributeInt(*comment, OV_AttributeId_Comment_YCenterPosition) + dy));
        }
    }

    return pasted;
}

}  // namespace Designer
}  // namespace OpenViBE
