// SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

#include "CVisualizationTree.hpp"
#include "CVisualizationWidget.hpp"

#include "json/json.h"

namespace OpenViBE {
namespace Designer {

CVisualizationTree::~CVisualizationTree() {
    
}

bool CVisualizationTree::init(const Kernel::IScenario* scenario)
{
  m_scenario = scenario;
  return true;
}

bool CVisualizationTree::getNextID(CIdentifier& id,
                                   const std::function<bool(typename std::map<CIdentifier, std::unique_ptr<VisualizationToolkit::IVisualizationWidget>>::const_iterator)>& test) const
{
    typename std::map<CIdentifier, std::unique_ptr<VisualizationToolkit::IVisualizationWidget>>::const_iterator it;

    if (id == CIdentifier::undefined()) {
        it = m_widgets.begin();
    }
    else {
        it = m_widgets.find(id);
        if (it == m_widgets.end()) {
            id = CIdentifier::undefined();
            return false;
        }
        ++it;
    }

    while (it != m_widgets.end()) {
        if (test(it)) {
            id = it->first;
            return true;
        }
        ++it;
    }

    return false;
}
    
bool CVisualizationTree::getNextWidgetIdentifier(CIdentifier& id) const 
{
  return getNextID(id);
}

bool CVisualizationTree::getNextWidgetIdentifier(CIdentifier& id, VisualizationToolkit::EVisualizationWidget type) const 
{
  return getNextID(id, [type](const auto& it) { return it->second->getType() == type; });
}

VisualizationToolkit::IVisualizationWidget* CVisualizationTree::getWidget(const CIdentifier& id) const 
{
  const auto it = m_widgets.find(id);
  if (it == m_widgets.end()) {
    return nullptr;
  }
  return it->second.get();
}

VisualizationToolkit::IVisualizationWidget* CVisualizationTree::getWidgetFromBoxIdentifier(const CIdentifier& boxID) const 
{
  for (auto& widget : m_widgets) {
    if (widget.second->getBoxIdentifier() == boxID) {
      return widget.second.get();
    }
  }
  return nullptr;
}

CIdentifier CVisualizationTree::getUnusedIdentifier(const CIdentifier& suggestedID) const
{
  uint64_t id = (uint64_t(rand()) << 32) + uint64_t(rand());
  if (suggestedID != CIdentifier::undefined()) {
    id = suggestedID.id() - 1;
  }

  CIdentifier result;
  std::map<CIdentifier, std::unique_ptr<VisualizationToolkit::IVisualizationWidget>>::const_iterator i;
  do {
    id++;
    result = CIdentifier(id);
    i = m_widgets.find(result);
  } while (i != m_widgets.end() || result == CIdentifier::undefined());
  return result;
}

bool CVisualizationTree::addWidget(CIdentifier& id,
                                   const CString& name,
                                   VisualizationToolkit::EVisualizationWidget type,
                                   const CIdentifier& parentID,
                                   size_t parentIdx,
                                   const CIdentifier& boxID,
                                   size_t nChild,
                                   const CIdentifier& suggestedID) 
{
  m_kernelCtx.getLogManager() << Kernel::LogLevel_Debug << "Adding new visualization widget\n";

  //create new widget
  std::unique_ptr<VisualizationToolkit::IVisualizationWidget> widget(new CVisualizationWidget(m_kernelCtx));
  id = getUnusedIdentifier(suggestedID);

  if (!widget->initialize(id, name, type, parentID, boxID, nChild)) {
    m_kernelCtx.getLogManager() << Kernel::LogLevel_Error << "Failed to add new visualization widget (couldn't initialize it)\n";
    return false;
  }

  // assign a parent to it
  if (parentID != CIdentifier::undefined()) {
    m_kernelCtx.getLogManager() << Kernel::LogLevel_Debug << "Parenting visualization widget\n";
    VisualizationToolkit::IVisualizationWidget* parentWidget = getWidget(parentID);

    if (parentWidget != nullptr) {
      if (parentWidget->getType() == VisualizationToolkit::EVisualizationWidget::Window) {
        //extend number of children of parent window if necessary
        if (parentWidget->getNbChildren() <= parentIdx) {
          for (size_t i = parentWidget->getNbChildren(); i <= parentIdx; ++i) {
            parentWidget->addChild(CIdentifier::undefined());
          }
        }
      }

      if (!parentWidget->setChildIdentifier(parentIdx, id)) {
        m_kernelCtx.getLogManager() << Kernel::LogLevel_Error <<
            "Failed to add new visualization widget (couldn't set child identifier in parent window)\n";
        return false;
      }
    }
  }

  m_widgets[id] = std::move(widget);
  return true;
}

bool CVisualizationTree::getWidgetIndex(const CIdentifier& id, size_t& index) const 
{
  VisualizationToolkit::IVisualizationWidget* visualizationWidget = getWidget(id);
  if (!visualizationWidget) {
    m_kernelCtx.getLogManager() << Kernel::LogLevel_Error << "Failed to get widget.\n";
    return false;
  }

  const CIdentifier& parentIdentifier = visualizationWidget->getParentIdentifier();
  if (parentIdentifier == CIdentifier::undefined()) {
    m_kernelCtx.getLogManager() << Kernel::LogLevel_Error << "Failed to get parent identifier widget\n";
    return false;
  }

  VisualizationToolkit::IVisualizationWidget* parentVisualizationWidget = getWidget(parentIdentifier);
  if (!parentVisualizationWidget) {
    m_kernelCtx.getLogManager() << Kernel::LogLevel_Error << "Failed to get parent visualization widget\n";
    return false;
  }

  parentVisualizationWidget->getChildIndex(id, index);
  return true;
}

bool CVisualizationTree::unparentWidget(const CIdentifier& id, size_t& index) 
{
  VisualizationToolkit::IVisualizationWidget* widget = getWidget(id);
  if (!widget) {
    return false;
  }

  //get its parent identifier
  const CIdentifier& parentID = widget->getParentIdentifier();
  if (parentID == CIdentifier::undefined()) {
    return true;
  }

  //unparent widget
  widget->setParentIdentifier(CIdentifier::undefined());

  //retrieve parent and remove widget from its children list
  VisualizationToolkit::IVisualizationWidget* parentWidget = getWidget(parentID);
  if (parentWidget) {
    parentWidget->getChildIndex(id, index);
    parentWidget->removeChild(id);
  }
  return true;
}

bool CVisualizationTree::parentWidget(const CIdentifier& id, const CIdentifier& parentID, const size_t index) 
{
  if (parentID == CIdentifier::undefined()) {
    return false;
  }

  //retrieve widget to be parented
  VisualizationToolkit::IVisualizationWidget* widget = getWidget(id);
  if (!widget) {
    return false;
  }

  //retrieve its parent
  VisualizationToolkit::IVisualizationWidget* parentWidget = getWidget(parentID);
  if (!parentWidget) {
    return false;
  }

  widget->setParentIdentifier(parentID);
  parentWidget->setChildIdentifier(index, id);
  return true;
}

bool CVisualizationTree::destroyHierarchy(const CIdentifier& id, bool destroyVisualizationBoxes) 
{
  bool res = true;
  
  auto widget = m_widgets.find(id);
  if (widget == m_widgets.end()) {
    return false;
  }

  // Unpaarent and clean parent widget
  size_t index;
  CIdentifier parentID =widget->second->getParentIdentifier();
  unparentWidget(id, index);

  if ((widget->second->getType() == VisualizationToolkit::EVisualizationWidget::Box)) {
    cleanupWidget(parentID);
  } else {
    //If not a box, destroy all children
    CIdentifier childID;
    const size_t nChildren = widget->second->getNbChildren();
    for (size_t i = 0; i < nChildren; ++i) {
      widget->second->getChildIdentifier(i, childID);
      res &= destroyHierarchy(childID, destroyVisualizationBoxes);
    }
  }

  if (widget->second->getType() == VisualizationToolkit::EVisualizationWidget::Box && !destroyVisualizationBoxes) {
    // If it's a box and we don't want to destroy it, we just return true
    return true;
  }

  m_widgets.erase(widget);

  return res;
}

bool CVisualizationTree::insertWidget(const CIdentifier& id, const CIdentifier& targetID, VisualizationToolkit::EDragLocation location)
{
  // Check if widget already exists
  // If id == targetID, then do nothing
  // Insert widget next to target widget
  // Check if widget is already parented (somewhere else in the tree) and remove it if so.
  // Inserted widgets cannot have children, they are end-widgets

  
  if (id == targetID) {
    // widget inserted next to itself, layout will not change
    return true;
  }

  auto widget = getWidget(id);
  if (!widget) {
    // widget should be in the list of widgets
    m_kernelCtx.getLogManager() << Kernel::LogLevel_Error << "Failed to insert widget into tree (widget not found)\n";
    return false;
  }

  // Retrive current parent of widget if any, and remove connection at the end of the process
  CIdentifier widgetParentID = widget->getParentIdentifier();
  size_t index;
  unparentWidget(id, index);

  auto targetWidget = getWidget(targetID);
  if (!targetWidget) {
    m_kernelCtx.getLogManager() << Kernel::LogLevel_Error << "Failed to insert widget into tree (target widget not found)\n";
    return false;
  }

  CIdentifier targetParentID = targetWidget->getParentIdentifier();
  auto targetParentWidget = getWidget(targetParentID);
  if (!targetParentWidget) {
    m_kernelCtx.getLogManager() << Kernel::LogLevel_Error << "Failed to insert widget into tree (parent widget not found)\n";
    return false;
  }

  size_t targetIndexInParent = 0;
  targetParentWidget->getChildIndex(targetID, targetIndexInParent);

  // Insert widget, adding vertical/horizontal splits as needed
  
  CIdentifier newParentID;
  if (location == VisualizationToolkit::EDragLocation::Top || location == VisualizationToolkit::EDragLocation::Bottom) {
    // Adding a new parent widget for the widget and target.
    // targetParentID and targetIndexInParent info will be used to replace the target widget with the newly created widget in the tree.
    addWidget(newParentID, CString(""), VisualizationToolkit::EVisualizationWidget::VerticalSplit, targetParentID, targetIndexInParent, CIdentifier::undefined(), 2, CIdentifier::undefined());
  } else if (location == VisualizationToolkit::EDragLocation::Left || location == VisualizationToolkit::EDragLocation::Right) {
    addWidget(newParentID, CString(""), VisualizationToolkit::EVisualizationWidget::HorizontalSplit, targetParentID, targetIndexInParent, CIdentifier::undefined(), 2, CIdentifier::undefined());
  }

  if (newParentID == CIdentifier::undefined()) {
    m_kernelCtx.getLogManager() << Kernel::LogLevel_Error << "Failed to insert split widget in the tree\n";
    return false;
  }
    // Add widget and target as children of the new parent widget, depending on the location.
  parentWidget(id, newParentID, (location == VisualizationToolkit::EDragLocation::Top || location == VisualizationToolkit::EDragLocation::Left) ? 0 : 1);
  parentWidget(targetID, newParentID, (location == VisualizationToolkit::EDragLocation::Top || location == VisualizationToolkit::EDragLocation::Left) ? 1 : 0);

  if (!cleanupWidget(widgetParentID)) {
    m_kernelCtx.getLogManager() << Kernel::LogLevel_Error << "Widget inserted but failed to cleanup parent widget\n";
    return false;
  }
  return true;
}

bool CVisualizationTree::cleanupWidget(const CIdentifier& id)
{
  // This method is meant to cleanup a split widget that would have been left with only one child
  if (id == CIdentifier::undefined()) {
    return true;
  }

  auto widget = getWidget(id);
  if (!widget) {
    m_kernelCtx.getLogManager() << Kernel::LogLevel_Error << "Failed to cleanup widget: widget not found\n";
    return false;
  }

  if (widget->getType() != VisualizationToolkit::EVisualizationWidget::VerticalSplit && widget->getType() != VisualizationToolkit::EVisualizationWidget::HorizontalSplit) {
    // Nothing to cleanup
    return true;
  }

  // Get child and parent of widget
  // unparent widget and move parent its child at the index of the widget in the parent widget

  CIdentifier widgetParentID = widget->getParentIdentifier();
  if (widgetParentID == CIdentifier::undefined()) {
    m_kernelCtx.getLogManager() << Kernel::LogLevel_Error << "Failed to cleanup widget: parent not found\n";
    return false;
  }

  size_t index;
  unparentWidget(id, index);

  int validChildrenCount = 0;
  CIdentifier lastValidChildID = -1;
  for (size_t i = 0; i < widget->getNbChildren(); ++i) {
    CIdentifier childID;
    widget->getChildIdentifier(i, childID);
    if (childID != CIdentifier::undefined()) {
      validChildrenCount++;
      lastValidChildID = childID;
    }
  }

  if (validChildrenCount != 1) {
    m_kernelCtx.getLogManager() << Kernel::LogLevel_Debug << "Not cleaning widget " << id << ": more that one child left\n";
    return true;
  }

  if (!parentWidget(lastValidChildID, widgetParentID, index)) {
    m_kernelCtx.getLogManager() << Kernel::LogLevel_Error << "Failed to parent child to parent during widget cleanup\n";
    return false;
  }

  return true;

}

json::Object CVisualizationTree::serializeWidget(VisualizationToolkit::IVisualizationWidget& widget) const
{
  json::Object representation;

    representation["identifier"] = widget.getIdentifier().str().c_str();

    // visualization box name can be retrieved from corresponding IBox, so we can skip it for these
    if (widget.getType() != VisualizationToolkit::EVisualizationWidget::Box) {
    representation["name"] = widget.getName().toASCIIString();
  }

    representation["type"]             = int(widget.getType());
    representation["parentIdentifier"] = widget.getParentIdentifier().str().c_str();

    // visualization widget index
    VisualizationToolkit::IVisualizationWidget* parentWidget = this->getWidget(widget.getParentIdentifier());
    if (parentWidget) {
        size_t childIndex = 0;
        parentWidget->getChildIndex(widget.getIdentifier(), childIndex);
        representation["index"] = int(childIndex);
    }

    representation["boxIdentifier"] = widget.getBoxIdentifier().str().c_str();
    representation["childCount"]    = int(widget.getNbChildren());

    if (widget.getType() == VisualizationToolkit::EVisualizationWidget::Window) {
        representation["width"]  = int(widget.getWidth());
        representation["height"] = int(widget.getHeight());
    }
    if (widget.getType() == VisualizationToolkit::EVisualizationWidget::HorizontalSplit || widget.getType() == VisualizationToolkit::EVisualizationWidget::VerticalSplit) {
        representation["dividerPosition"]    = widget.getDividerPosition();
        representation["maxDividerPosition"] = widget.getMaxDividerPosition();
    }

    return representation;
}

CString CVisualizationTree::serialize() const 
{
    json::Array representation;

    std::vector<CIdentifier> widgetsToExport;

    CIdentifier widgetID;
    while (this->getNextWidgetIdentifier(widgetID)) {
      VisualizationToolkit::IVisualizationWidget* widget = this->getWidget(widgetID);
      if (widget->getType() == VisualizationToolkit::EVisualizationWidget::Window || widget->getParentIdentifier() == CIdentifier::undefined()) {
        widgetsToExport.push_back(widgetID);
      }
    }

    for (size_t i = 0; i < widgetsToExport.size(); ++i) {
      VisualizationToolkit::IVisualizationWidget* widget = this->getWidget(widgetsToExport[i]);
      
      representation.push_back(this->serializeWidget(*widget));

      for (size_t j = 0; j < widget->getNbChildren(); ++j) {
        if (widget->getChildIdentifier(j, widgetID)) {
          widgetsToExport.push_back(widgetID);
        }
      }
    }
     
    return CString(Serialize(representation).c_str());
}

bool CVisualizationTree::deserialize(const CString& tree) 
{
    // Empty this visualization tree
    auto widgetID = CIdentifier::undefined();
    while (this->getNextWidgetIdentifier(widgetID) && widgetID != CIdentifier::undefined()) {
        this->destroyHierarchy(widgetID, true);
        widgetID = CIdentifier::undefined();
    }

    json::Array representation = json::Deserialize(tree.toASCIIString());

    for (auto it = representation.begin(); it != representation.end(); ++it) {
        json::Value& jsonWidget = *it;

        widgetID.fromString(jsonWidget["identifier"].ToString());

        CIdentifier boxID;
        boxID.fromString(jsonWidget["boxIdentifier"].ToString());

        const VisualizationToolkit::EVisualizationWidget widgetType = VisualizationToolkit::EVisualizationWidget(jsonWidget["type"].ToInt());

        CString name;
        if (widgetType == VisualizationToolkit::EVisualizationWidget::Box) {
            const Kernel::IBox* box = m_scenario->getBoxDetails(boxID);
            if (!box) {
                m_kernelCtx.getLogManager() << Kernel::LogLevel_Error << "The box identifier [" << boxID <<
                        "] used in Window manager was not found in the scenario.\n";
                return false;
            }
            name = box->getName();
        }
        else {
      name = jsonWidget["name"].ToString().c_str();
    }

        CIdentifier id, parentID;
        parentID.fromString(jsonWidget["parentIdentifier"].ToString());

        size_t index = 0;
        if (this->getWidget(parentID)) {
      index = size_t(jsonWidget["index"].ToInt());
    }
        const size_t nChild = size_t(jsonWidget["childCount"].ToInt());

        this->addWidget(id, name, widgetType, parentID, index, boxID, nChild, widgetID);

        if (widgetID != id) {
            m_kernelCtx.getLogManager() << Kernel::LogLevel_Error << "Visualization widget [" << widgetID << "] for box [" << boxID <<
                    "] could not be imported.\n";
            return false;
        }

        VisualizationToolkit::IVisualizationWidget* widget = this->getWidget(widgetID);

        if (widget) {
            if (widget->getType() == VisualizationToolkit::EVisualizationWidget::Window) {
                widget->setWidth(size_t(jsonWidget["width"].ToInt()));
                widget->setHeight(size_t(jsonWidget["height"].ToInt()));
            }
            if (widget->getType() == VisualizationToolkit::EVisualizationWidget::HorizontalSplit || widget->getType() == VisualizationToolkit::EVisualizationWidget::VerticalSplit) {
                widget->setDividerPosition(jsonWidget["dividerPosition"].ToInt());
                widget->setMaxDividerPosition(jsonWidget["maxDividerPosition"].ToInt());
            }
        }
    }

    return true;
}


} // namespace Designer
} // namespace OpenViBE