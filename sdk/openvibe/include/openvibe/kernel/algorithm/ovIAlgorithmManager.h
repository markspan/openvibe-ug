#pragma once

#include "../ovIKernelObject.h"

namespace OpenViBE {
namespace Plugins {
class IAlgorithmDesc;
}  // namespace Plugins

namespace Kernel {
class IAlgorithmProxy;

/**
 * \class IAlgorithmManager
 * \author Yann Renard (INRIA/IRISA)
 * \date 2007-11-06
 * \brief Manager for all kind of plugin algorithms
 * \ingroup Group_Algorithm
 * \ingroup Group_Kernel
 * \sa Plugins::IAlgorithm
 */
class OV_API IAlgorithmManager : public IKernelObject
{
public:

    /**
     * \brief Creates a new algorithm
     * \param algorithmClassID [out] : the class identifier of
     *        the newly created algorithm
     * \return \e true in case of success.
     * \return \e CIdentifier::undefined() in case of error.
     */
    virtual CIdentifier createAlgorithm(const CIdentifier& algorithmClassID) = 0;
    /**
      * \brief Creates a new algorithm
      * \param algorithmDesc [in] : the algorithm descriptor of
      *        the algorithm to create
      * \return \e identifier of the created algorithm
      * \return \e CIdentifier::undefined()
      */
    virtual CIdentifier createAlgorithm(const Plugins::IAlgorithmDesc& algorithmDesc) = 0;

    /**
     * \brief Releases an existing algorithm
     * \param id [in] : the existing algorithm identifier
     * \return \e true in case of success.
     * \return \e false in case of error.
     */
    virtual bool releaseAlgorithm(const CIdentifier& id) = 0;
    /**
     * \brief Releases an existing algorithm
     * \param algorithm [in] : the existing algorithm
     * \return \e true in case of success.
     * \return \e false in case of error.
     */
    virtual bool releaseAlgorithm(IAlgorithmProxy& algorithm) = 0;
    /**
     * \brief Gets details on a specific algorithm
     * \param id [in] : the algorithm identifier which details should be returned
     * \return the corresponding algorithm reference.
     * \warning Calling this function with a bad identifier causes a crash
     */
    virtual IAlgorithmProxy& getAlgorithm(const CIdentifier& id) = 0;
    /**
     * \brief Gets next algorithm identifier
     * \param previousID [in] : The identifier
     *        for the preceeding algorithm
     * \return The identifier of the next algorithm in case of success.
     * \return \c CIdentifier::undefined() on error.
     * \note Giving \c CIdentifier::undefined() as \c previousID
     *       will cause this function to return the first algorithm
     *       identifier.
     */
    virtual CIdentifier getNextAlgorithmIdentifier(const CIdentifier& previousID) const = 0;

    _IsDerivedFromClass_(IKernelObject, OV_ClassId_Kernel_Algorithm_AlgorithmManager)
};
}  // namespace Kernel
}  // namespace OpenViBE
