#include "ovkCPlayerManager.h"
#include "ovkCPlayer.h"

#include <system/ovCMath.h>

namespace OpenViBE {
namespace Kernel {

bool CPlayerManager::createPlayer(CIdentifier& playerID)
{
    playerID            = getUnusedIdentifier();
    m_players[playerID] = new CPlayer(getKernelContext());
    return true;
}

bool CPlayerManager::releasePlayer(const CIdentifier& playerID)
{
    auto it = m_players.find(playerID);

    OV_ERROR_UNLESS_KRF(it != m_players.end(), "Player release failed, identifier :" << playerID.str(), Kernel::ErrorType::ResourceNotFound);

    delete it->second;
    m_players.erase(it);
    return true;
}

IPlayer& CPlayerManager::getPlayer(const CIdentifier& playerID)
{
    const auto it = m_players.find(playerID);

    // use fatal here because the signature does not allow
    // proper checking
    OV_FATAL_UNLESS_K(it != m_players.end(), "Trying to retrieve non existing player with id " << playerID.str(), Kernel::ErrorType::ResourceNotFound);

    // use a fatal here because failing to meet this invariant
    // means there is a bug in the manager implementation
    OV_FATAL_UNLESS_K(it->second, "Null player found for id " << playerID.str(), Kernel::ErrorType::BadValue);

    return *it->second;
}

CIdentifier CPlayerManager::getUnusedIdentifier() const
{
    uint64_t id = CIdentifier::random().id();
    CIdentifier res;
    std::map<CIdentifier, CPlayer*>::const_iterator i;
    do
    {
        id++;
        res = CIdentifier(id);
        i   = m_players.find(res);
    } while (i != m_players.end() || res == CIdentifier::undefined());
    return res;
}

}  // namespace Kernel
}  // namespace OpenViBE
