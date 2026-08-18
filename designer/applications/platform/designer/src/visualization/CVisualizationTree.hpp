// SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

#pragma once

#include <visualization-toolkit/ovvizIVisualizationTree.h>

#include <map>

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
    
    bool getNextWidgetIdentifier(CIdentifier& id) const override;
    bool getNextWidgetIdentifier(CIdentifier& id, VisualizationToolkit::EVisualizationWidget type) const override;
    VisualizationToolkit::IVisualizationWidget* getWidget(const CIdentifier& id) const override;
    VisualizationToolkit::IVisualizationWidget* getWidgetFromBoxIdentifier(const CIdentifier& boxID) const override;
    bool addWidget(CIdentifier& id, const CString& name, VisualizationToolkit::EVisualizationWidget type, const CIdentifier& parentID,
                                size_t parentIdx, const CIdentifier& boxID, size_t nChild, const CIdentifier& suggestedID) override;
    bool getWidgetIndex(const CIdentifier& id, size_t& index) const override;
    bool unparentWidget(const CIdentifier& id, size_t& index) override;
    bool parentWidget(const CIdentifier& id, const CIdentifier& parentID, const size_t index) override;
    bool destroyHierarchy(const CIdentifier& id, bool destroyVisualizationBoxes) override;
    
    bool insertWidget(const CIdentifier& id, const CIdentifier& parentID, VisualizationToolkit::EDragLocation location);
    bool cleanupWidget(const CIdentifier& id);
    
    CString serialize() const override;
    bool deserialize(const CString& tree) override;
    
    
  protected:
    const Kernel::IKernelContext& m_kernelCtx;
    const Kernel::IScenario* m_scenario;
    std::map<CIdentifier, std::unique_ptr<VisualizationToolkit::IVisualizationWidget>> m_widgets;


    /**
     * \brief Helper function to retrieve the next identifier in the widgets map
     * \param id [out] identifier to fill
     * \param test function to test on the map iterator (default: always true, returning the first identifier)
     * \return true if an identifier was found, false otherwise
     */
    bool getNextID(CIdentifier& id,
                   const std::function<bool(typename std::map<CIdentifier, std::unique_ptr<VisualizationToolkit::IVisualizationWidget>>::const_iterator)>& test = [](const auto&) { return true; }) const;

    /**
     * \brief Helper function to get an identifier that is not already used in the widgets map
     * \param suggestedID identifier to use as a suggestion
     * \return suggestedID if it is not already used, otherwise a new identifier
     */
      CIdentifier getUnusedIdentifier(const CIdentifier& suggestedID) const;

    /**
     * \brief Helper function to serialize a widget
     * \param widget widget to be serialized
     * \return json representation of widget
     */
      json::Object serializeWidget(VisualizationToolkit::IVisualizationWidget& widget) const;
};

} // namespace Designer
} // namespace OpenViBE