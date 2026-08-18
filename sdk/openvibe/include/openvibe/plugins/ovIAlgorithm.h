#pragma once

#include "ovIPluginObject.h"

namespace OpenViBE {
namespace Kernel {
class IAlgorithmContext;
}   // namespace Kernel

namespace Plugins {
/**
 * \class IAlgorithm
 * \author Yann Renard (INRIA/IRISA)
 * \date 2007-11-06
 * \brief Abstract algorithm, base element of OpenViBE processing objects
 * \ingroup Group_Extend
 *
 * This class should be derived by any plugin that is related
 * to data processing. It basically has a parameterable interface
 * contained in the IAlgorithmContext object. This interface
 * stores several typed parameters which can be modified either by
 * outside world in order to provide input paramters to this algorithm
 * or by this algorithm itself in order to produce output parameters.
 *
 * This is the heart of the extension mechanism of the
 * OpenViBE platform.
 *
 * \sa Kernel::IAlgorithmDesc
 * \sa Kernel::IAlgorithmContext
 *
 * \todo details about building new plugins
 */
class OV_API IAlgorithm : public IPluginObject
{
public:

    /**
     * \brief Initializes this algorithm
     * \param ctx [in] : the execution context for this algorithm
     * \return \e true in case of success.
     * \return \e false in case of error.
     */
    virtual bool initialize(Kernel::IAlgorithmContext& ctx) { return true; }
    /**
     * \brief Unitializes this algorithm
     * \param ctx [in] : the extecution context for this algorithm
     * \return \e true in case of success.
     * \return \e false in case of error.
     * \exception this method must be noexcept
     */
    virtual bool uninitialize(Kernel::IAlgorithmContext& ctx) { return true; }
    /**
     * \brief Effectively executes this algorithm
     * \param ctx [in] : the extecution context for this algorithm
     * \return \e true in case of success.
     * \return \e false in case of error.
     *
     * When called, this function should get its "input" parameters, do stuffs with them
     * and finally produce "output" parameters.
     */
    virtual bool process(Kernel::IAlgorithmContext& ctx) = 0;

    _IsDerivedFromClass_(IPluginObject, OV_ClassId_Plugins_Algorithm)
};
}  // namespace Plugins
}  // namespace OpenViBE
