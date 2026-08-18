#pragma once

#include "../ovIKernelObject.h"

namespace OpenViBE {
namespace Plugins {
class IPluginObject;
}  // namespace Plugins

namespace Kernel {
class IConfigurationManager;
class ILogManager;
class IScenarioManager;

/**
 * \class IPlayerContext
 * \author Yann Renard (INRIA/IRISA)
 * \date 2006-07-07
 * \brief Player interface for plugin objects
 * \ingroup Group_Player
 * \ingroup Group_Kernel
 *
 * Instances of this class are given to plugin object
 * so they can communicate with the platform kernel,
 * providing services such as message sending etc...
 */
class OV_API IPlayerContext : public IKernelObject
{
public:

    /** \name Time management */
    //@{

    /**
     * \brief Gets the current player time
     * \return the current player time.
     * \note The time value is fixed point 32:32 representated in seconds
     */
    virtual uint64_t getCurrentTime() const = 0;
    /**
     * \brief Gets the current player lateness
     * \return the current player lateness
     * \note The lateness is fixed point 32:32 representated in seconds
     */
    virtual uint64_t getCurrentLateness() const = 0;
    /**
     * \brief Gets the current CPU use for the running processing unit
     * \return the current CPU use
     */
    virtual double getCurrentCPUUsage() const = 0;
    /**
     * \brief Gets the current fast forward factor to be used when the getStatus states the player runs in fast forward mode
     * \return the current fast forward factor
     */
    virtual double getCurrentFastForwardMaximumFactor() const = 0;

    //@}
    /** \name Player control */
    //@{

    /**
     * \brief Stops player execution
     * \return \e true in case of success.
     * \return \e false in case of error.
     * \warning Once the player is stopped, there's no way to restart it
     */
    virtual bool stop() = 0;
    /**
     * \brief Pauses player execution
     * \return \e true in case of success.
     * \return \e false in case of error.
     */
    virtual bool pause() = 0;
    /**
     * \brief Makes player run normal speed
     * \return \e true in case of success.
     * \return \e false in case of error.
     */
    virtual bool play() = 0;
    /**
     * \brief Makes player run as fast as possible
     * \return \e true in case of success.
     * \return \e false in case of error.
     */
    virtual bool forward() = 0;
    /**
     * \brief Gets current player status
     * \return current player status
     */
    virtual EPlayerStatus getStatus() const = 0;

    //@}
    /** \name Give access to some managers */
    //@{

    /**
     * \brief Gets the current player's configuration manager
     * \return The current player's configuration manager
     *
     * \warning The plugin object should not use this reference after it
     *          has finished its work, it could be deprecated.
     */
    virtual IConfigurationManager& getConfigurationManager() const = 0;
    /**
     * \brief Gets the current player's algorithm manager
     * \return The current player's algorithm manager
     *
     * \warning The plugin object should not use this reference after it
     *          has finished its work, it could be deprecated.
     */
    virtual IAlgorithmManager& getAlgorithmManager() const = 0;
    /**
     * \brief Gets the current player's log manager
     * \return The current player's log manager
     *
     * \warning The plugin object should not use this reference after it
     *          has finished its work, it could be deprecated.
     */
    virtual ILogManager& getLogManager() const = 0;
    /**
     * \brief Gets the current player's error manager
     * \return The current player's error manager
     *
     * \warning The plugin object should not use this reference after it
     *          has finished its work, it could be deprecated.
     */
    virtual CErrorManager& getErrorManager() const = 0;

    /**
     * \brief Gets the current player's scenario manager
     * \return The current player's scenario manager
     *
     * \warning The plugin object should not use this reference after it
     *          has finished its work, it could be deprecated.
     */
    virtual IScenarioManager& getScenarioManager() const = 0;
    /**
     * \brief Gets the current player's type manager
     * \return The current player's type manager
     *
     * \warning The plugin object should not use this reference after it
     *          has finished its work, it could be deprecated.
     */
    virtual ITypeManager& getTypeManager() const = 0;

    virtual bool canCreatePluginObject(const CIdentifier& pluginID) const = 0;
    virtual Plugins::IPluginObject* createPluginObject(const CIdentifier& pluginID) const = 0;
    virtual bool releasePluginObject(Plugins::IPluginObject* pluginObject) const = 0;

    //@}

    _IsDerivedFromClass_(IKernelObject, OV_ClassId_Kernel_Player_PlayerContext)
};
}  // namespace Kernel
}  // namespace OpenViBE
