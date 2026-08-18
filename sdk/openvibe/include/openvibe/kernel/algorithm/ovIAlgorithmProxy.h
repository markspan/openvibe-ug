#pragma once

#include "../ovIKernelObject.h"

namespace OpenViBE {
namespace Kernel {
class IParameter;

/**
 * \class IAlgorithmProxy
 * \brief Application interface to an algorithm plugin
 * \author Yann Renard (INRIA/IRISA)
 * \date 2007-11-21
 * \ingroup Group_Algorithm
 * \ingroup Group_Kernel
 * \sa Plugins::IAlgorithm
 *
 * This class is an easier to user interface to an IAlgorihtm instanciated
 * object. Its purpose is to automatically handle input / output trigger
 * activation and to help in calling processing methods.
 */
class OV_API IAlgorithmProxy : public IKernelObject
{
public:

    /** \name Input parameters */
    //@{

    /**
     * \brief Gets next input parameter identifier given the previous input parameter
     * \param previousID [in] : the previous input parameter
     * \return the next input parameter identifier if existing.
     * \return \c CIdentifier::undefined() if the previous was the last input parameter identifier.
     * \note Passing \c CIdentifier::undefined() as \c previousID will
     *       cause this function to return the first input parameter identifier.
     */
    virtual CIdentifier getNextInputParameterIdentifier(const CIdentifier& previousID) const = 0;
    /**
     * \brief Gets the name of a specific input parameter
     * \param id [in] : the identifier of the parameter which name should be returned
     * \return The name of the specified input parameter on success
     * \return En empty string on error
     */
    virtual CString getInputParameterName(const CIdentifier& id) const = 0;
    /**
     * \brief Gets the parameter details of a specific input parameter
     * \param id [in] : the identifier of the parameter which details should be returned
     * \return the parameter pointer that corresponds to the provided id.
     * \return \c NULL in case of error.
     */
    virtual IParameter* getInputParameter(const CIdentifier& id) = 0;

    //@}
    /** \name Output parameters */
    //@{

    /**
     * \brief Gets next output parameter identifier given the previous output parameter
     * \param previousID [in] : the previous output parameter
     * \return the next output parameter identifier if existing.
     * \return \c CIdentifier::undefined() if the previous was the last output parameter identifier.
     * \note Passing \c CIdentifier::undefined() as \c previousID will
     *       cause this function to return the first output parameter identifier.
     */
    virtual CIdentifier getNextOutputParameterIdentifier(const CIdentifier& previousID) const = 0;
    /**
     * \brief Gets the name of a specific input parameter
     * \param id [in] : the identifier of the parameter which name should be returned
     * \return The name of the specified input parameter on success
     * \return En empty string on error
     */
    virtual CString getOutputParameterName(const CIdentifier& id) const = 0;
    /**
     * \brief Gets the parameter details of a specific output parameter
     * \param id [in] : the identifier of the parameter which details should be returned
     * \return the parameter pointer that corresponds to the provided id.
     * \return \c NULL in case of error.
     */
    virtual IParameter* getOutputParameter(const CIdentifier& id) = 0;

    //@}
    /** \name Trigger management */
    //@{

    /**
     * \brief Gets next output trigger identifier given the previous output trigger
     * \param previousID [in] : the previous output trigger
     * \return the next output trigger identifier if existing.
     * \return \c CIdentifier::undefined() if the previous was the last output trigger identifier.
     * \note Passing \c CIdentifier::undefined() as \c previousID will
     *       cause this function to return the first output trigger identifier.
     */
    virtual CIdentifier getNextOutputTriggerIdentifier(const CIdentifier& previousID) const = 0;
    /**
     * \brief Gets the name of a specific output trigger
     * \param id [in] : the identifier of the trigger which name should be returned
     * \return The name of the specified output trigger on success
     * \return En empty string on error
     */
    virtual CString getOutputTriggerName(const CIdentifier& id) const = 0;
    /**
     * \brief Tests whether an output trigger is activated or not.
     * \param id [in] : the identifier of the output trigger which activation status should be returned
     * \return \e true if the provided trigger is currently active.
     * \return \e false if the provided trigger is not currently active or does not exist.
     */
    virtual bool isOutputTriggerActive(const CIdentifier& id) const = 0;

    /**
     * \brief Gets next input trigger identifier given the previous input trigger
     * \param previousID [in] : the previous input trigger
     * \return the next input trigger identifier if existing.
     * \return \c CIdentifier::undefined() if the previous was the last input trigger identifier.
     * \note Passing \c CIdentifier::undefined() as \c previousID will
     *       cause this function to return the first input trigger identifier.
     */
    virtual CIdentifier getNextInputTriggerIdentifier(const CIdentifier& previousID) const = 0;
    /**
     * \brief Gets the name of a specific input trigger
     * \param id [in] : the identifier of the trigger which name should be returned
     * \return The name of the specified input trigger on success
     * \return En empty string on error
     */
    virtual CString getInputTriggerName(const CIdentifier& id) const = 0;
    /**
     * \brief Activates an input trigger before calling the processing function.
     * \param id [in] : the identifier of the input to activate
     * \param state [in]: the new trigger state this input trigger should take (\e true for active, \e false for inactive)
     * \return \e true in case the state was correctly changed.
     * \return \e false in case the state was not changed or the provided trigger id does not exist.
     */
    virtual bool activateInputTrigger(const CIdentifier& id, bool state) = 0;

    //@}
    /** \name Algorithm functions */
    //@{

    /**
     * \brief Initializes the algorithm
     * \return \e true in case of success.
     * \return \e false in case of error.
     * \note The algorithm context is managed internally
     * \pre The algorithm is not yet initialized
     */
    virtual bool initialize() = 0;
    /**
     * \brief Uninitializes the algorithm
     * \return \e true in case of success.
     * \return \e false in case of error.
     * \note The algorithm context is managed internally
     * \pre The algorithm is initialized
     */
    virtual bool uninitialize() = 0;
    /**
     * \brief Calls the processing method of the algorithm
     * \return \e true in case of success.
     * \return \e false in case of error.
     * \note The algorithm context is managed internally
     * \pre The algorithm is initialized
     */
    virtual bool process() = 0;
    /**
     * \brief Activates an input trigger and immediatly calls the processing method
     * \param id [in] : the identifier of the input trigger to activate
     * \return \e true in case of success.
     * \return \e false in case the trigger does not exist or the processing did not succeed.
     * \note The algorithm context is managed internally
     * \pre The algorithm is initialized
     */
    virtual bool process(const CIdentifier& id) = 0;


    virtual bool isAlgorithmDerivedFrom(const CIdentifier& classID) = 0;

    //@}

    _IsDerivedFromClass_(IKernelObject, OV_ClassId_Kernel_Algorithm_AlgorithmProxy)
};
}  // namespace Kernel
}  // namespace OpenViBE
