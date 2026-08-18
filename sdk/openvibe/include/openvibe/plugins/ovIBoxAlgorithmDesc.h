#pragma once

#include "ovIPluginObjectDesc.h"

namespace OpenViBE {
namespace Kernel {
class IBoxProto;
class IBoxAlgorithmContext;
}  //namespace Kernel

namespace Plugins {
class IBoxListener;

/**
 * \class IBoxAlgorithmDesc
 * \author Yann Renard (INRIA/IRISA)
 * \date 2006-06-19
 * \brief Box algorithm plugin descriptor
 * \ingroup Group_Extend
 *
 * This class should be derived by any plugin developer in
 * order to describe a specific OpenViBE box algorithm.
 *
 * \sa IBoxAlgorithm
 *
 * \todo details about building new plugins
 */
class OV_API IBoxAlgorithmDesc : public IPluginObjectDesc
{
public:

    /** \name Box prototype and description */
    //@{

    /**
     * \brief Gets the box prototype for this algorithm
     * \param prototype [out] : the box prototype to fill
     * \return \e true in case of success \e false in other cases.
     *
     * When this function is called by the OpenViBE
     * platform, the plugin descriptor should fill in
     * the structure to let the OpenViBE platform know
     * what the corresponding box should look like
     * (inputs/outputs/settings).
     *
     * \sa IBoxProto
     */
    virtual bool getBoxPrototype(Kernel::IBoxProto& prototype) const = 0;
    /**
     * \brief Gets the stock item to display with this algorithm
     * \return The stock item to display with this algorithm.
     *
     * This item name will be used by the GUI to display
     * a symbol to the algorithm list so a user can quickly
     * find them in the list.
     *
     * Default implementation returns empty string. If
     * the item can not be found by name, or an empty string
     * is returned, a default item will be displayed.
     *
     */
    virtual CString getStockItemName() const { return ""; }

    //@{
    /** \name Box modification monitoring */
    //@{

    /**
     * \brief Creates a new box listener
     * \return a new box listener
     *
     * This function is called by the kernel when a box instance
     * is created if any modification flag is set in its prototype.
     * This box listener will be notified each time the box is modified.
     *
     * \sa Kernel::IBoxProto
     * \sa IBoxListener
     */
    virtual IBoxListener* createBoxListener() const { return nullptr; }
    /**
     * \brief Releases an existing box listener
     * \param listener [in] : the box listener to release
     *
     * This function is called by the kernel as soon as it knows
     * a box listener won't be used any more. In case this descriptor
     * allocated some memory for this box listener, this memory
     * can be freed safely, no more call will be done on this
     * box listener.
     */
    virtual void releaseBoxListener(IBoxListener* listener) const { }

    //@}

    _IsDerivedFromClass_(IPluginObjectDesc, OV_ClassId_Plugins_BoxAlgorithmDesc)
};
}  // namespace Plugins
}  // namespace OpenViBE
