#pragma once

#include <openvibe/ov_all.h>

namespace OpenViBE {
namespace VisualizationToolkit {
/// <summary> This enum lists the different types of IVisualizationWidget supported by the platform. </summary>
enum class EVisualizationWidget
{
    Undefined,      ///< Undefined widget (empty slot in an IVisualizationTree)
    Window,         ///< Top-level IVisualizationWidget container
    Panel,          ///< Notebook tab containing IVisualizationWidget objects
    Box,            ///< Visualization plugin
    VerticalSplit,  ///< Split widget that divides its client area vertically in two
    HorizontalSplit ///< Split widget that divides its client area horizontally in two
};

/**
 * \class IVisualizationWidget
 * \author Vincent Delannoy (INRIA/IRISA)
 * \date 2007-11
 * \brief Interface of visualization widgets that are handled by an IVisualizationTree
 * These objects are stored in an IVisualizationTree object as they are being created and modified
 * to suit the graphical needs of a scenario.
 */
class IVisualizationWidget
{
public:
    virtual ~IVisualizationWidget() = default;
    /**
     * \brief Initializes the widget
     * \param identifier identifier of the widget
     * \param name name of the widget (optional)
     * \param type type of the widget
     * \param parentID parent widget identifier (OV_Undefined for top-level widgets)
     * \param boxID if widget type is EVisualizationWidget::Box, identifier of corresponding IBox
     * \param nChild number of children of this widget (none for a visualization box, 1 for a visualization panel, 2 for split widgets, variable number for windows)
     * \return True if widget was successfully initialized, false otherwise
     */
    virtual bool initialize(const CIdentifier& identifier, const CString& name, const EVisualizationWidget type, const CIdentifier& parentID,
                            const CIdentifier& boxID, const size_t nChild) = 0;

    /**
     * \brief Returns the identifier of the widget
     * \return Widget identifier
     */
    virtual CIdentifier getIdentifier() const = 0;

    /**
     * \brief Returns the name of the widget
     * \return Widget name
     */
    virtual const CString& getName() const = 0;

    /**
     * \brief Sets the name of the widget
     * \param name name to give to the widget
     */
    virtual void setName(const CString& name) = 0;

    /**
     * \brief Returns the type of the widget
     * \return Widget type
     */
    virtual EVisualizationWidget getType() const = 0;

    /**
     * \brief Returns the identifier of the widget's parent (if any)
     * \return Widget's parent identifier if any, OV_Undefined otherwise
     */
    virtual CIdentifier getParentIdentifier() const = 0;
    /**
     * \brief Sets the identifier of the widget's parent
     * \param parentID identifier of the widget's parent
     */
    virtual void setParentIdentifier(const CIdentifier& parentID) = 0;

    /**
     * \brief Returns the identifier of the IBox associated to this widget.
     *
     * This only applies to widgets of type EVisualizationWidget::Box.
     * \return Identifier of IBox associated to this widget
     */
    virtual CIdentifier getBoxIdentifier() const = 0;

    /**
     * \brief Returns the number of children of this widget
     * \return Number of child widgets
     */
    virtual size_t getNbChildren() const = 0;

    /**
     * \brief Returns the index of a given child
     * \param identifier identifier of a child widget
     * \param index [out] index at which the child widget is stored
     * \return True if the child was found, false otherwise
     */
    virtual bool getChildIndex(const CIdentifier& identifier, size_t& index) const = 0;

    /**
     * \brief Adds a child to a widget
     *
     * Only useful for top-level widgets (EVisualizationWidget::Window) since the number
     * of tabs their notebook may contain is unknown a priori. The child is added after existing children.
     * \param childIdentifier identifier of child to be added to widget
     * \return True if child was successfully added
     */
    virtual bool addChild(const CIdentifier& childIdentifier) = 0;

    /**
     * \brief Removes a child from a widget
     * \param childIdentifier identifier of child to be removed to the widget
     * \return True if the child was successfully removed
     */
    virtual bool removeChild(const CIdentifier& childIdentifier) = 0;

    /**
     * \brief Returns the identifier of a given child
     * \param childIndex index of child whose identifier is to be retrieved
     * \param childIdentifier [out] identifier of child
     * \return True if child identifier was successfully returned, false otherwise
     */
    virtual bool getChildIdentifier(const size_t childIndex, CIdentifier& childIdentifier) const = 0;

    /**
     * \brief Sets the identifier of a child
     * \param childIndex index of child whose identifier is to be set
     * \param childIdentifier identifier of the child to be added to the widget
     * \return True if the child was successfully set
     */
    virtual bool setChildIdentifier(const size_t childIndex, const CIdentifier& childIdentifier) = 0;


    virtual void setWidth(const size_t width) = 0;
    virtual void setHeight(const size_t height) = 0;
    virtual size_t getWidth() = 0;
    virtual size_t getHeight() = 0;

    virtual void setDividerPosition(const int dividerPosition) = 0;
    virtual void setMaxDividerPosition(const int maxDividerPosition) = 0;
    virtual int getDividerPosition() = 0;
    virtual int getMaxDividerPosition() = 0;
};
}  // namespace VisualizationToolkit
}  // namespace OpenViBE
