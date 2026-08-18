#pragma once

#include "../ovIKernelObject.h"

namespace OpenViBE {
namespace Kernel {
class IPlayer;

/**
 * \class IPlayerManager
 * \author Yann Renard (INRIA/IRISA)
 * \date 2006-07-21
 * \brief The manager responsible for scenario playback
 * \ingroup Group_Player
 * \ingroup Group_Kernel
 *
 * The player manager is responsible for creating the different player
 * instances. Each player is responsible for the playback of a specific
 * scenario.
 */
class OV_API IPlayerManager : public IKernelObject
{
public:

    /**
     * \brief Creates a new player
     * \param id [out] : the identifier of the created player
     * \return \e true in case of success.
     * \return \e false in case of error.
     */
    virtual bool createPlayer(CIdentifier& id) = 0;
    /**
     * \brief Releases a specific player
     * \param id [in] : the identifier of the player to release
     * \return \e true in case of success.
     * \return \e false in case of error.
     *
     * Releasing a player means the player instance won't be used any more,
     * thus the player manager can release any allocated resource / memory
     * for this player.
     */
    virtual bool releasePlayer(const CIdentifier& id) = 0;
    /**
     * \brief Gets a specific player instance, provided its identifier
     * \param id [in] : the identifier of the player instance to return
     * \return A reference on the player instance
     * \warning Using a non player identifier will cause a crash
     */
    virtual IPlayer& getPlayer(const CIdentifier& id) = 0;
    /**
     * \brief Gets next player identifier
     * \param previousID [in] : The identifier
     *        for the preceeding player
     * \return The identifier of the next player in case of success.
     * \return \c CIdentifier::undefined() on error.
     * \note Giving \c CIdentifier::undefined() as \c previousID
     *       will cause this function to return the first player
     *       identifier.
     */
    virtual CIdentifier getNextPlayerIdentifier(const CIdentifier& previousID) const = 0;

    _IsDerivedFromClass_(IKernelObject, OV_ClassId_Kernel_Player_PlayerManager)
};
}  // namespace Kernel
}  // namespace OpenViBE
