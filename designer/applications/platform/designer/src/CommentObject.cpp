// SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

#include "CommentObject.h"

namespace OpenViBE {
namespace Designer {

CommentObject::CommentObject(Kernel::IKernelContext* kernelCtx, Kernel::IComment *comment) : m_kernelCtx(kernelCtx), m_comment(comment)
{
    if (m_comment)
    {
        CString xString = m_comment->getAttributeValue(OV_AttributeId_Comment_XCenterPosition);
        CString yString = m_comment->getAttributeValue(OV_AttributeId_Comment_YCenterPosition);
        std::string xStringStd = xString.toASCIIString();
        std::string yStringStd = yString.toASCIIString();
        if(!xStringStd.empty() && !yStringStd.empty())
        {
            m_center = QPoint(std::stoi(xStringStd), std::stoi(yStringStd));
        }
    }

}

CommentObject::~CommentObject() 
{
}

CIdentifier CommentObject::id() const
{
    return m_comment->getIdentifier();
}

QString CommentObject::identifier() const
{
    return QString::fromStdString(m_comment->getIdentifier().str());
}

QString CommentObject::text() const
{
    return m_comment->getText().toASCIIString();
}

QPoint CommentObject::center() const
{
    return m_center;
}

void CommentObject::setText(const QString &text)
{
    QString old_text = m_comment->getText().toASCIIString();
    if (old_text != text)
    {
        m_comment->setText(text.toStdString().c_str());
        emit textChanged();
        emit edited(tr("Edit comment"), 0);
    }
}

void CommentObject::setCenter(const QPoint &center)
{
    if (m_center != center)
    {
        m_center = center;
        CString xString = std::to_string(center.x()).c_str();
        CString yString = std::to_string(center.y()).c_str();
        m_comment->setAttributeValue(OV_AttributeId_Comment_XCenterPosition, xString);
        m_comment->setAttributeValue(OV_AttributeId_Comment_YCenterPosition, yString);
        emit centerChanged();
    }
}

void CommentObject::notifyCenterChanged(int new_x, int new_y)
{
    setCenter(QPoint(new_x, new_y));
}


}
}