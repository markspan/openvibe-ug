#pragma once

#include "../ovIKernelObject.h"

namespace OpenViBE {
namespace Kernel {
class IScenario;
class IConfigurationManager;

enum class EPlayerStatus { Stop, Pause, Step, Play, Forward };

enum class EPlayerReturnCodes { Success, Failed, BoxInitializationFailed };

inline std::string toString(const EPlayerStatus status)
{
    switch (status)
    {
        case EPlayerStatus::Stop: return "Stop";
        case EPlayerStatus::Pause: return "Pause";
        case EPlayerStatus::Step: return "Step";
        case EPlayerStatus::Play: return "Play";
        case EPlayerStatus::Forward: return "Forward";
        default: return "Invalid Status";
    }
}

inline std::string toString(const EPlayerReturnCodes code)
{
    switch (code)
    {
        case EPlayerReturnCodes::Success: return "Success";
        case EPlayerReturnCodes::Failed: return "Failed";
        case EPlayerReturnCodes::BoxInitializationFailed: return "Box Initialization Failed";
        default: return "Invalid Code";
    }
}

/**
 * \class IPlayer
 * \author Yann Renard (INRIA/IRISA)
 * \date 2006-09-26
 * \brief The main player class
 * \ingroup Group_Player
 * \ingroup Group_Kernel
 *
 * A player is responsible for the playback of a specific scenario.
 * This player scenario is provided at initialisation stage and should
 * not be changed until the player terminates. The player idea of calling
 * a "play" function forces the use of threads in order to avoid CPU locking.
 * Thus we prefer the idea of having a "short-time" function that is to be
 * called repeatedly until the player terminates. This is the role of the
 * \c IPlayer::loop function, that should be called repeatedly by the outside
 * application.
 *
 * \todo Refactor this base class to propose an interface
 *       more OpenViBE compliant (use IDs, intialize/unitialize
 *       pairs etc...)
 */
class OV_API IPlayer : public IKernelObject
{
public:

    /**
     * \brief Attaches a scenario to this player
     * \param id [in] : the scenario identifier to attach to this player. The scenario itself is found from the scenario manager.
     * \param localConfigTokens [in] : An optional map that contains configuration tokens to add.
     * \return \e true in case of success.
     * \return \e false in case of error.
     */
    virtual bool setScenario(const CIdentifier& id, const CNameValuePairList* localConfigTokens = nullptr) = 0;

    /**
     * \brief returns a reference to the runtime configuration manager
     * \return a reference to the runtime configuration manager
     *
     * When initialized, a player creates a runtime configuration manager
     * which will be used for scenario and runtime related specific configuration.
     * The use of this features includes the loading of a delayed configuration file
     * for instance. The reference should be valid up to the duration of the player itself.
     *
     * \sa IConfigurationManager
     */
    virtual IConfigurationManager& getRuntimeConfigurationManager() const = 0;

    /**
     * \brief returns a reference to the runtime scenario manager
     * \return a reference to the runtime scenario manager
     *
     * When a scenario is loaded, a copy is made and stored in the 
     * runtime scenario manager. This function allows getting the 
     * runtime scenario manager that contains the current executed scenario.
     *
     * \sa IScenarioManager
     */
    virtual IScenarioManager& getRuntimeScenarioManager() const = 0;

    /**
     * \brief returns the cidentifier of the associated runtime scenario
     * \return the cidentifier of the associated runtime scenario
     *
     * When initialized, a player copy the scenario in the runtime configuration manager
     * this method allows getting the identifier of said copied scenario
     *
     * \sa CIdentifier
     */
    virtual CIdentifier getRuntimeScenarioIdentifier() const = 0;

    /**
     * \brief Initializes this player
     * \return \e true in case of success.
     * \return \e false in case of error.
     */
    virtual EPlayerReturnCodes initialize() = 0;
    /**
     * \brief Uninitializes this player
     * \return \e true in case of success.
     * \return \e false in case of error.
     */
    virtual bool uninitialize() = 0;

    /**
     * \brief Stops player execution
     * \return \e true in case of success.
     * \return \e false in case of error.
     */
    virtual bool stop() = 0;
    /**
     * \brief Pauses player execution
     * \return \e true in case of success.
     * \return \e false in case of error.
     */
    virtual bool pause() = 0;
    /**
     * \brief Executes one more step and pauses
     * \return \e true in case of success.
     * \return \e false in case of error.
     */
    virtual bool step() = 0;
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

    /**
     * \brief Sets maximum fast forward factor coefficient
     * \param factor : the maximum speed multiplier to be applied when playing in EPlayerStatus::Forward mode
     * \return \e true in case of success.
     * \return \e false in case of error.
     * \note If a negative value is passed, it is turned back to 0
     * \note It the fast forward factor is 0, it tells the player to go as fast as possible
     */
    virtual bool setFastForwardMaximumFactor(double factor) = 0;
    /**
     * \brief Gets the maximum fast forward factor coefficient
     * \return The maximum fast forward factor coefficient.
     */
    virtual double getFastForwardMaximumFactor() const = 0;

    /**
     * \brief Gets an estimate of the actual time ratio spent in the player's loop
     * \return the amount of time spent in the player's loop (given in percentage)
     */
    virtual double getCPUUsage() const = 0;

    /**
     * \brief "short time" function to be called repeatedly by the outstide application
     * \param elapsedTime [in] : real elapsed time given in seconds 32:32
     * \param maximumTimeToReach [in] : maximum time to reach given in seconds 32:32
     * \return \e true if the execution went successfully.
     * \return \e false in case their was a problem or the execution terminated.
     *
     * The \e maximumTimeToReach parameter guarantees that the player does not
     * run beyond the specified time when ran in fast forward mode. It defaults
     * to \c uint64_t(-1) which represents the largest OpenViBE time.
     */
    virtual bool loop(uint64_t elapsedTime, uint64_t maximumTimeToReach = uint64_t(-1)) = 0;

    virtual uint64_t getCurrentSimulatedTime() const = 0;

    _IsDerivedFromClass_(IKernelObject, OV_ClassId_Kernel_Player_Player)
};
}  // namespace Kernel
}  // namespace OpenViBE
