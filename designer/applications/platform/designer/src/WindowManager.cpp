// SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

#include <QQuickWindow>

#include "WindowManager.h"
#include "DesignerScenario.h"

namespace OpenViBE {
namespace Designer {

WindowManager::WindowManager(const Kernel::IKernelContext& ctx, DesignerScenario* scenario):
  m_kernelCtx(ctx),
  m_tree(ctx)
{
  m_tree.init(&scenario->scenario());

  const std::string label = "Window";

  // Create window with one child slot
  m_tree.addWidget(m_windowID, CString(label.c_str()), VisualizationToolkit::EVisualizationWidget::Window, CIdentifier::undefined(),
                                  0, CIdentifier::undefined(), 1, CIdentifier::undefined());
}

WindowManager::~WindowManager()
{
  // First, clear the root and notify QML BEFORE deleting anything
  // This allows QML to safely unbind from the objects
  WindowManagerNode* oldRoot = m_root;
  m_root = nullptr;
  emit rootChanged();  // Notify QML that root is now null

  // Now clean up the tree hierarchy (non-box nodes only, boxes are in m_boxNodes)
  if (oldRoot && oldRoot->nodeType() != "Box") {
    clearTreeNodeChildren(oldRoot);
    delete oldRoot;
  }

  // Clean up all box nodes
  for (auto* node : m_boxNodes) {
    delete node;
  }
  m_boxNodes.clear();
}

void WindowManager::show()
{
  if (!m_showVisualizationWindow) {
    m_showVisualizationWindow = true;
    emit showVisualizationWindowChanged();
  }
}

void WindowManager::hide()
{
  if (m_showVisualizationWindow) {
    m_showVisualizationWindow = false;
    emit showVisualizationWindowChanged();
  }
}

void WindowManager::insertRootBox(const QString& id)
{
  VisualizationToolkit::IVisualizationWidget* window = m_tree.getWidget(m_windowID);

  auto it = std::find_if(m_boxNodes.begin(), m_boxNodes.end(), [id](const auto& it) {
     return it->id() == id;
  });
  if (it != m_boxNodes.end()) {
    CIdentifier rootID((*it)->id().toStdString());
    m_tree.parentWidget(rootID, m_windowID, 0);
  }

  updateTreeVisualRepresentation();
  emit unparentedBoxesChanged();
}

void WindowManager::insertBox(const QString& id, const QString& parentID, const QString& position) {
  CIdentifier cid(id.toStdString());
  CIdentifier parentCID(parentID.toStdString());
  VisualizationToolkit::EDragLocation location = getLocationFromString(position);
  if (m_tree.insertWidget(cid, parentCID, location)) {
    updateTreeVisualRepresentation();
  } else {
    m_kernelCtx.getLogManager() << Kernel::LogLevel_Error << "Failed to insert box into tree\n";
  }
  emit unparentedBoxesChanged();
}

void WindowManager::removeNode(const QString& id)
{
  CIdentifier cid(id.toStdString());
  m_tree.destroyHierarchy(cid, false);
  updateTreeVisualRepresentation();
  emit unparentedBoxesChanged();
}

void WindowManager::requestCloseWindow()
{
  emit windowCloseRequested();
}

void WindowManager::addVisualizationBox(BoxObject* box)
{
  // Todo: add boxes to visualizationTree for structure management.
  // This adds a box that is "standalone"
  // add invokable methods to handle drag/drop dealing with tree structure
  CIdentifier id;
  m_tree.addWidget(id, box->name().toStdString().c_str(), VisualizationToolkit::EVisualizationWidget::Box, CIdentifier::undefined(), 0, box->id(), 0, CIdentifier::undefined());
  auto boxNode = new WindowManagerNode(m_kernelCtx, WindowManagerNode::NodeType::Box, QString::fromStdString(id.str()), box->identifier(), box->name());
  m_boxNodes.push_back(boxNode);

  // Conntect box name change to boxNode name change
  QObject::connect(box, &BoxObject::nameChanged, boxNode, &WindowManagerNode::setName);

  emit boxesChanged();
  emit unparentedBoxesChanged();
}

void WindowManager::removeVisualizationBox(BoxObject* box)
{
  auto it = std::find_if(m_boxNodes.begin(), m_boxNodes.end(), [box](const auto& it) { return it->boxId() == box->identifier(); });
  if (it != m_boxNodes.end()) {
    // Remove from tree hierarchy
    CIdentifier id((*it)->id().toStdString());
    m_tree.destroyHierarchy(id, true);

    // Remove corresponding node
    delete *it;
    m_boxNodes.erase(it);

    updateTreeVisualRepresentation();
    emit boxesChanged();
    emit unparentedBoxesChanged();
  }
}

QQmlEngine* WindowManager::getQmlEngine() {
    return QmlEngineSingleton::getInstance().engine();
}

void WindowManager::setVisualizationItem(CIdentifier& boxID, QQuickItem* item) {
  auto it = std::find_if(m_boxNodes.begin(), m_boxNodes.end(), [boxID](const auto& it) {
    return it->boxId().toStdString() == boxID.str();
  });

  if (it != m_boxNodes.end()) {
    (*it)->setVisual(item);
  }
}

void WindowManager::clearVisualizationItem(CIdentifier& boxID) {
  auto it = std::find_if(m_boxNodes.begin(), m_boxNodes.end(), [boxID](const auto& it) {
    return it->boxId().toStdString() == boxID.str();
  });

  if (it != m_boxNodes.end()) {
    m_kernelCtx.getLogManager() << Kernel::LogLevel_Debug << "Clearing visualization item for box " << boxID.str() << "\n";
    (*it)->setVisual(nullptr);
  }
}

void WindowManager::setVisualizationItemFullScreen(CIdentifier& boxID, bool fullScreen)
{
    auto it = std::find_if(m_boxNodes.begin(), m_boxNodes.end(), [boxID](const auto& it) {
        return it->boxId().toStdString() == boxID.str();
    });

    if (it != m_boxNodes.end()) {
        (*it)->setFullScreen(fullScreen);
    }
}

QQmlListProperty<WindowManagerNode> WindowManager::boxes()
{
  return QQmlListProperty<WindowManagerNode>(this,
                                     this,
                                     &WindowManager::boxCount,
                                     &WindowManager::boxAt
  );
}

QQmlListProperty<WindowManagerNode> WindowManager::unparentedBoxes()
{
  return QQmlListProperty<WindowManagerNode>(this,
                                     this,
                                     &WindowManager::unparentedBoxCount,
                                     &WindowManager::unparentedBoxAt
  );
}

int WindowManager::boxCount()
{
  return m_boxNodes.size();
}

WindowManagerNode* WindowManager::boxAt(int index)
{
  return m_boxNodes.at(index);
}

int WindowManager::unparentedBoxCount()
{
  int count = 0;
  for (auto* node : m_boxNodes) {
    if (!isNodeInTree(node)) {
      count++;
    }
  }
  return count;
}

WindowManagerNode* WindowManager::unparentedBoxAt(int index)
{
  int currentIndex = 0;
  for (auto* node : m_boxNodes) {
    if (!isNodeInTree(node)) {
      if (currentIndex == index) {
        return node;
      }
      currentIndex++;
    }
  }

  // Should not happen since unparentedBoxCount() should provide the actual number of unparented boxes
  return nullptr;
}

qsizetype WindowManager::boxCount(QQmlListProperty<WindowManagerNode>* list)
{
  auto manager = static_cast<WindowManager*>(list->object);
  return static_cast<qsizetype>(manager->boxCount());
}

WindowManagerNode* WindowManager::boxAt(QQmlListProperty<WindowManagerNode>* list, qsizetype index)
{
  auto manager = static_cast<WindowManager*>(list->object);
  return manager->boxAt(static_cast<int>(index));
}

qsizetype WindowManager::unparentedBoxCount(QQmlListProperty<WindowManagerNode>* list)
{
  auto manager = static_cast<WindowManager*>(list->object);
  return static_cast<qsizetype>(manager->unparentedBoxCount());
}

WindowManagerNode* WindowManager::unparentedBoxAt(QQmlListProperty<WindowManagerNode>* list, qsizetype index)
{
  auto manager = static_cast<WindowManager*>(list->object);
  return manager->unparentedBoxAt(static_cast<int>(index));
}

WindowManagerNode::NodeType WindowManager::getTypeFromWidgetType(const VisualizationToolkit::EVisualizationWidget type)
{
  switch (type) {
    case VisualizationToolkit::EVisualizationWidget::Box:
      return WindowManagerNode::NodeType::Box;
    case VisualizationToolkit::EVisualizationWidget::HorizontalSplit:
      return WindowManagerNode::NodeType::HorizontalSplit;
    case VisualizationToolkit::EVisualizationWidget::VerticalSplit:
      return WindowManagerNode::NodeType::VerticalSplit;
    case VisualizationToolkit::EVisualizationWidget::Undefined:
    default:
      return WindowManagerNode::NodeType::Undefined;
  }
}

VisualizationToolkit::EDragLocation WindowManager::getLocationFromString(const QString& position)
{
  if (position == "top") {
    return VisualizationToolkit::EDragLocation::Top;
  } else if (position == "left") {
    return VisualizationToolkit::EDragLocation::Left;
  } else if (position == "right") {
    return VisualizationToolkit::EDragLocation::Right;
  } else if (position == "bottom") {
    return VisualizationToolkit::EDragLocation::Bottom;
  } else {
    m_kernelCtx.getLogManager() << Kernel::LogLevel_Error << "Invalid position [" << position.toStdString() << "]\n";
    return VisualizationToolkit::EDragLocation::Top;
  }
}

void WindowManager::clearTreeNodeChildren(WindowManagerNode* node)
{
  if (node != nullptr) {
    for (auto child : node->childrenList()) {
      clearTreeNodeChildren(child);
      if (child->nodeType() != "Box") {
        delete child;
      }
    }
    node->childrenList().clear();
  }
}

void WindowManager::clearTreeVisualRepresentation()
{
  m_kernelCtx.getLogManager() << Kernel::LogLevel_Info << "Clearing tree visual representation, m_root=" << (void*)m_root << "\n";
  if (m_root && m_root->nodeType() != "Box") {
    clearTreeNodeChildren(m_root);
    delete m_root;
  }
  m_root = nullptr;
  m_kernelCtx.getLogManager() << Kernel::LogLevel_Info << "Cleared tree visual representation\n";
}

void WindowManager::updateTreeVisualRepresentation()
{
  m_kernelCtx.getLogManager() << Kernel::LogLevel_Info << "Updating tree visual representation\n";
  clearTreeVisualRepresentation();
  VisualizationToolkit::IVisualizationWidget* window = m_tree.getWidget(m_windowID);
  if (window->getNbChildren() > 0) {
    CIdentifier rootID;
    window->getChildIdentifier(0, rootID);

    if (rootID == CIdentifier::undefined()) {
      m_root = nullptr;
    } else {
      //build node from root info
      auto it = std::find_if(m_boxNodes.begin(), m_boxNodes.end(), [rootID](const auto& it) { return it->id().toStdString() == rootID.str(); });
      if (it != m_boxNodes.end()) {
        m_root = *it;
      } else {
        // Extract root ID info (vertical/horizontal, ID, ...) and create a node
        m_root = new WindowManagerNode(m_kernelCtx, getTypeFromWidgetType(m_tree.getWidget(rootID)->getType()), QString::fromStdString(rootID.str()), QString(""), QString(""));
        populateTreeNodeChildren(m_root);
      }
    }
  }
  m_kernelCtx.getLogManager() << Kernel::LogLevel_Info << "Emitting rootChanged, new m_root=" << (void*)m_root << "\n";
  emit rootChanged();
}

void WindowManager::populateTreeNodeChildren(WindowManagerNode* node)
{
  // Get children of node from tree
  // create child if it's a split, or get from boxNodes if it's a box.
  CIdentifier nodeID(node->id().toStdString());
  auto widget = m_tree.getWidget(nodeID);
  if (widget->getType() == VisualizationToolkit::EVisualizationWidget::HorizontalSplit
      || widget->getType() == VisualizationToolkit::EVisualizationWidget::VerticalSplit) {
    for(size_t i = 0; i < widget->getNbChildren(); ++i) {
      CIdentifier childID;
      widget->getChildIdentifier(i, childID);

      auto it = std::find_if(m_boxNodes.begin(), m_boxNodes.end(), [childID](const auto& it) { return it->id().toStdString() == childID.str(); });
      if (it != m_boxNodes.end()) {
        node->addChild(*it);
      } else {
        WindowManagerNode* child = new WindowManagerNode(m_kernelCtx, getTypeFromWidgetType(m_tree.getWidget(childID)->getType()), QString::fromStdString(childID.str()), QString(""), QString(""));
        node->addChild(child);
        populateTreeNodeChildren(child);
      }
    }
  }

}

bool WindowManager::isNodeInTree(WindowManagerNode* node) const
{
  if (!node) {
    return false;
  }

  CIdentifier widgetID(node->id().toStdString());
  CIdentifier parentID = m_tree.getWidget(widgetID)->getParentIdentifier();

  return parentID != CIdentifier::undefined();
}

}  // Designer
}  // OpenViBE