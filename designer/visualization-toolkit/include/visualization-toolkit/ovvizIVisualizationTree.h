#pragma once

#include "ovvizIVisualizationWidget.h"

namespace OpenViBE {
namespace VisualizationToolkit {
/**
     * \brief Drag locations around a widget
     */
enum class EDragLocation { Left, Right, Top, Bottom };

/**
     * \brief Node types in a visualization tree
     */
enum class EVisualizationTreeNode
{
    Undefined,              ///< May be used for placeholders
    VPU,                    ///< Not used yet
    VisualizationWindow,    ///< A top level window
    VisualizationPanel,     ///< A notebook tab inside a window
    VisualizationBox,       ///< A visualization box (a plugin which displays something)
    VerticalSplit,          ///< A vertical split widget
    HorizontalSplit,        ///< A horizontal split widget
    Unaffected              ///< A special node parent to all unaffected visualization boxes
};

class IVisualizationTree;

/**
     * \class IVisualizationTree
     * \author Vincent Delannoy (INRIA/IRISA)
     * \date 2007-11
     * \brief Handles visualization widgets and their arrangement in space
     * This interface lets applications configure visualization widgets by adding them to windows and
     * tabs, positioning them next to each other using paned widgets, and resizing them as the user
     * sees fit. All widgets are saved in a symbolic way as IVisualizationWidget instances referenced in a tree store.
     * Methods of this class are essentially meant to be called from an external class inheriting from the ITreeViewCB
     * interface, for offline design or online use of the widgets.
     */
class IVisualizationTree
{
public:
    virtual ~IVisualizationTree() = default;
    /**
     * \brief Initializes the visualization tree
     * This method registers the scenario associated to this tree and creates a tree store.
     * \param scenario scenario associated to this tree store
     * \return true if tree was successfully initialized, false otherwise
     */
    virtual bool init(const Kernel::IScenario* scenario) = 0;

    /**
     * \name IVisualizationWidget management
     */
    //@{

    /**
     * \brief Iterates through IVisualizationWidget instances managed by this tree
     * Upon first call, this method should be passed an identifier initialized with CIdentifier::undefined().
     * It is modified at each subsequent call until all widgets have been returned (in which
     * case the identifier is reset to OV_Undefined)
     * \param id [in/out] identifier of current visualization widget when calling the method and of next widget upon return
     * \return true if a widget was found, false if past beyond last one
     */
    virtual bool getNextWidgetIdentifier(CIdentifier& id) const = 0;

    /**
     * \brief Iterates through IVisualizationWidget instances of type type managed by this tree
     * \param id [in/out] identifier of current visualization widget when calling the method and of next widget upon return
     * \param type restricts search to IVisualizationWidget instances of type type
     * \return true if a widget was found, false if past beyond last one
     * \sa getNextVisualizationWidgetIdentifier()
     */
    virtual bool getNextWidgetIdentifier(CIdentifier& id, EVisualizationWidget type) const = 0;

    /**
     * \brief Returns the IVisualizationWidget instance whose identifier is passed in parameter
     * \param id identifier to look for in the internal tree store
     * \return pointer to IVisualizationWidget whose identifier matches 'identifier' if any, nullptr otherwise
     */
    virtual IVisualizationWidget* getWidget(const CIdentifier& id) const = 0;

    /**
     * \brief Returns the IVisualizationWidget instance whose corresponding IBox identifier matches 'boxID'
     * This method inherently restricts the search to nodes of type EVisualizationTreeNode::VisualizationBox
     * since only these nodes are associated to an IBox instance.
     * \param boxID identifier of IBox whose associated IVisualizationWidget is to be retrieved
     * \return IVisualizationWidget whose associated IBox identifier matches 'boxID' if any, nullptr otherwise
     */
    virtual IVisualizationWidget* getWidgetFromBoxIdentifier(const CIdentifier& boxID) const = 0;

    /**
     * \brief Adds an IVisualizationWidget instance to the tree
     * Seven parameters describe the IVisualizationWidget to be created. Not all are relevant for every type of widget.
     * \param id identifier of the widget to be created
     * \param name name of the widget
     * \param type type of the widget
     * \param parentID parent widget identifier (CIdentifier::undefined() for top-level widgets)
     * \param parentIdx index where this widget is to be parented (irrelevant for top-level widgets)
     * \param boxID identifier of associated IBox (for widgets of type EVisualizationWidget::Box only)
     * \param nChild number of children of this widget (none for a visualization box, 1 for a visualization panel, 2 for split widgets, variable number for windows)
     * \param suggestedID a suggestion as to the identifier to use
     * \return true if widget successfully added to the internal tree store, false otherwise
     */
    virtual bool addWidget(CIdentifier& id, const CString& name, EVisualizationWidget type, const CIdentifier& parentID, size_t parentIdx,
                                        const CIdentifier& boxID, size_t nChild, const CIdentifier& suggestedID) = 0;

    /**
     * \brief Returns the index where a widget is parented
     * Irrelevant for top level windows.
     * \param id identifier of the IVisualizationWidget whose index is to be retrieved
     * \param index [out] index where the widget is parented
     * \return true if widget index could be determined, false otherwise
     */
    virtual bool getWidgetIndex(const CIdentifier& id, size_t& index) const = 0;

    /**
     * \brief Unparents a widget from its parent, if any
     * \param id identifier of widget to be unparented
     * \param index [out] index where this widget was parented
     * \return true if widget could be removed from its parent, false otherwise
     */
    virtual bool unparentWidget(const CIdentifier& id, size_t& index) = 0;

    /**
     * \brief Parents a widget to a parent widget
     * \param id identifier of widget to be parented
     * \param parentID identifier of parent widget
     * \param index index where widget is to be parented
     * \return true if widget could be parented as desired, false otherwise
     */
    virtual bool parentWidget(const CIdentifier& id, const CIdentifier& parentID, const size_t index) = 0;

    /**
     * \brief Destroys a widget hierarchy
     * \param id identifier of widget that is to be destroyed, along with all widgets in its subtree
     * \param destroy if false, widgets of type EVisualizationTreeNode::VisualizationBox are unaffected only (as opposed to destroyed)
     * \return true if hierarchy was successfully destroyed, false otherwise
     */
    virtual bool destroyHierarchy(const CIdentifier& id, bool destroy = true) = 0;
    //@}

    /**
     * \return String representation of the visualization tree
     */
    virtual CString serialize() const = 0;

    /**
     * \brief Initializes the visualization tree from a string representation
     * \param tree The string representation of the new tree
     * \retval true In case of success
     * \retval false In case of error
     */
    virtual bool deserialize(const CString& tree) = 0;
};
}  // namespace VisualizationToolkit
}  // namespace OpenViBE
