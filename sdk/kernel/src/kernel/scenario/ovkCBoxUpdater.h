#pragma once

#include "../ovkTKernelObject.h"

#include "ovkCScenario.h"

#include <map>
#include <array>

namespace OpenViBE {
namespace Kernel {
typedef struct _InterfacorRequest
{
    size_t index           = size_t(-1);
    CIdentifier identifier = CIdentifier::undefined();
    CIdentifier typeID     = CIdentifier::undefined();
    CString name;
    bool toBeRemoved;

    // only for settings
    CString defaultValue;
    bool modifiability = false;
    CString value;
} InterfacorRequest;

class CBoxUpdater final : public TKernelObject<IKernelObject>
{
public:

    CBoxUpdater(CScenario& scenario, IBox* box);
    ~CBoxUpdater() override;

    bool initialize();

    const std::map<size_t, size_t>& getOriginalToUpdatedInterfacorCorrespondence(const EBoxInterfacorType type) const
    {
        return m_originalToUpdatedCorrespondence.at(type);
    }

    IBox& getUpdatedBox() const { return *m_updatedBox; }

    bool flaggedForManualUpdate() const
    {
        OV_FATAL_UNLESS_K(m_initialized, "Box Updater is not initialized", Kernel::ErrorType::BadCall);

        return m_kernelBox->hasAttribute(OV_AttributeId_Box_FlagNeedsManualUpdate)
               || m_kernelBox->hasAttribute(OV_AttributeId_Box_FlagCanAddInput)
               || m_kernelBox->hasAttribute(OV_AttributeId_Box_FlagCanModifyInput)
               || m_kernelBox->hasAttribute(OV_AttributeId_Box_FlagCanAddOutput)
               || m_kernelBox->hasAttribute(OV_AttributeId_Box_FlagCanModifyOutput)
               || m_kernelBox->hasAttribute(OV_AttributeId_Box_FlagCanAddSetting)
               || m_kernelBox->hasAttribute(OV_AttributeId_Box_FlagCanModifySetting);
    }

    bool isUpdateRequired() const { return m_isUpdateRequired; }

    static const std::array<CIdentifier, 10> UPDATABLE_ATTRIBUTES;

    _IsDerivedFromClass_Final_(TKernelObject<IKernelObject>, OV_ClassId_Kernel_Scenario_BoxUpdater)

private:

    static size_t getInterfacorIndex(EBoxInterfacorType type, const IBox& box, const CIdentifier& typeID, const CIdentifier& id, const CString& name);
    bool updateInterfacors(EBoxInterfacorType interfacorType);

    /**
     * \brief Check if supported type have to be updated between the box to be updated and the kernel
     * \return true when at least input or output supported types have to be updated
     */
    bool checkForSupportedTypesToBeUpdated() const;

    /**
     * \brief Check if supported inputs, outputs or settings attributes have to be updated between the box to be updated and the kernel
     * \return true when at least input, output or settings attributes have to be updated
     */
    bool checkForSupportedIOSAttributesToBeUpdated() const;

    // pointer to the parent scenario
    CScenario* m_scenario = nullptr;
    // pointer to the original box to be updated
    IBox* m_sourceBox = nullptr;
    // pointer to the kernel box
    const IBox* m_kernelBox = nullptr;
    // pointer to the updated box. This box will be used to update the prototype of the original box
    IBox* m_updatedBox = nullptr;
    // true when updater has been initialized
    bool m_initialized = false;

    std::map<EBoxInterfacorType, std::map<size_t, size_t>> m_originalToUpdatedCorrespondence;
    bool m_isUpdateRequired = false;
};
}  // namespace Kernel
}  // namespace OpenViBE
