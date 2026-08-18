#pragma once

#include "ovIParameter.h"
#include "ovIConfigurable.h"

namespace OpenViBE {
namespace Kernel {
/**
 * \class TParameterHandler
 * \brief Helper class in order to manipulate IParameter and IParameter values natively
 * \author Yann Renard (INRIA/IRISA)
 * \date 2007-11-21
 * \ingroup Group_Kernel
 * \ingroup Group_Helper
 * \sa IParameter
 * \sa EParameterType
 *
 * The purpose for this template is to help user code to manipulate
 * IParameter abstract class and it's associated value. Client code
 * should probably use this template better than IParameter directly.
 * The template argument is the handled data type :
 *  - for \c ParameterType_Integer : \c int64_t
 *  - for \c ParameterType_UInteger : \c uint64_t
 *  - for \c ParameterType_Boolean : \c bool
 *  - for \c ParameterType_Float : \c double
 *  - for \c ParameterType_String : \c CString
 *  - for \c ParameterType_Identifier : \c CIdentifier
 *  - for \c ParameterType_Matrix : \c CMatrix*
 *  - for \c ParameterType_StimulationSet : \c CStimulationSet*
 *  - for \c ParameterType_MemoryBuffer : \c CMemoryBuffer*
 *  - for \c ParameterType_Object : \c IObject*
 *  - for \c ParameterType_Pointer : \c uint8_t*
 *
 * The idea is to connect the handler to the corresponding IParameter object
 * thanks to the \c initialize function. Then the provided interface translates
 * handled object native operation to generic IParameter operations automatically.
 * Moreover, the handler is able to provide the handled object's interface thanks
 * to an automatic cast operator.
 */
template <typename T>
class TParameterHandler
{
public:

    /** \name Construction */
    //@{

    /**
     * \brief Default construction, does nothing
     */
    TParameterHandler() { }

    /**
     * \brief IParameter based construction
     * \param parameter [in] : the parameter to use for the initialization
     *
     * This constructor considers the parameter handler to be
     * connected to the provided parameter. This is equivalent to
     * using the default constructor and call the initialize function
     * with the provided IParameter pointer.
     */
    TParameterHandler(IParameter* parameter) : m_parameter(parameter) { }

    //@}
    /** \name Initialization */
    //@{

    /**
     * \brief Connects this parameter handler to a concrete IParameter pointer
     * \param parameter [in] : the concrete parameter to connect to (it can not be \c NULL )
     * \return \e true in case of success.
     * \return \e false in case of error.
     *
     * Future calls to this handler will be redirected to the
     * concrete parameter object.
     */
    bool initialize(IParameter* parameter)
    {
        if (m_parameter || !parameter) { return false; }

        m_parameter = parameter;
        return true;
    }

    /**
     * \brief Disconnects this parameter handler from its concrete IParameter pointer
     * \return \e true in case of success.
     * \return \e false in case of error.
     *
     * This handler won't be usable until \c initialize is called successfully again.
     */
    bool uninitialize()
    {
        if (!m_parameter) { return false; }
        m_parameter = nullptr;
        return true;
    }

    /**
     * \brief Checks whether this handler is connected to a concrete IParameter or not
     * \return \e true if this handler is connected to a concrete IParameter, \e false if not
     */
    bool exists() const { return m_parameter != nullptr; }

    //@{
    /** \name Transparent operators */
    //@{

    /**
     * \brief Handled object type cast
     * \return The handled object itself
     *
     * This cast operator allows the handler to be used as if it was the handled
     * object itself. For example, an unsigned integer parameter handler is usable
     * in arithmetic operations :
     *
     * \code
     * // parameter will directly be used as if it is an unsigned integer
     * TParameterHandler<uint64_t> parameter ( ... );
     * uint64_t a= ...;
     * uint64_t c=a*parameter+parameter;
     * \endcode
     */
    operator T() const
    {
        if (!m_parameter) { throw; }
        T t;
        m_parameter->getValue(&t);
        return t;
    }
    /**
     * \brief Handled object type member access operator
     * \return The handled object itself for access operation
     *
     * This operator allows the handler to be used as if it was the handled
     * object itself. For example, a matrix parameter handler is usable
     * as if it was a matrix pointer :
     *
     * \code
     * // parameter will directly be used as if it is a matrix pointer
     * TParameterHandler<CMatrix*> parameter ( ... );
     * parameter->setDimensionCount(2);
     * \endcode
     */
    T operator ->() const
    {
        if (!m_parameter) { throw; }
        T t;
        m_parameter->getValue(&t);
        return t;
    }
    /**
     * \brief Affectation operator
     * \return This handler itself
     *
     * This operator allows to affect handled object type values to the
     * handled object itself. For example, an unsigned integer parameter
     * handler can be affected this way :
     *
     * \code
     * // the affectation of parameter will directly go to its handled unsigned integer
     * TParameterHandler<uint64_t> parameter ( ... );
     * parameter = 1;
     * \endcode
     */
    TParameterHandler<T>& operator=(const T& t)
    {
        if (!m_parameter) { throw; }
        m_parameter->setValue(&t);
        return *this;
    }

    //@}
    /** \name Reference target management */
    //@{

    /**
     * \brief Clears all reference targets
     * \return \e true in case of success.
     * \return \e false in case of error.
     */
    bool clearReferenceTarget() const { return m_parameter ? m_parameter->clearReferenceTarget() : false; }
    /**
     * \brief Sets a new reference target
     * \param rParameterHandler [in] : a parameter handler of the same type as this parameter handler
     * \return \e true in case of success.
     * \return \e false in case of error.
     */
    bool setReferenceTarget(TParameterHandler<T>& rParameterHandler)
    {
        return m_parameter && rParameterHandler.m_parameter ? m_parameter->setReferenceTarget(rParameterHandler.m_parameter) : false;
    }
    /**
     * \brief Sets a new reference target
     * \param pParameter [in] : a parameter to use as reference target
     * \return \e true in case of success.
     * \return \e false in case of error.
     */
    bool setReferenceTarget(IParameter* pParameter) const { return m_parameter && pParameter ? m_parameter->setReferenceTarget(pParameter) : false; }
    /**
     * \brief Sets a new reference target
     * \param t [in] : a value to use as reference target
     * \return \e true in case of success.
     * \return \e false in case of error.
     */
    bool setReferenceTarget(T& t) { return m_parameter ? m_parameter->setReferenceTarget(&t) : false; }

    //@}

protected:

    IParameter* m_parameter = nullptr; ///< Handled parameter
};
}  // namespace Kernel
}  // namespace OpenViBE
