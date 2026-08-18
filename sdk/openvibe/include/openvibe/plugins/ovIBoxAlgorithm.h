#pragma once

#include "ovIPluginObject.h"

namespace OpenViBE {
namespace Kernel {

/**
 * \brief This enum lists all the way a box can be modified
 * \sa Plugins::IBoxListener::process
 */
enum class EBoxModification
{
    Initialized, DefaultInitialized,
    NameChanged, IdentifierChanged, AlgorithmClassIdentifierChanged,
    InputConnected, InputDisconnected, InputAdded, InputRemoved, InputTypeChanged, InputNameChanged,
    OutputConnected, OutputDisconnected, OutputAdded, OutputRemoved, OutputTypeChanged, OutputNameChanged,
    SettingAdded, SettingRemoved, SettingTypeChanged, SettingNameChanged, SettingDefaultValueChanged, SettingValueChanged
};
}  // namespace Kernel


namespace Kernel {
class IBoxAlgorithmContext;
class IBoxListenerContext;
}

namespace Plugins {
/**
 * \class IBoxAlgorithm
 * \author Yann Renard (INRIA/IRISA)
 * \date 2006-06-19
 * \brief Algorithm to create/process/transform OpenViBE data
 * \ingroup Group_Extend
 *
 * This class should be derived by any plugin that is related
 * to data processing. It can be data acquisition/production
 * from an hardware device or from a file. It can be data
 * processing/transforming, moving time information into
 * frequency space for example. It can be data classification
 * generating discrete classification events better than
 * continuous data flow.
 *
 * This is the heart of the extension mechanism of the
 * OpenViBE platform.
 *
 * \sa Kernel::IBoxAlgorithmDesc
 *
 * \todo details about building new plugins
 */
class OV_API IBoxAlgorithm : public IPluginObject
{
public:

    /** \name Behavioral configuration */
    //@{

    /**
     * \brief Gets the clock frequency to call this algorithm
     * \param ctx [in] : The current box state
     * \return The clock frequency to call this algorithm
     * \note Default implementation returns 0
     *
     * This function is used for algorithms that are triggered on
     * clock signals. The frequency is given in Hz, with 32:32 fixed
     * point representation thus returning \c (1<<32) will make the
     * algorithm to be called every second, returning \c (100<<32)
     * will make the algorithm being called 100 times a second,
     * returning \c (1<<31) will make the algorithm be called once
     * every two seconds and so on...
     *
     * \note Returning 0 means the algorithm should not be
     *       clock activated.
     *
     * \sa processClock
     */
    virtual uint64_t getClockFrequency(Kernel::IBoxAlgorithmContext& ctx) { return 0; }

    //@}
    /** \name Initialization / Uninitialization */
    //@{

    /**
     * \brief Prepares plugin object
     * \param ctx [in] : the plugin object context
     * \return \e true when this object successfully initialized
     *         or \e false if it didn't succeed to initialize.
     * \note Default implementation simply returns \e true.
     *
     * After a successful initialization, the caller knows
     * the object can safely be used... On failure, this object
     * should be ready to be uninitialized and then released.
     *
     * \sa uninitialize
     */
    virtual bool initialize(Kernel::IBoxAlgorithmContext& ctx) { return true; }
    /**
     * \brief Unprepares the object so it can be deleted
     * \param ctx [in] : the plugin object context
     * \return \e true when this object sucessfully uninitialized or \e false if didn't succeed to uninitialize.
     * \exception this method must be noexcept
     * \note Default implementation simply returns \e true.
     *
     * If this function returns \e false, it means it could not
     * uninitialize the object members correctly. Thus, the
     * object won't be released and none of its method will
     * be called anymore for security. Note that this results
     * in memory leaks. This is why this method should return
     * \e true as often as possible.
     *
     * \sa initialize
     */
    virtual bool uninitialize(Kernel::IBoxAlgorithmContext& ctx) { return true; }

    //@}
    /** \name Several event processing callbacks */
    //@{

