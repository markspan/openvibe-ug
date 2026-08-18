///-------------------------------------------------------------------------------------------------
/// 
/// \file CVisualizationManager.cpp
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

#include "CVisualizationTree.hpp"
#include "CVisualizationManager.hpp"

#if defined TARGET_OS_Windows
#include <gdk/gdkwin32.h>
#elif defined TARGET_OS_Linux
#include <gdk/gdkx.h>
#elif defined TARGET_OS_MacOS
#define Cursor XCursor
//#include <gdk/gdkx.h>
#undef Cursor
#else
#endif


namespace OpenViBE {
namespace Designer {

using IVTTree = VisualizationToolkit::IVisualizationTree;

bool CVisualizationManager::createVisualizationTree(CIdentifier& treeID)
{
    IVTTree* newTree = new CVisualizationTree(m_kernelCtx);
    treeID           = getUnusedIdentifier();
    m_trees[treeID]  = newTree;
    return true;
}

bool CVisualizationManager::releaseVisualizationTree(const CIdentifier& treeID)
{
    auto it = m_trees.find(treeID);
    if (it != m_trees.end()) {
        delete it->second;
        m_trees.erase(it);
        return true;
    }

    return false;
}

IVTTree& CVisualizationManager::getVisualizationTree(const CIdentifier& id)
{
    const auto it = m_trees.find(id);
    if (it == m_trees.end()) { m_kernelCtx.getLogManager() << Kernel::LogLevel_Fatal << "Visualization Tree " << id << " does not exist !\n"; }
    return *it->second;
}

bool CVisualizationManager::setToolbar(const CIdentifier& treeID, const CIdentifier& boxID, GtkWidget* toolbar)
{
    IVTTree& tree = getVisualizationTree(treeID);
    tree.setToolbar(boxID, toolbar);
    return true;
}

bool CVisualizationManager::setWidget(const CIdentifier& treeID, const CIdentifier& boxID, GtkWidget* topmostWidget)
{
    IVTTree& tree = getVisualizationTree(treeID);
    tree.setWidget(boxID, topmostWidget);
    return true;
}

CIdentifier CVisualizationManager::getUnusedIdentifier() const
{
    uint64_t id = CIdentifier::random().id();
    CIdentifier res;
    std::map<CIdentifier, IVTTree*>::const_iterator it;
    do {
        res = CIdentifier(id++);
        it  = m_trees.find(res);
    } while (it != m_trees.end() || res == CIdentifier::undefined());
    return res;
}

}  // namespace Designer
}  // namespace OpenViBE
