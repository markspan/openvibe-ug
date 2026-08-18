#pragma once

#include "ovIAttributable.h"

namespace OpenViBE {
namespace Kernel {
/**
 * \class ILink
 * \author Yann Renard (IRISA/INRIA)
 * \date 2006-08-16
 * \brief Link information between OpenViBE box
 * \ingroup Group_Scenario
 * \ingroup Group_Kernel
 *
 * This class collects informations between OpenViBE
 * boxes : box identifiers and input / output indices.
 */
class OV_API ILink : public IAttributable
{
public:

    /** \name Identification */
    //@{

    /**
     * \brief Initialize link from existing link by copying members
     * \param link  the existing link
     * @return true
     */
    virtual bool initializeFromExistingLink(const ILink& link) = 0;

    /**
     * \brief Changes this link's id
     * \param id [in] : The new id this link should have
     * \return \e true in case of success.
     * \return \e false in case of error.
     */
    virtual bool setIdentifier(const CIdentifier& id) = 0;
    /**
     * \brief Gets the identifier of this link
     * \return The identifier of this link.
     */
    virtual CIdentifier getIdentifier() const = 0;

    //@}
    /** \name Source / Target management */
    //@{

    /**
     * \brief Sets the source of this link
     * \param boxID [in] : The identifier of the source box
     * \param boxOutputIdx [in] : The index of the output to use on the source box
     * \param boxOutputID [in] : The identifier of the output to use on the source box
     * \return \e true in case of success.
     * \return \e false in case of error.
     */
    virtual bool setSource(const CIdentifier& boxID, size_t boxOutputIdx, CIdentifier boxOutputID) = 0;
    /**
     * \brief Sets the target of this link
     * \param boxID [in] : The identifier of the target box
     * \param boxInputIdx [in] : The index of the input to use on the target box
     * \param boxInputID [in] : The identifier of the input to use on the target box
     * \return \e true in case of success.
     * \return \e false in case of error.
     */
    virtual bool setTarget(const CIdentifier& boxID, size_t boxInputIdx, CIdentifier boxInputID) = 0;
    /**
     * \brief Gets the source information for this link
     * \param boxID [out] : The identifier of the source box
     * \param boxOutputIdx [out] : The output index of the source box
     * \param boxOutputID [out] : The output identifier of the target box
     * \return \e true in case of success.
     * \return \e false in case of error.
     */
    virtual bool getSource(CIdentifier& boxID, size_t& boxOutputIdx, CIdentifier& boxOutputID) const = 0;
    /**
     * \brief Gets the source box identifier for this link
     * \return the source box identifier for thit link
     */
    virtual CIdentifier getSourceBoxIdentifier() const = 0;
    /**
     * \brief Gets the source box output index for this link
     * \return the source box output index for this link
     */
    virtual size_t getSourceBoxOutputIndex() const = 0;
    /**
     * \brief Gets the source box output index for this link
     * \return the source box output index for this link
     */
    virtual CIdentifier getSourceBoxOutputIdentifier() const = 0;
    /**
     * \brief Gets the target information for this link
     * \param boxID [out] : The identifier of the target box
     * \param boxInputIdx [out] : The input index of the target box
     * \param boxInputID [out] : The input identifier of the target box
     * \return \e true in case of success.
     * \return \e false in case of error.
     */
    virtual bool getTarget(CIdentifier& boxID, size_t& boxInputIdx, CIdentifier& boxInputID) const = 0;
    /**
     * \brief Gets the target box identifier for this link
     * \return the target box identifier for this link
     */
    virtual CIdentifier getTargetBoxIdentifier() const = 0;
    /**
     * \brief Gets the target box input index for this link
     * \return the target box input index for this link
     */
    virtual size_t getTargetBoxInputIndex() const = 0;
    /**
     * \brief Gets the target box input identifier for this link
     * \return the target box input identifier for this link
     */
    virtual CIdentifier getTargetBoxInputIdentifier() const = 0;

    //@}

    _IsDerivedFromClass_(IAttributable, OV_ClassId_Kernel_Scenario_Link)
};
}  // namespace Kernel
}  // namespace OpenViBE
