// SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

#include "WindowManagerNode.h"

namespace OpenViBE {
namespace Designer {

WindowManagerNode::WindowManagerNode(const Kernel::IKernelContext& ctx, NodeType type, QString id, QString boxId, QString boxName, QColor color):
  m_kernelCtx(ctx),
  m_nodeType(type),
  m_id(id),
  m_boxId(boxId),
  m_boxName(boxName),
  m_color(color)
{
}

QString WindowManagerNode::nodeType()
{
  switch(m_nodeType) {
    case WindowManagerNode::NodeType::Box:
      return "Box";
    case WindowManagerNode::NodeType::VerticalSplit:
      return "VerticalSplit";
    case WindowManagerNode::NodeType::HorizontalSplit:
      return "HorizontalSplit";
    case WindowManagerNode::NodeType::Undefined:
      return "Undefined";
  }
}

QString WindowManagerNode::id()
{
  return m_id;
}

QString WindowManagerNode::name()
{
  return m_boxName;
}

void WindowManagerNode::setName(QString name)
{
  if (name == m_boxName) {
    return;
  }
  m_boxName = name;
  emit nameChanged();
}

QColor WindowManagerNode::color()
{
  return m_color;
}

void WindowManagerNode::setColor(QColor color)
{
  if (color == m_color) {
    return;
  }
  m_color = color;
  emit colorChanged();
}

QQmlListProperty<WindowManagerNode> WindowManagerNode::children()
{
  return QQmlListProperty<WindowManagerNode>(this, &m_children);
}

bool WindowManagerNode::addChild(WindowManagerNode* child)
{
  if (child == nullptr) {
    return false;
  }

  m_children.append(child);
  return true;
}

QString WindowManagerNode::boxId()
{
  return m_boxId;
}

} // Designer
} // OpenViBE