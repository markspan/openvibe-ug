// SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

#include "LinkObject.h"
#include "BoxObject.h"

namespace OpenViBE {
namespace Designer {

LinkObject::LinkObject(Kernel::IKernelContext* context, Kernel::ILink *link, BoxObject *sourceBox, BoxObject *targetBox)
    : m_context(context),
        m_link(link),
        m_sourceBox(sourceBox),
        m_targetBox(targetBox)
{
    connect(m_sourceBox, &BoxObject::centerChanged, this, &LinkObject::sourcePositionChanged);
    connect(m_sourceBox, &BoxObject::portOffsetsChanged, this, &LinkObject::sourcePositionChanged);

    connect(m_targetBox, &BoxObject::centerChanged, this, &LinkObject::targetPositionChanged);
    connect(m_targetBox, &BoxObject::portOffsetsChanged, this, &LinkObject::targetPositionChanged);
}

LinkObject::~LinkObject()
{
}

QPoint LinkObject::sourcePosition() const
{
    QPointF offset = m_sourceBox->m_outputPortOffsets.at(m_link->getSourceBoxOutputIndex());
    return m_sourceBox->center() + offset.toPoint();
}

QPoint LinkObject::targetPosition() const
{
    QPointF offset = m_targetBox->m_inputPortOffsets.at(m_link->getTargetBoxInputIndex());
    return m_targetBox->center() + offset.toPoint();
}

QString LinkObject::sourceDataType() const
{
    return m_sourceBox->m_outputPorts.at(m_link->getSourceBoxOutputIndex())->dataType();
}

bool LinkObject::isLinked() const
{
    // When QML reads it from a LinkObject, it's always final.
    // UI can also create Link component that is not final for drawing.
    return true;
}

}
}