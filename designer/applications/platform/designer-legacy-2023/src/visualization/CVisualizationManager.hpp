///-------------------------------------------------------------------------------------------------
/// 
/// \file CVisualizationManager.hpp
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

#include <openvibe/ov_all.h>
#include <map>

#include <visualization-toolkit/ovvizIVisualizationManager.h>
#include <visualization-toolkit/ovvizIVisualizationTree.h>

typedef struct _GtkWidget GtkWidget;

namespace OpenViBE {
namespace Designer {
class CVisualizationManager final : public VisualizationToolkit::IVisualizationManager
{
public:
    explicit CVisualizationManager(const Kernel::IKernelContext& ctx) : m_kernelCtx(ctx) {}
    ~CVisualizationManager() override = default;

    bool createVisualizationTree(CIdentifier& treeID) override;
    bool releaseVisualizationTree(const CIdentifier& treeID) override;
    VisualizationToolkit::IVisualizationTree& getVisualizationTree(const CIdentifier& id) override;

    bool setToolbar(const CIdentifier& treeID, const CIdentifier& boxID, GtkWidget* toolbar) override;
    bool setWidget(const CIdentifier& treeID, const CIdentifier& boxID, GtkWidget* topmostWidget) override;

private:
    CIdentifier getUnusedIdentifier() const;

    /// Map of visualization trees (one per scenario, storing visualization widgets arrangement in space)
    std::map<CIdentifier, VisualizationToolkit::IVisualizationTree*> m_trees;
    const Kernel::IKernelContext& m_kernelCtx;
};
}  // namespace Designer
}  // namespace OpenViBE
