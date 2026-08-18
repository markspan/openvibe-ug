#pragma once

#include "../ovkTKernelObject.h"

#include <map>

namespace OpenViBE {
namespace Kernel {
class CPlayer;

class CPlayerManager final : public TKernelObject<IPlayerManager>
{
public:

    explicit CPlayerManager(const IKernelContext& ctx) : TKernelObject<IPlayerManager>(ctx) {}
    bool createPlayer(CIdentifier& playerID) override;
    bool releasePlayer(const CIdentifier& playerID) override;
    IPlayer& getPlayer(const CIdentifier& playerID) override;
    CIdentifier getNextPlayerIdentifier(const CIdentifier& previousID) const override { return getNextIdentifier<CPlayer*>(m_players, previousID); }

    _IsDerivedFromClass_Final_(TKernelObject<IPlayerManager>, OVK_ClassId_Kernel_Player_PlayerManager)

protected:

    CIdentifier getUnusedIdentifier() const;

    std::map<CIdentifier, CPlayer*> m_players;
};
}  // namespace Kernel
}  // namespace OpenViBE
