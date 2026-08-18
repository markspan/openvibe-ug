#pragma once

#include "../ovIKernelObject.h"

namespace OpenViBE {
namespace Plugins {
class IPluginObject;
class IPluginObjectDesc;

class IBoxAlgorithm;
class IBoxAlgorithmDesc;

class IAlgorithm;
class IAlgorithmDesc;
}  // namespace Plugins

namespace Kernel {
// class IPluginModule;

/**
 * \class IPluginManager
 * \brief Log manager
 * \author Yann Renard (INRIA/IRISA)
 * \date 2006-09-26
 * \ingroup Group_Plugins
 * \ingroup Group_Kernel
 *
 * The plugin manager is in charge of loading/unloading plugin
 * modules (ie DLL/so files) containing OpenViBE plugins.
 * It also provides functions in order to list plugin descriptors,
 * create or release plugin objects...
 */
class OV_API IPluginManager : public IKernelObject
{
public:

    /** \name Plugin modules/descriptors management */
    //@{

    /**
     * \brief Loads new DLL/so plugin module file(s)
     * \param fileNameWildCard [in] : a wild card with the file(s) to search plugins in
     * \return \e true in case of success.
     * \return \e false in case of error.
     */
    virtual bool addPluginsFromFiles(const CString& fileNameWildCard) = 0;
    /**
     * \brief Registers a plugin object descriptor
     * \param desc [in] : the actual plugin object descriptor to register
     * \return \e true in case of success.
     * \return \e false in case of error.
     */
    virtual bool registerPluginDesc(const Plugins::IPluginObjectDesc& desc) = 0;
    /**
     * \brief Gets next plugin object descriptor identifier
     * \param previousID [in] : The identifier
     *        for the preceeding plugin object descriptor
     * \return The identifier of the next plugin object descriptor in case of success.
     * \return \c CIdentifier::undefined() on error.
     * \note Giving \c CIdentifier::undefined() as \c previousID
     *       will cause this function to return the first plugin object
     *       descriptor identifier.
     */
    virtual CIdentifier getNextPluginObjectDescIdentifier(const CIdentifier& previousID) const = 0;
    /**
     * \brief Gets next plugin object descriptor identifier given a base class identifier
     * \param previousID [in] : The identifier
     *        for the preceeding plugin object descriptor
     * \param baseClassID [in] : the class the plugin object descriptor should derive from
     * \return The identifier of the next plugin object descriptor in case of success.
     * \return \c CIdentifier::undefined() on error.
     * \note Giving \c CIdentifier::undefined() as \c previousID
     *       will cause this function to return the first plugin object
     *       descriptor identifier.
     */
    virtual CIdentifier getNextPluginObjectDescIdentifier(const CIdentifier& previousID, const CIdentifier& baseClassID) const = 0;
    /**
     * \brief Checks if a plugin object can be created or not
     * \param classID [in] : the class identifier a descriptor should be able to create
     * \return \e true in case this manager is able to create a plugin object with the provided class identifier.
     * \return \e false in other case.
     */
    virtual bool canCreatePluginObject(const CIdentifier& classID) = 0;
    /**
     * \brief Gets details on a specific plugin object descriptor
     * \param id [in] : the plugin object descriptor id which details should be returned
     * \return the corresponding plugin object descriptor pointer.
     * \sa getNextPluginObjectDescIdentifier
     */
    virtual const Plugins::IPluginObjectDesc* getPluginObjectDesc(const CIdentifier& id) const = 0;
    /**
     * \brief Gets details on a specific plugin object descriptor given the class identifier it should create
     * \param classID [in] : the plugin object class identifier of the descriptor which details should be returned
     * \return the corresponding plugin object descriptor pointer.
     * \sa canCreatePluginObject
     */
    virtual const Plugins::IPluginObjectDesc* getPluginObjectDescCreating(const CIdentifier& classID) const = 0;

    //@}
    /** \name Plugin lifetime management */
    //@{

