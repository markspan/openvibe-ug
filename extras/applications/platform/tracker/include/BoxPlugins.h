#pragma once

#include <openvibe/ov_all.h>

#include "BoxAdapter.h"
#include "Contexted.h"

namespace OpenViBE {
namespace Tracker {
/**
 * \class BoxPlugins 
 * \brief Box Plugins is a factory-like class that keeps a list of box plugins that can be 'applied' to streams in Tracker
 * \details Boxes can be registered in the constructor
 * \author J. T. Lindgren
 *
 */
class BoxPlugins final : protected Contexted
{
public:
    explicit BoxPlugins(const Kernel::IKernelContext& ctx);
    ~BoxPlugins() override { for (auto ptr : m_boxPlugins) { delete ptr; } }

    // Get a list of available box plugins
    const std::vector<BoxAdapterStream*>& getBoxPlugins() const { return m_boxPlugins; }

protected:
    bool create(const CIdentifier& streamType, const CIdentifier& alg, const CIdentifier& desc);

    std::vector<BoxAdapterStream*> m_boxPlugins;
};
}  // namespace Tracker
}  // namespace OpenViBE
