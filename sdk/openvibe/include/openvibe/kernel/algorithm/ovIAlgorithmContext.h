#pragma once

#include "../ovIKernelObject.h"

namespace OpenViBE {
namespace Kernel {
class IParameter;

/**
 * \class IAlgorithmContext
 * \author Yann Renard (INRIA/IRISA)
 * \date 2007-11-06
 * \brief Exectution context for algorithm objects
 * \ingroup Group_Algorithm
 * \ingroup Group_Kernel
 * \ingroup Group_Extend
 * \sa Plugins::IAlgorithm
 */
class OV_API IAlgorithmContext : public IKernelObject
{
public:

    /**
     * \brief Gets a reference on the current configuration manager
     * \return a reference on the configuration manager
     *
     * \warning The algorithm should not use this reference after it
     *          has finished its work, it could be deprecated.
     */
    virtual IConfigurationManager& getConfigurationManager() const = 0;
    /**
     * \brief Gets a reference on the current algorithm manager
     * \return a reference on the algorithm manager
     *
     * \warning The algorithm should not use this reference after it
     *          has finished its work, it could be deprecated.
     */
    virtual IAlgorithmManager& getAlgorithmManager() const = 0;
    /**
     * \brief Gets a reference on the current log manager
     * \return a reference on the current log manager
     *
     * \warning The algorithm should not use this reference after it
     *          has finished its work, it could be deprecated.
     */
    virtual ILogManager& getLogManager() const = 0;
    /**
     * \brief Gets a reference on the current error manager
     * \return a reference on the current error manager
     *
     * \warning The algorithm should not use this reference after it
     *          has finished its work, it could be deprecated.
     */
    virtual CErrorManager& getErrorManager() const = 0;
    /**
     * \brief Gets a reference on the current type manager
     * \return a reference on the current type manager
     */
    virtual ITypeManager& getTypeManager() const = 0;

    /** \name Input parameters */
    //@{

    /**
     * \brief Gets next input parameter identifier given the previous input parameter
     * \param prevInputParameterID [in] : the previous input parameter
     * \return the next input parameter identifier if existing.
     * \return \c CIdentifier::undefined() if the previous was the last input parameter identifier.
     * \note Passing \c CIdentifier::undefined() as \c prevInputParameterID will
     *       cause this function to return the firs input parameter identifier.
     */
    virtual CIdentifier getNextInputParameterIdentifier(const CIdentifier& prevInputParameterID) const = 0;
    /**
     * \brief Gets the parameter details of a specific input parameter
     * \param id [in] : the identifier of the parameter which details should be returned
     * \return the parameter pointer that corresponds to the provided identifier.
     * \return \c NULL in case of error.
     */
    virtual IParameter* getInputParameter(const CIdentifier& id) = 0;

    //@}
    /** \name Output parameters */
    //@{

    /**
     * \brief Gets next output parameter identifier given the previous output parameter
     * \param prevOutputParameterID [in] : the previous output parameter
     * \return the next output parameter identifier if existing.
     * \return \c CIdentifier::undefined() if the previous was the last output parameter identifier.
     * \note Passing \c CIdentifier::undefined() as \c prevOutputParameterID will
     *       cause this function to return the firs output parameter identifier.
     */
    virtual CIdentifier getNextOutputParameterIdentifier(const CIdentifier& prevOutputParameterID) const = 0;
    /**
     * \brief Gets the parameter details of a specific output parameter
     * \param id [in] : the identifier of the parameter which details should be returned
     * \return the parameter pointer that corresponds to the provided identifier.
     * \return \c NULL in case of error.
     */
    virtual IParameter* getOutputParameter(const CIdentifier& id) = 0;

    //@}
    /** \name Trigger management */
    //@{

    /**
     * \brief Tests whether an input trigger is activated or not.
     * \param id [in] : the identifier of the input trigger which activation status should be returned
     * \return \e true if the provided trigger is currently active.
     * \return \e false if the provided trigger is not currently active or does not exist.
     */
    virtual bool isInputTriggerActive(const CIdentifier& id) const = 0;
    /**
     * \brief Activates an output trigger before calling the processing function.
     * \param id [in] : the identifier of the output to activate
     * \param state [in]: the new trigger state this output trigger should take
     *                            (\e true for active, \e false for inactive)
     * \return \e true in case the state was correctly changed.
     * \return \e false in case the state was not changed or the provided trigger identifier does not exist.
     */
    virtual bool activateOutputTrigger(const CIdentifier& id, const bool state) = 0;

    //@}

    _IsDerivedFromClass_(IKernelObject, OV_ClassId_Kernel_Algorithm_AlgorithmContext)
};
}  // namespace Kernel
}  // namespace OpenViBE