    /**
     * \brief Reaction to a clock tick
     * \param ctx [in] : the box algorithm context to use
     * \param msg [in] : the clock message the box received
     * \return \e true when the message is processed.
     * \return \e false when the message is not processed.
     * \note Default implementation returns \e false
     *
     * This function is called by the OpenViBE kernel when
     * it has sent clock messages. Clock messages are used for
     * processes that should be executed regularly and which
     * can not be triggered thanks to their inputs (for example
     * acquisition modules). They also can be used for example
     * when viewing inputs on smaller range than what input
     * sends periodically, thus needing a moving
     * 'viewed-window' on lastly received data.
     *
     * \sa Kernel::IBoxAlgorithmContext
     * \sa getClockFrequency
     */
    virtual bool processClock(Kernel::IBoxAlgorithmContext& ctx, Kernel::CMessageClock& msg) { return false; }
    /**
     * \brief Reaction to an input update
     * \param ctx [in] : the box algorithm context to use
     * \param index [in] : the index of the input which has ben updated
     * \return \e true when the message is processed.
     * \return \e false when the message is not processed.
     * \note Default implementation returns \e false
     *
     * This function is called by the OpenViBE kernel each
     * time an input of this box is updated. This allows the
     * algorithm to decide to call the process function and
     * eventually to the received data.
     *
     * \sa Kernel::IBoxAlgorithmContext
     */
    virtual bool processInput(Kernel::IBoxAlgorithmContext& ctx, const size_t index) { return false; }

    //@}
    /** \name Algorithm processing */
    //@{

    /**
     * \brief Processing function
     * \param ctx [in] : the box algorithm context to use
     * \return \e true on success, \e false when something went wrong.
     *
     * This function is used to process the arrived data and
     * eventually generate results. See OpenViBE global
     * architecture to understand how the commponents interact
     * and how an OpenViBE box works internally.
     *
     * The processing function may use the provided context
     * in order to read its inputs and write its outputs...
     * Also it could use the provided context to send
     * messages/events to other boxes. Finally, it may use
     * the provided context in order to perform rendering
     * tasks !
     *
     * \sa Kernel::IBoxAlgorithmContext
     */
    virtual bool process(Kernel::IBoxAlgorithmContext& ctx) = 0;

    //@}

    _IsDerivedFromClass_(IPluginObject, OV_ClassId_Plugins_BoxAlgorithm)
};

class OV_API IBoxListener : public IPluginObject
{
public:
    ~IBoxListener() override { }
    void release() override { }

    virtual bool initialize(Kernel::IBoxListenerContext& /*boxListenerCtx*/) { return true; }
    virtual bool uninitialize(Kernel::IBoxListenerContext& /*boxListenerCtx*/) { return true; }

    /** \name Box modifications callbacks */
    //@{

    /**
     * \brief This callback is called when the box is modified in some way
     * \param 1: input modified, 2: output modified, 3: setting modified
     * \param index: the index of the modified element
     */
    virtual void setCallback(const std::function<void(Kernel::EBoxInterfacorType, int)>& callback) = 0;

    /**
     * \brief This callback is called when the box is modified in some way
     * \return \e true in case of success.
     * \return \e false in case of error.
     * \param boxListenerCtx [in] : the box listener context
     *        containing the description of the box
     * \param boxModificationType [in] : the type of modification
     *        applied to the box
     *
     * This function is called as soon as a modification is done
     * on the box which this listener is attached to. The box listener
     * is then allowed to examine and check box status validity
     * and to adpat the box itself according to this validity.
     *
     * \sa IBoxProto
     * \sa IBoxListenerContext
     * \sa EBoxModification
     */
    virtual bool process(Kernel::IBoxListenerContext& boxListenerCtx, const Kernel::EBoxModification boxModificationType) = 0;

    //@}

    _IsDerivedFromClass_(IPluginObject, OV_ClassId_Plugins_BoxListener)
};
}  // namespace Plugins
}  // namespace OpenViBE
