#include "../ovkTKernelObject.h"

#include "ovkCScenario.h"
#include "ovkCBoxUpdater.h"
#include "ovkTBox.hpp"

namespace OpenViBE {
namespace Kernel {

const std::array<CIdentifier, 10> CBoxUpdater::UPDATABLE_ATTRIBUTES = {
    OV_AttributeId_Box_InitialPrototypeHashValue,
    OV_AttributeId_Box_InitialInputCount,
    OV_AttributeId_Box_InitialOutputCount,
    OV_AttributeId_Box_InitialSettingCount,
    OV_AttributeId_Box_FlagCanAddInput,
    OV_AttributeId_Box_FlagCanModifyInput,
    OV_AttributeId_Box_FlagCanAddOutput,
    OV_AttributeId_Box_FlagCanModifyOutput,
    OV_AttributeId_Box_FlagCanAddSetting,
    OV_AttributeId_Box_FlagCanModifySetting
};

CBoxUpdater::CBoxUpdater(CScenario& scenario, IBox* box)
    : TKernelObject<IKernelObject>(scenario.getKernelContext()), m_scenario(&scenario), m_sourceBox(box)
{
    m_originalToUpdatedCorrespondence[Input]   = std::map<size_t, size_t>();
    m_originalToUpdatedCorrespondence[Output]  = std::map<size_t, size_t>();
    m_originalToUpdatedCorrespondence[Setting] = std::map<size_t, size_t>();
}

CBoxUpdater::~CBoxUpdater()
{
    if (!m_kernelBox || !m_updatedBox) { return; }

    if (m_kernelBox->getAlgorithmClassIdentifier() != OVP_ClassId_BoxAlgorithm_Metabox)
    {
        // do not manage destruction of metaboxes (done by metabox manager)
        delete m_kernelBox;
    }

    delete m_updatedBox;
}

bool CBoxUpdater::initialize()
{
    // initialize kernel box reference
    if (m_sourceBox->getAlgorithmClassIdentifier() == OVP_ClassId_BoxAlgorithm_Metabox)
    {
        const CString metaboxID = m_sourceBox->getAttributeValue(OVP_AttributeId_Metabox_ID);
        OV_ERROR_UNLESS_KRF(metaboxID != CString(""), "Failed to find metabox with id " << metaboxID, Kernel::ErrorType::BadCall);

        CIdentifier metaboxId;
        metaboxId.fromString(metaboxID);
        const CString path(this->getKernelContext().getMetaboxManager().getMetaboxFilePath(metaboxId));

        OV_ERROR_UNLESS_KRF(path != CString(""), "Metabox scenario is not available for " << m_sourceBox->getName(), Kernel::ErrorType::BadCall);


        // We are going to copy the template scenario, flatten it and then copy all
        // Note that copy constructor for IScenario does not exist
        CIdentifier templateID;
        this->getKernelContext().getScenarioManager().importScenarioFromFile(templateID, OV_ScenarioImportContext_SchedulerMetaboxImport, path);

        CScenario* instance = dynamic_cast<CScenario*>(&(this->getKernelContext().getScenarioManager().getScenario(templateID)));
        instance->setAlgorithmClassIdentifier(OVP_ClassId_BoxAlgorithm_Metabox);
        m_kernelBox = instance;
    }
    else
    {
        CBox* kernelBox = new CBox(m_scenario->getKernelContext());
        kernelBox->initializeFromAlgorithmClassIdentifierNoInit(m_sourceBox->getAlgorithmClassIdentifier());
        m_kernelBox = kernelBox;
    }

    // initialize updated box
    m_updatedBox = new CBox(m_scenario->getKernelContext());

    m_updatedBox->setIdentifier(m_sourceBox->getIdentifier());
    m_updatedBox->setName(m_sourceBox->getName());

    if (m_sourceBox->getAlgorithmClassIdentifier() == OVP_ClassId_BoxAlgorithm_Metabox)
    {
        m_updatedBox->setAttributeValue(OV_AttributeId_Box_InitialPrototypeHashValue, m_kernelBox->getAttributeValue(OV_AttributeId_Scenario_MetaboxHash));
    }

    // initialize updated box attribute to kernel ones
    CIdentifier attributeIdentifier;
    while ((attributeIdentifier = m_kernelBox->getNextAttributeIdentifier(attributeIdentifier)) != CIdentifier::undefined())
    {
        CString attributeValue = m_kernelBox->getAttributeValue(attributeIdentifier);
        m_updatedBox->addAttribute(attributeIdentifier, attributeValue);
    }

    // initialize supported types to kernel ones
    if (m_sourceBox->getAlgorithmClassIdentifier() != OVP_ClassId_BoxAlgorithm_Metabox)
    {
        m_updatedBox->setSupportTypeFromAlgorithmIdentifier(m_kernelBox->getAlgorithmClassIdentifier());
    }
    // should not be done before adding IO elements so the box listener is never called
    // updatedBox->setAlgorithmClassIdentifier(kernelBox->getAlgorithmClassIdentifier());
    m_initialized = true;

    m_isUpdateRequired = false;

    const bool isHashDifferent = m_scenario->isBoxOutdated(m_sourceBox->getIdentifier());

    if (this->flaggedForManualUpdate())
    {
        m_isUpdateRequired = isHashDifferent;
        return true;
    }

    if (isHashDifferent)
    {
        m_isUpdateRequired |= this->updateInterfacors(Input);
        m_isUpdateRequired |= this->updateInterfacors(Output);
        m_isUpdateRequired |= this->updateInterfacors(Setting);
        m_isUpdateRequired |= this->checkForSupportedTypesToBeUpdated();
        m_isUpdateRequired |= this->checkForSupportedIOSAttributesToBeUpdated();
    }

    if (m_sourceBox->getAlgorithmClassIdentifier() == OVP_ClassId_BoxAlgorithm_Metabox) { m_isUpdateRequired |= isHashDifferent; }

    return true;
}


bool CBoxUpdater::checkForSupportedTypesToBeUpdated() const
{
    //check for supported inputs diff
    for (auto& type : m_sourceBox->getInputSupportTypes()) { if (!m_kernelBox->hasInputSupport(type)) { return true; } }
    for (auto& type : m_kernelBox->getInputSupportTypes()) { if (!m_sourceBox->hasInputSupport(type)) { return true; } }

    //check for supported outputs diff
    for (auto& type : m_sourceBox->getOutputSupportTypes()) { if (!m_kernelBox->hasOutputSupport(type)) { return true; } }
    for (auto& type : m_kernelBox->getOutputSupportTypes()) { if (!m_sourceBox->hasOutputSupport(type)) { return true; } }
    return false;
}

bool CBoxUpdater::checkForSupportedIOSAttributesToBeUpdated() const
{
    // check for attributes
    for (auto& attr : UPDATABLE_ATTRIBUTES)
    {
        if ((m_sourceBox->hasAttribute(attr) && !m_kernelBox->hasAttribute(attr))
            || (!m_sourceBox->hasAttribute(attr) && m_kernelBox->hasAttribute(attr))) { return true; }
    }
    return false;
}

bool CBoxUpdater::updateInterfacors(const EBoxInterfacorType interfacorType)
{
    std::vector<InterfacorRequest> interfacors;

    bool updated = false;

    // First add and optionally modify all requests from the Kernel prototype
    size_t index = 0;
    while (index < m_kernelBox->getInterfacorCount(interfacorType))
    {
        CIdentifier kTypeIdentifier;
        CIdentifier kIdentifier;
        CString kName;
        m_kernelBox->getInterfacorType(interfacorType, index, kTypeIdentifier);
        m_kernelBox->getInterfacorIdentifier(interfacorType, index, kIdentifier);
        m_kernelBox->getInterfacorName(interfacorType, index, kName);


        InterfacorRequest request;
        request.index       = index;
        request.identifier  = kIdentifier;
        request.name        = kName;
        request.typeID      = kTypeIdentifier;
        request.toBeRemoved = false;

        if (interfacorType == Setting)
        {
            CString defaultValue;
            bool modifiable;
            m_kernelBox->getSettingDefaultValue(index, defaultValue);
            m_kernelBox->getSettingMod(index, modifiable);
            request.defaultValue  = defaultValue;
            request.value         = defaultValue;
            request.modifiability = modifiable;
        }

        auto indexInBox = getInterfacorIndex(interfacorType, *m_sourceBox, kTypeIdentifier, kIdentifier, kName);
        if (indexInBox != size_t(-1))
        {
            CIdentifier identifier;
            CString name;

            m_sourceBox->getInterfacorIdentifier(interfacorType, indexInBox, identifier);
            m_sourceBox->getInterfacorName(interfacorType, indexInBox, name);
            updated |= (identifier == kIdentifier && name != kName) || (identifier != kIdentifier && name == kName);

            m_originalToUpdatedCorrespondence[interfacorType][indexInBox] = index;

            // For settings, we need to give them the value from the original box
            if (interfacorType == Setting)
            {
                CString valueInBox;
                m_sourceBox->getSettingValue(indexInBox, valueInBox);
                request.value = valueInBox;
            }
        }
        else
        {
            // try to modify the type in the kernel proto to adjust to the inputs
            updated = true;
        }

        interfacors.push_back(request);

        ++index;
    }

    // Now go throught the inputs we have not yet associated and decide what to do with them
    // As we currently only support boxes with fixed amount of inputs/outputs this always means that the
    // I/O is redundant
    index = 0;
    while (index < m_sourceBox->getInterfacorCount(interfacorType))
    {
        // Skip if the input was handled in the previous step
        if (m_originalToUpdatedCorrespondence.at(interfacorType).find(index) != m_originalToUpdatedCorrespondence.at(interfacorType).end())
        {
            ++index;
            continue;
        }

        CIdentifier sTypeIdentifier;
        CIdentifier sIdentifier;
        CString name;
        m_sourceBox->getInterfacorType(interfacorType, index, sTypeIdentifier);
        m_sourceBox->getInterfacorIdentifier(interfacorType, index, sIdentifier);
        m_sourceBox->getInterfacorName(interfacorType, index, name);

        InterfacorRequest request;
        request.index       = index;
        request.identifier  = sIdentifier;
        request.name        = name;
        request.typeID      = sTypeIdentifier;
        request.toBeRemoved = true;

        if (interfacorType == Setting)
        {
            CString defaultValue;
            CString value;
            bool modifiable;
            m_sourceBox->getSettingDefaultValue(index, defaultValue);
            m_sourceBox->getSettingValue(index, value);
            m_sourceBox->getSettingMod(index, modifiable);
            request.defaultValue  = defaultValue;
            request.value         = value;
            request.modifiability = modifiable;
        }

        updated = true;

        interfacors.push_back(request);

        m_originalToUpdatedCorrespondence[interfacorType][index] = interfacors.size() - 1;

        ++index;
    }

    for (auto& i : interfacors)
    {
        m_updatedBox->addInterfacor(interfacorType, i.name, i.typeID, i.identifier);
        if (interfacorType == Setting)
        {
            const auto idx = m_updatedBox->getInterfacorCountIncludingDeprecated(Setting) - 1;
            m_updatedBox->setSettingDefaultValue(idx, i.defaultValue);
            m_updatedBox->setSettingValue(idx, i.value);
            m_updatedBox->setSettingMod(idx, i.modifiability);
        }
        if (i.toBeRemoved)
        {
            m_updatedBox->setInterfacorDeprecatedStatus(interfacorType, m_updatedBox->getInterfacorCountIncludingDeprecated(interfacorType) - 1, true);
        }
    }

    return updated;
}

size_t CBoxUpdater::getInterfacorIndex(const EBoxInterfacorType type, const IBox& box, const CIdentifier& typeID, const CIdentifier& id, const CString& name)
{
    size_t index = size_t(-1);
    if (id != CIdentifier::undefined() && box.hasInterfacorWithIdentifier(type, id)) { box.getInterfacorIndex(type, id, index); }
    else if (box.hasInterfacorWithNameAndType(type, name, typeID)) { box.getInterfacorIndex(type, name, index); }

    return index;
}

}  // namespace Kernel
}  // namespace OpenViBE
