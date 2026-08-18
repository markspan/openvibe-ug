#pragma once

#include "../ovIKernelObject.h"

namespace OpenViBE {
namespace Kernel {
/**
 * \class IMetadata
 * \author Jozef Legeny
 * \date 2016-10-11
 * \brief Metadata class for scenarios
 *
 * Scenario can contain string metadata for the purpose of being used by applications. Each piece of metadata is identified by a (unique) Identifier
 * and a (non-unique) Type. Applications can create metadata in scenarios and should not modify metadata with types they do not understand.
 *
 * \ingroup Group_Scenario
 * \ingroup Group_Kernel
 */
class OV_API IMetadata : public IKernelObject
{
public:

    /**
     * \return The identifier of this scenario metadata.
     */
    virtual CIdentifier getIdentifier() const = 0;

    /**
     * \return The type of this scenario metadata
     */
    virtual CIdentifier getType() const = 0;

    /**
     * \return The enclosed metadata
     */
    virtual CString getData() const = 0;

    /**
     * \brief Change the identifier of this metadata
     * \param[in] id The new id
     * \retval true in case of success.
     * \retval false in case of error.
     */
    virtual bool setIdentifier(const CIdentifier& id) = 0;

    /**
     * \brief Change the identifier of this metadata
     * \param[in] id The new id
     * \retval true in case of success.
     * \retval false in case of error.
     */
    virtual bool setType(const CIdentifier& id) = 0;

    /**
     * \brief Change the enclosed data
     * \param[in] data The text this comment should contain
     * \retval true in case of success.
     * \retval false in case of error.
     */
    virtual bool setData(const CString& data) = 0;

    /**
     * \brief Initializes the metadata from an already existing metadata
     * \param[in] metadata The existing metadata
     * \retval true in case of success.
     * \retval false in case of error.
     *
     * Resets the metadata and initializes its text/attributes according to the existing object.
     */
    virtual bool initializeFromExistingMetadata(const IMetadata& metadata) = 0;

    _IsDerivedFromClass_(IKernelObject, OV_ClassId_Kernel_Scenario_Metadata)
};
}  // namespace Kernel
}  // namespace OpenViBE
