#include "ovvizCVisualizationContext.hpp"

namespace OpenViBE {
namespace VisualizationToolkit {

CIdentifier CVisualizationContext::addWindowManager(std::unique_ptr<IWindowManager> windowManager)
{
    CIdentifier newID = CIdentifier::random();
    while (m_windowManagers.find(newID) != m_windowManagers.end() && newID != CIdentifier(0)) {
        ++newID;
    }
    m_windowManagers[newID] = std::move(windowManager);
    return newID;
}

bool CVisualizationContext::removeWindowManager(const CIdentifier& windowManagerID)
{
    auto it = m_windowManagers.find(windowManagerID);
    if (it != m_windowManagers.end()) {
        m_windowManagers.erase(it); // unique_ptr will be deleted upon removal
        return true;
    }
    return false;
}

IWindowManager* CVisualizationContext::getWindowManager(const CIdentifier& windowManagerID)
{
    auto it = m_windowManagers.find(windowManagerID);
    if (it != m_windowManagers.end()) {
        return it->second.get();
    }
    return nullptr;
}

}  // namespace VisualizationToolkit
}  // namespace OpenViBE