    /**
     * \brief Gets a hash value for a given plugin
     * \param classID [in] : the class identifier of the plugin which hash value
     *        has to be returned
     * \return a hash code for the corresponding plugin object
     *
     * This function can be used to compute a has code of the described plugin object
     * If this hash code differs from session to session, it means that the plugin
     * descriptor changed in some way.
     *
     * \note The method used to compute the hash code is specific to the class
     *       of plugin that is considered (for example, for box algorithms, the
     *       hash code is based on what IBoxProto receives at
     *       description stage)
     */
    virtual CIdentifier getPluginObjectHashValue(const CIdentifier& classID) const = 0;

    virtual CIdentifier getPluginObjectHashValue(const Plugins::IBoxAlgorithmDesc& boxAlgorithmDesc) const = 0;
    /**
     * \brief Gets a hint whether a plugin is deprecated or not
     * \param classID [in] : the class identifier of the plugin which deprecation should be returned
     * \return \e true in case the plugin is deprecated
     * \return \e false in case the plugin is not deprecated
     *
     * If this function returns \e true, this means that the considered
     * plugin is still valid and functionnal but that it will
     * be removed soon or later. Code relying on this plugin
     * should consider any alternative available to avoid future problems.
     */
    virtual bool isPluginObjectFlaggedAsDeprecated(const CIdentifier& classID) const = 0;

    //@}
    /** \name Plugin creation and destruction */
    //@{

    /**
     * \brief Creates a new plugin object given its class identifier
     * \param classID [in] : the class identifier of the plugin object to create
     * \return a pointer on the newly created plugin object.
     * \return \c NULL in case of error.
     * \sa releasePluginObject
     */
    virtual Plugins::IPluginObject* createPluginObject(const CIdentifier& classID) = 0;
    /**
     * \brief Tells the plugin manager a plugin object won't be ever used
     * \param pluginObject [in] : the plugin object to release
     * \return \e true in case of success.
     * \return \e false in case of error.
     *
     * The client code should not call any function of the plugin object after this
     * call has been made. However, the plugin manager is free to release allocated memory
     * and resources for this plugin object.
     */
    virtual bool releasePluginObject(Plugins::IPluginObject* pluginObject) = 0;

    //@{
    /**\name Helpers */
    //@{

    /**
     * \brief Creates a new algorithm given its class identifier and eventually returns the associated descriptor
     * \param classID [in] : the class identifier of the algorithm to create
     * \param desc [out] : a pointer where to store the descriptor information
     * \return The newly created algorithm in case of success.
     * \return \c NULL in case of error.
     *
     * This function is a helper for the use of \c createPluginObject and co.
     */
    virtual Plugins::IAlgorithm* createAlgorithm(const CIdentifier& classID, const Plugins::IAlgorithmDesc** desc) = 0;
    /**
     * \brief Creates a new algorithm given a descriptor
     * \param desc [in] : the class descriptor of the algorithm to create
     * \return The newly created algorithm in case of success.
     * \return \c NULL in case of error.
     *
     * This function is a helper for the use of \c createPluginObject and co.
     */
    virtual Plugins::IAlgorithm* createAlgorithm(const Plugins::IAlgorithmDesc& desc) = 0;
    /**
     * \brief Creates a new box algorithm given its class identifier and eventually returns the associated descriptor
     * \param classID [in] : the class identifier of the box algorithm to create
     * \param desc [out] : a pointer where to store the descriptor information
     * \return The newly created box algorithm in case of success.
     * \return \c NULL in case of error.
     *
     * This function is a helper for the use of \c createPluginObject and co.
     */
    virtual Plugins::IBoxAlgorithm* createBoxAlgorithm(const CIdentifier& classID, const Plugins::IBoxAlgorithmDesc** desc) = 0;

    //@}

    _IsDerivedFromClass_(IKernelObject, OV_ClassId_Kernel_Plugins_PluginManager)
};
}  // namespace Kernel
}  // namespace OpenViBE
