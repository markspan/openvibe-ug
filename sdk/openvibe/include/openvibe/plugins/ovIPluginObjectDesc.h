#pragma once

#include "../ovIObject.h"

namespace OpenViBE {
namespace Plugins {
/// <summary> Functionality enumeration in order to know what a plugin is capable of </summary>
enum class EPluginFunctionality { Undefined, Processing, Visualization };

class IPluginObject;

/**
 * \class IPluginObjectDesc
 * \author Yann Renard (INRIA/IRISA)
 * \date 2006-06-19
 * \brief Base class for plugin descriptor
 * \ingroup Group_Extend
 *
 * This class is the base class for all the plugin description classes. 
 * It contains basic functions that could be used for each plugin description. 
 * Derived plugin descriptions will be used as a prototype of what they can create.
 *
 * \sa IBoxAlgorithmDesc
 * \sa IScenarioImporterDesc
 * \sa IScenarioExporterDesc
 * \sa IPluginObject
 *
 * \todo details about building new plugins
 */
class OV_API IPluginObjectDesc : public IObject
{
public:
    /** \name Memory management */
    //@{

    /**
     * \brief Informs the plugin description it won't be used any more
     *
     * This is called by the OpenViBE platform to inform the plugin description it is not useful anymore.
     * The plugin can chose whether to delete itself or to stay in memory until it decides it is ok to be deleted. 
     * However, the OpenViBE platform will not call any of the plugin functions after release is called.
     */
    virtual void release() = 0;

    //@}
    /** \name Creation process */
    //@{

    /**
     * \brief Gives a tip on what this plugin descriptor is able to create
     *
     * This may inform the OpenViBE platform about what kind of plugin can be created using this plugin descriptor. 
     * It should return the concrete class identifier of the plugin object itself.
     */
    virtual CIdentifier getCreatedClass() const = 0;
    virtual CIdentifier getCreatedClassIdentifier() const { return this->getCreatedClass(); }
    /**
     * \brief Creates the plugin object itself
     * \return the created object.
     *
     * This method creates the plugin object itself and returns it with the lowest level interface. 
     * The OpenVIBE platform then uses the IObject::isDerivedFromClass method to use the plugin correctly.
     */
    virtual IPluginObject* create() = 0;

    //@}
    /** \name Textual plugin object description and information */
    //@{

    /**
     * \brief Gets the plugin name
     * \return The plugin name.
     *
     * Default implementation simply returns empty string.
     */
    virtual CString getName() const { return "no name"; }
    /**
     * \brief Gets the author name for this plugin
     * \return The author name for this plugin.
     *
     * Default implementation simply returns "no name".
     */
    virtual CString getAuthorName() const { return "unknown"; }
    /**
     * \brief Gets the author company name for this plugin
     * \return The author company name for this plugin.
     *
     * Default implementation simply returns "unknown".
     */
    virtual CString getAuthorCompanyName() const { return "unknown"; }
    /**
     * \brief Gets a short description of the plugin
     * \return A short description of the plugin.
     *
     * Default implementation simply returns "unknown".
     */
    virtual CString getShortDescription() const { return ""; }
    /**
     * \brief Gets a detailed description of the plugin
     * \return A detailed description of the plugin.
     *
     * Default implementation simply returns empty string.
     *
     * \note You can use std::endl to have the description on several lines when needed.
     */
    virtual CString getDetailedDescription() const { return ""; }
    /**
     * \brief Gets a basic category of the plugin
     * \return the category tokens of the plugin
     *
     * The tokens should be separated with '/' characters in order to create sub categories.
     *
     * Default implementation returns "unknown".
     */
    virtual CString getCategory() const { return "unknown"; }
    /**
     * \brief Gets the version of the plugin
     * \return the version of the plugin.
     *
     * Default implementation simply returns "unknown".
     */
    virtual CString getVersion() const { return "unknown"; }
    /**
     * \brief Tests whether the plugin has a given functionality
     * \param functionality [in] : functionality of interest
     * \return \e true in case plugin has this functionality.
     * \return \e false otherwise.
     */
    virtual bool hasFunctionality(const EPluginFunctionality functionality) const { return false; }

    //@}

    _IsDerivedFromClass_(IObject, OV_ClassId_Plugins_PluginObjectDesc)
};
}  // namespace Plugins
}  // namespace OpenViBE
