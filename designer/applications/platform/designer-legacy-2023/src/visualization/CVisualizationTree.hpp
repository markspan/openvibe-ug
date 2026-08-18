///-------------------------------------------------------------------------------------------------
/// 
/// \file CVisualizationTree.hpp
/// \copyright Copyright (C) 2022 Inria
///
/// This program is free software: you can redistribute it and/or modify
/// it under the terms of the GNU Affero General Public License as published
/// by the Free Software Foundation, either version 3 of the License, or
/// (at your option) any later version.
///
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU Affero General Public License for more details.
///
/// You should have received a copy of the GNU Affero General Public License
/// along with this program.  If not, see <https://www.gnu.org/licenses/>.
/// 
///-------------------------------------------------------------------------------------------------

#pragma once

#include <visualization-toolkit/ovvizIVisualizationTree.h>

#include <map>
#include <gtk/gtk.h>

namespace json {
class Object;
}

namespace OpenViBE {
namespace Designer {
class CVisualizationTree final : public VisualizationToolkit::IVisualizationTree
{
public:
    CVisualizationTree(const Kernel::IKernelContext& ctx) : m_kernelCtx(ctx) {}
    ~CVisualizationTree() override;

    bool init(const Kernel::IScenario* scenario) override;

    bool getNextVisualizationWidgetIdentifier(CIdentifier& id) const override;
    bool getNextVisualizationWidgetIdentifier(CIdentifier& id, VisualizationToolkit::EVisualizationWidget type) const override;
    bool isVisualizationWidget(const CIdentifier& id) const override;
    VisualizationToolkit::IVisualizationWidget* getVisualizationWidget(const CIdentifier& id) const override;
    VisualizationToolkit::IVisualizationWidget* getVisualizationWidgetFromBoxIdentifier(const CIdentifier& boxID) const override;
    bool addVisualizationWidget(CIdentifier& id, const CString& name, VisualizationToolkit::EVisualizationWidget type, const CIdentifier& parentID,
                                size_t parentIdx, const CIdentifier& boxID, size_t nChild, const CIdentifier& suggestedID) override;
    bool getVisualizationWidgetIndex(const CIdentifier& id, size_t& index) const override;
    bool unparentVisualizationWidget(const CIdentifier& id, size_t& index) override;
    bool parentVisualizationWidget(const CIdentifier& id, const CIdentifier& parentID, const size_t index) override;
    bool destroyHierarchy(const CIdentifier& id, bool destroyVisualizationBoxes) override;

    GtkTreeView* createTreeViewWithModel() override;
    bool setTreeViewCB(VisualizationToolkit::ITreeViewCB* callback) override;

    bool reloadTree() override;

    bool getTreeSelection(GtkTreeView* preeView, GtkTreeIter* iter) override;
    GtkTreePath* getTreePath(GtkTreeIter* iter) const override;
    size_t getULongValueFromTreeIter(GtkTreeIter* iter, VisualizationToolkit::EVisualizationTreeColumn colType) const override;
    bool getStringValueFromTreeIter(GtkTreeIter* iter, char*& string, VisualizationToolkit::EVisualizationTreeColumn colType) const override;
    bool getPointerValueFromTreeIter(GtkTreeIter* iter, void*& pointer, VisualizationToolkit::EVisualizationTreeColumn colType) const override;
    bool getIdentifierFromTreeIter(GtkTreeIter* iter, CIdentifier& id, VisualizationToolkit::EVisualizationTreeColumn colType) const override;

    bool findChildNodeFromRoot(GtkTreeIter* iter, const char* label, VisualizationToolkit::EVisualizationTreeNode type) override;
    bool findChildNodeFromParent(GtkTreeIter* iter, const char* label, VisualizationToolkit::EVisualizationTreeNode type) override;
    bool findChildNodeFromRoot(GtkTreeIter* iter, void* widget) override;
    bool findChildNodeFromParent(GtkTreeIter* iter, void* widget) override;
    bool findChildNodeFromRoot(GtkTreeIter* iter, CIdentifier id) override;
    bool findChildNodeFromParent(GtkTreeIter* iter, CIdentifier id) override;
    bool findParentNode(GtkTreeIter* iter, VisualizationToolkit::EVisualizationTreeNode type) override;

    bool dragDataReceivedInWidgetCB(const CIdentifier& srcWidgetID, GtkWidget* dstWidget) override;
    bool dragDataReceivedOutsideWidgetCB(const CIdentifier& srcWidgetID, GtkWidget* dstWidget, VisualizationToolkit::EDragLocation location) override;

    bool setToolbar(const CIdentifier& boxID, GtkWidget* toolbar) override;
    bool setWidget(const CIdentifier& boxID, GtkWidget* widget) override;

    CString serialize() const override;
    bool deserialize(const CString& tree) override;

private:
    json::Object serializeWidget(VisualizationToolkit::IVisualizationWidget& widget) const;

    bool destroyHierarchyR(const CIdentifier& id, bool destroy);
    CIdentifier getUnusedIdentifier(const CIdentifier& suggestedID) const;

    bool findChildNodeFromParentR(GtkTreeIter* iter, const char* label, VisualizationToolkit::EVisualizationTreeNode type);
    bool findChildNodeFromParentR(GtkTreeIter* iter, void* widget);
    bool findChildNodeFromParentR(GtkTreeIter* iter, const CIdentifier& id);

    bool loadVisualizationWidget(VisualizationToolkit::IVisualizationWidget* widget, GtkTreeIter* parentIter);

    std::map<CIdentifier, VisualizationToolkit::IVisualizationWidget*> m_widgets;
    CIdentifier m_scenarioID = CIdentifier::undefined();
    const Kernel::IKernelContext& m_kernelCtx;
    const Kernel::IScenario* m_scenario = nullptr;
    GtkTreeStore* m_treeStore           = nullptr;
    GtkTreeIter m_internalTreeNode;
    VisualizationToolkit::ITreeViewCB* m_treeViewCB = nullptr;
};
}  // namespace Designer
}  // namespace OpenViBE
