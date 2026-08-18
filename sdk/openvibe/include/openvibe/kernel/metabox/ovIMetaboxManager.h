#pragma once

#include "../ovIKernelObject.h"
#include "../../metaboxes/ovIMetaboxObjectDesc.h"

namespace OpenViBE {
namespace Kernel {
/**
 * \class IMetaboxManager
 * \brief Metabox manager
 * \author Thierry Gaugry (INRIA/Mensia)
 * \date 2017-04-12
 * \ingroup Group_Metabox
 * \ingroup Group_Kernel
 *
 * The metabox manager is in charge of loading/unloading metaboxes
 * modules (defined in OV_ScenarioImportContext_OnLoadMetaboxImport import context)
 * containing OpenViBE metaboxes.
 * It also provides functions in order to list metabox descriptors,
 * create or release metabox objects...
 */
class OV_API IMetaboxManager : public IKernelObject
{
public:

    /** \name Metabox modules/descriptors management */
    //@{

    /**
     * \brief Loads new metaboxes module file(s)
     * \param[in] fileNameWildCard : a wild card with the file(s) to search metaboxes in
     * \return \e true in case of success.
     * \return \e false in case of error.
     */
    virtual bool addMetaboxesFromFiles(const CString& fileNameWildCard) = 0;

    /**
     * \brief Gets next metabox object descriptor identifier
     * \param[in] previousID : The identifier
     *        for the preceeding metabox object descriptor
     * \return The identifier of the next metabox object descriptor in case of success.
     * \retval \c CIdentifier::undefined() on error.
     * \note Giving \c CIdentifier::undefined() as \c previousID
     *       will cause this function to return the first metabox object
     *       descriptor identifier.
     */
    virtual CIdentifier getNextMetaboxObjectDescIdentifier(const CIdentifier& previousID) const = 0;

    /**
     * \brief Gets details on a specific metabox object descriptor
     * \param[in] id : the metabox object descriptor identifier which details should be returned
     * \return the corresponding metabox object descriptor pointer.
     */
    virtual const Plugins::IPluginObjectDesc* getMetaboxObjectDesc(const CIdentifier& id) const = 0;

    /**
     * \brief Sets details on a specific metabox object descriptor
     * \param[in] id : the metabox object descriptor identifier
     * \param[in] desc : the metabox object descriptor
     */
    virtual void setMetaboxObjectDesc(const CIdentifier& id, Plugins::IPluginObjectDesc* desc) = 0;


    /**
     * \brief Gets the path of the scenario of a specific metabox
     * \param[in] id : the metabox object descriptor identifier which path should be returned
     * \return the path to the scenario file of the metabox.
     */
    virtual CString getMetaboxFilePath(const CIdentifier& id) const = 0;
    /**
     * \brief Sets the path of the scenario of a specific metabox
     * \param[in] id : the metabox object descriptor identifier
     * \param[in] filePath : the metabox scenario path
     */
    virtual void setMetaboxFilePath(const CIdentifier& id, const CString& filePath) = 0;

    /**
     * \brief Gets the hash of the metabox
     * \param[in] id : the metabox object descriptor identifier which hash should be returned
     * \return the hash of the metabox.
     */
    virtual CIdentifier getMetaboxHash(const CIdentifier& id) const = 0;
    /**
     * \brief Sets the hash of the metabox
     * \param[in] id : the metabox object descriptor identifier
     * \param[in] hash : the metabox hash
     */
    virtual void setMetaboxHash(const CIdentifier& id, const CIdentifier& hash) = 0;

    _IsDerivedFromClass_(IKernelObject, OV_ClassId_Kernel_Metabox_MetaboxManager)
};
}  // namespace Kernel
}  // namespace OpenViBE
