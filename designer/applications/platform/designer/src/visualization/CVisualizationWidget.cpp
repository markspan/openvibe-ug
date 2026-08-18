///-------------------------------------------------------------------------------------------------
/// 
/// \file CVisualizationWidget.cpp
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

#include <vector>

#include "CVisualizationWidget.hpp"

namespace OpenViBE {
namespace Designer {

bool CVisualizationWidget::initialize(const CIdentifier& id, const CString& name, const VisualizationToolkit::EVisualizationWidget type,
                                      const CIdentifier& parentID, const CIdentifier& boxID, const size_t nChild)
{
    m_id       = id;
    m_name     = name;
    m_type     = type;
    m_parentID = parentID;
    m_boxID    = boxID;
    m_childrens.resize(nChild, CIdentifier::undefined());
    return true;
}

bool CVisualizationWidget::getChildIndex(const CIdentifier& id, size_t& index) const
{
    for (index = 0; index < m_childrens.size(); ++index) { if (m_childrens[index] == id) { return true; } }
    return false;
}

bool CVisualizationWidget::addChild(const CIdentifier& childID)
{
    m_childrens.push_back(childID);
    return true;
}

bool CVisualizationWidget::removeChild(const CIdentifier& id)
{
    for (size_t i = 0; i < m_childrens.size(); ++i) {
        if (m_childrens[i] == id) {
            //remove tab from a window (variable number of children)
            if (m_type == VisualizationToolkit::EVisualizationWidget::Window) { 
                m_childrens.erase(m_childrens.begin() + i);
            }
            else //clear identifier if ith child for a regular widget (fixed number of children)
            {
                m_childrens[i] = CIdentifier::undefined();
            }
            return true;
        }
    }

    std::cout << "Error: Child with id " << id << " not found" << std::endl;
    return false;
}

bool CVisualizationWidget::getChildIdentifier(const size_t index, CIdentifier& id) const
{
    if (index >= m_childrens.size()) {
        id = CIdentifier::undefined();
        std::cout << "Error: Child with index " << index << " not found" << std::endl;
        return false;
    }
    id = m_childrens[index];
    return true;
}

bool CVisualizationWidget::setChildIdentifier(const size_t index, const CIdentifier& id)
{
    if (index >= m_childrens.size()) {
        if (m_type == VisualizationToolkit::EVisualizationWidget::Window && m_childrens.size() == 0 && index == 0) {
            // window should have only one child. So this is the only case where it's ok to add a child.
            m_childrens.push_back(id);
            return true;
        } else {
            std::cout << "Error: Child with index " << index << " not found" << std::endl;
            return false;
        }
    }
    m_childrens[index] = id;
    return true;
}

}  // namespace Designer
}  // namespace OpenViBE
