#pragma once

#include "../ovkTKernelObject.h"

#include "ovkTAttributable.h"
#include "ovkCBoxListenerContext.h"
#include "ovkCBoxProto.h"
#include <openvibe/plugins/ovIPluginObjectDesc.h>
#include "../ovkCObjectVisitorContext.h"
#include <openvibe/ov_defines.h>
#include <system/ovCMath.h>

#include <algorithm>
#include <vector>
#include <string>
#include <iostream>
#include <memory>

const std::map<OpenViBE::Kernel::EBoxInterfacorType, OpenViBE::CString> INTERFACOR_TYPE_TO_NAME = {
    { OpenViBE::Kernel::EBoxInterfacorType::Setting, "Setting" },
    { OpenViBE::Kernel::EBoxInterfacorType::Input, "Input" },
    { OpenViBE::Kernel::EBoxInterfacorType::Output, "Output" }
};
//This class is used to set up the restriction of a stream type for input and output. Each box comes with a
// decriptor that call functions describe in IBoxProto for intialize the CBox object.
// This implementation is derived from CBoxProto, to benefit from
// the implementation of the stream restriction mecanism but neutralizes all other initialization function.
class CBoxProtoRestriction final : public OpenViBE::Kernel::CBoxProto
{
public:

    CBoxProtoRestriction(const OpenViBE::Kernel::IKernelContext& ctx, OpenViBE::Kernel::IBox& box): CBoxProto(ctx, box) {}

    bool addInput(const OpenViBE::CString& /*name*/, const OpenViBE::CIdentifier& /*typeID*/,
                  const OpenViBE::CIdentifier& /*identifier*/ = OpenViBE::CIdentifier::undefined(), const bool /*notify*/  = true) override { return true; }

    bool addOutput(const OpenViBE::CString& /*name*/, const OpenViBE::CIdentifier& /*typeID*/,
                   const OpenViBE::CIdentifier& /*identifier*/ = OpenViBE::CIdentifier::undefined(), const bool /*notify*/  = true) override { return true; }

    bool addSetting(const OpenViBE::CString& /*name*/, const OpenViBE::CIdentifier& /*typeID*/, const OpenViBE::CString& /*defaultValue*/,
                    const bool /*modifiable*/  = false, const OpenViBE::CIdentifier& /*identifier*/ = OpenViBE::CIdentifier::undefined(),
                    const bool /*notify*/      = true) override { return true; }

    bool addFlag(const OpenViBE::Kernel::EBoxFlag /*boxFlag*/) override { return true; }
    bool addFlag(const OpenViBE::CIdentifier& /*identifier*/) override { return true; }
};

class CInterfacor
{
public:
    CInterfacor() { }

    CInterfacor(const CInterfacor& other)
        : m_Name(other.m_Name), m_TypeID(other.m_TypeID), m_ID(other.m_ID), m_Deprecated(other.m_Deprecated) { }

    CInterfacor(const OpenViBE::CString& name, const OpenViBE::CIdentifier& idType, const OpenViBE::CIdentifier& id)
        : m_Name(name), m_TypeID(idType), m_ID(id) {}

    virtual void setName(const OpenViBE::CString& name) { m_Name = name; }
    virtual void setTypeID(const OpenViBE::CIdentifier& idType) { m_TypeID = idType; }
    virtual void setID(const OpenViBE::CIdentifier& id) { m_ID = id; }

    OpenViBE::CString m_Name;
    OpenViBE::CIdentifier m_TypeID = OpenViBE::CIdentifier::undefined();
    OpenViBE::CIdentifier m_ID     = OpenViBE::CIdentifier::undefined();
    bool m_Deprecated              = false;
};

class CInputOutput : public CInterfacor
{
public:
    CInputOutput() { }
    CInputOutput(const CInputOutput& i) : CInterfacor(i) { this->m_Deprecated = false; }
    CInputOutput(const OpenViBE::CString& name, const OpenViBE::CIdentifier& idType, const OpenViBE::CIdentifier& id) : CInterfacor(name, idType, id) { }
};

class CSetting : public CInterfacor
{
public:
    CSetting() { }
    CSetting(const CSetting& s) :
     CInterfacor(s),
     m_parameter(s.m_parameter ? s.m_parameter->clone() : nullptr ),
     m_DefaultValue(s.m_DefaultValue),
     m_Mod(s.m_Mod) {
         this->m_Deprecated = false;
    }

    // Copy assignment operator
    CSetting& operator=(const CSetting& s) {
        if (this != &s) {
            CInterfacor::operator=(s);
            m_parameter.reset(s.m_parameter ? s.m_parameter->clone() : nullptr);
            m_DefaultValue = s.m_DefaultValue;
            m_Mod = s.m_Mod;
        }
        return *this;
    }

    // Move constructor (for performance)
    CSetting(CSetting&& s) noexcept :
        CInterfacor(std::move(s)),
        m_parameter(std::move(s.m_parameter)),
        m_DefaultValue(std::move(s.m_DefaultValue)),
        m_Mod(s.m_Mod) {
    }

    // Move assignment operator
    CSetting& operator=(CSetting&& s) noexcept {
        if (this != &s) {
            CInterfacor::operator=(std::move(s));
            m_parameter = std::move(s.m_parameter);
            m_DefaultValue = std::move(s.m_DefaultValue);
            m_Mod = s.m_Mod;
        }
        return *this;
    }

    void setName(const OpenViBE::CString& name) override {
        CInterfacor::setName(name);
        if (m_parameter != nullptr) {
            m_parameter->setName(name.toASCIIString());
        }
    }

    OpenViBE::CString getValue() const {
        if (m_parameter != nullptr) {
            return m_parameter->toString().c_str();
        }
        return OpenViBE::CString("");
    }

    void setValue(const OpenViBE::CString& value) {
        if (m_parameter != nullptr) {
            m_parameter->setValue(value.toASCIIString());
        }
    }

    OpenViBE::CString m_DefaultValue;
    std::unique_ptr<OpenViBE::Parameters::ParameterTraits> m_parameter = nullptr;
    bool m_Mod = false;
};


namespace OpenViBE {
namespace Kernel {
template <class T>
class TBox : public TAttributable<TKernelObject<T>>
{
public:

    explicit TBox(const IKernelContext& ctx) : TAttributable<TKernelObject<T>>(ctx), m_identifier(CIdentifier::undefined()),
                                               m_algorithmClassID(CIdentifier::undefined())
    {
        for (auto i : { Input, Output, Setting })
        {
            m_interfacors[i]         = std::vector<std::shared_ptr<CInterfacor>>();
            m_interfacorIDToIdx[i]   = std::map<CIdentifier, size_t>();
            m_interfacorNameToIdx[i] = std::map<CString, size_t>();
        }
    }

    ~TBox() override
    {
        if (m_boxAlgorithmDesc && m_boxListener)
        {
            CBoxListenerContext ctx(this->getKernelContext(), *this, 0xffffffff);
            m_boxListener->uninitialize(ctx);
            m_boxAlgorithmDesc->releaseBoxListener(m_boxListener);
        }
    }

    void setOwnerScenario(IScenario* ownerScenario) { m_ownerScenario = ownerScenario; }
    CIdentifier getIdentifier() const override { return m_identifier; }
    CString getName() const override { return m_name; }
    CIdentifier getAlgorithmClassIdentifier() const override { return m_algorithmClassID; }

    void setListenerCallback(const std::function<void(EBoxInterfacorType, int)>& callback) override 
    {
        if(m_boxListener)
        {
            m_boxListener->setCallback(callback);
        } else {
            //box listener not initialized, try to set it 
            if (m_boxAlgorithmDesc) {
                m_boxListener = m_boxAlgorithmDesc->createBoxListener();
                if (m_boxListener) {
                    CBoxListenerContext context(this->getKernelContext(), *this, 0xffffffff);
                    m_boxListener->initialize(context);
                    m_boxListener->setCallback(callback);
                }
            }
        }
    }

    bool setIdentifier(const CIdentifier& identifier) override
    {
        OV_ERROR_UNLESS_KRF(m_identifier == CIdentifier::undefined(), "Trying to overwrite an already set indentifier", Kernel::ErrorType::BadCall);
        OV_ERROR_UNLESS_KRF(identifier != CIdentifier::undefined(), "Trying to set an undefined identifier", Kernel::ErrorType::BadArgument);

        m_identifier = identifier;
        this->notify(EBoxModification::IdentifierChanged);

        return true;
    }

    bool setName(const CString& name) override
    {
        m_name = name;
        this->notify(EBoxModification::NameChanged);
        return true;
    }

    bool setAlgorithmClassIdentifier(const CIdentifier& algorithmClassID) override
    {
        // We need to set the box algorithm identifier in any case. This is because OpenViBE should be able to load
        // a scenario with non-existing boxes and save it without modifying them.
        m_algorithmClassID = algorithmClassID;

        if (!(algorithmClassID == OVP_ClassId_BoxAlgorithm_Metabox || this
                                                                      ->getKernelContext().getPluginManager().canCreatePluginObject(algorithmClassID)))
        {
            // OV_WARNING_K("Box algorithm descriptor not found " << algorithmClassID.str());
            return true;
        }

        if (m_boxAlgorithmDesc && m_boxListener)
        {
            CBoxListenerContext context(this->getKernelContext(), *this, 0xffffffff);
            m_boxListener->uninitialize(context);
            m_boxAlgorithmDesc->releaseBoxListener(m_boxListener);
        }

        const Plugins::IPluginObjectDesc* pluginObjectDesc = this->getKernelContext().getPluginManager().getPluginObjectDescCreating(algorithmClassID);
        m_boxAlgorithmDesc                                 = dynamic_cast<const Plugins::IBoxAlgorithmDesc*>(pluginObjectDesc);

        if (m_boxAlgorithmDesc)
        {
            if(m_boxListener) {
                delete m_boxListener;
            }
            m_boxListener = m_boxAlgorithmDesc->createBoxListener();
            if (m_boxListener)
            {
                CBoxListenerContext context(this->getKernelContext(), *this, 0xffffffff);
                m_boxListener->initialize(context);
            }
        }

        if (algorithmClassID != OVP_ClassId_BoxAlgorithm_Metabox)
        {
            //We use the neutralized version of CBoxProto to just initialize the stream restriction mecanism
            CBoxProtoRestriction oTempProto(this->getKernelContext(), *this);
            m_boxAlgorithmDesc->getBoxPrototype(oTempProto);
        }

        this->notify(EBoxModification::AlgorithmClassIdentifierChanged);

        return true;
    }

    bool initializeFromAlgorithmClassIdentifier(const CIdentifier& algorithmClassID) override
    {
        if (!this->initializeFromAlgorithmClassIdentifierNoInit(algorithmClassID)) { return false; }

        this->notify(EBoxModification::Initialized);
        this->notify(EBoxModification::DefaultInitialized);
        return true;
    }

    bool initializeFromAlgorithmClassIdentifierNoInit(const CIdentifier& algorithmClassID)
    {
        this->disableNotification();

        const Plugins::IBoxAlgorithmDesc* desc = dynamic_cast<const Plugins::IBoxAlgorithmDesc*>(this
                                                                                                 ->getKernelContext().getPluginManager().
                                                                                                 getPluginObjectDescCreating(algorithmClassID));
        if (!desc)
        {
            this->enableNotification();
            OV_ERROR_KRF("Algorithm descriptor not found " << algorithmClassID.str(), Kernel::ErrorType::ResourceNotFound);
        }

        this->clearBox();
        this->setName(desc->getName());
        this->setAlgorithmClassIdentifier(algorithmClassID);

        CBoxProto proto(this->getKernelContext(), *this);
        desc->getBoxPrototype(proto);

        if (this->hasAttribute(OV_AttributeId_Box_InitialPrototypeHashValue))
        {
            this->setAttributeValue(
                OV_AttributeId_Box_InitialPrototypeHashValue, this->getPluginManager().getPluginObjectHashValue(algorithmClassID).toString());
        }
        else
        {
            this->addAttribute(
                OV_AttributeId_Box_InitialPrototypeHashValue, this->getPluginManager().getPluginObjectHashValue(algorithmClassID).toString());
        }

        this->enableNotification();

        return true;
    }

    bool initializeFromBoxAlgorithmDesc(const Plugins::IBoxAlgorithmDesc& boxAlgorithmDesc)
    {
        this->clearBox();
        this->setName(boxAlgorithmDesc.getName());
        this->setAlgorithmClassIdentifier(boxAlgorithmDesc.getCreatedClassIdentifier());

        CBoxProto boxProto(this->getKernelContext(), *this);
        boxAlgorithmDesc.getBoxPrototype(boxProto);

        if (this->hasAttribute(OV_AttributeId_Box_InitialPrototypeHashValue))
        {
            this->setAttributeValue(
                OV_AttributeId_Box_InitialPrototypeHashValue, this->getPluginManager().getPluginObjectHashValue(boxAlgorithmDesc).toString());
        }
        else
        {
            this->addAttribute(
                OV_AttributeId_Box_InitialPrototypeHashValue, this->getPluginManager().getPluginObjectHashValue(boxAlgorithmDesc).toString());
        }

        this->enableNotification();

        this->notify(EBoxModification::Initialized);
        this->notify(EBoxModification::DefaultInitialized);
        return true;
    }

    bool initializeFromExistingBox(const IBox& rExistingBox) override
    {
        this->disableNotification();
        m_isObserverNotificationActive = false;

        this->clearBox();
        this->setName(rExistingBox.getName());
        this->setAlgorithmClassIdentifier(rExistingBox.getAlgorithmClassIdentifier());

        for (const auto interfacorType : { Input, Output, Setting })
        {
            for (size_t i = 0; i < rExistingBox.getInterfacorCountIncludingDeprecated(interfacorType); ++i)
            {
                CIdentifier identifier = CIdentifier::undefined();
                CIdentifier type       = CIdentifier::undefined();
                CString name;
                bool isDeprecated;
                rExistingBox.getInterfacorIdentifier(interfacorType, i, identifier);
                rExistingBox.getInterfacorType(interfacorType, i, type);
                rExistingBox.getInterfacorName(interfacorType, i, name);
                rExistingBox.getInterfacorDeprecatedStatus(interfacorType, i, isDeprecated);
                if (interfacorType == Input) {
                    this->addInput(name, type, identifier, true);
                }
                else if (interfacorType == Output) {
                    this->addOutput(name, type, identifier, true);
                } else if (interfacorType == Setting) {
                    CString value;
                    CString defaultValue;
                    bool isModifiable;
                    rExistingBox.getSettingValue(i, value);
                    rExistingBox.getSettingDefaultValue(i, defaultValue);
                    rExistingBox.getSettingMod(i, isModifiable);
                    this->addSetting(name, type, defaultValue, i, isModifiable, identifier, true);
                    this->setSettingValue(i, value);
                    if (isModifiable) { m_modifiableSettingIndexes.push_back(i); }
                }

                this->setInterfacorDeprecatedStatus(interfacorType, i, isDeprecated);
            }
        }

        CIdentifier id = rExistingBox.getNextAttributeIdentifier(CIdentifier::undefined());
        while (id != CIdentifier::undefined())
        {
            this->addAttribute(id, rExistingBox.getAttributeValue(id));
            id = rExistingBox.getNextAttributeIdentifier(id);
        }

        CIdentifier streamTypeID = CIdentifier::undefined();
        while ((streamTypeID = this->getKernelContext().getTypeManager().getNextTypeIdentifier(streamTypeID)) !=
               CIdentifier::undefined())
        {
            if (this->getKernelContext().getTypeManager().isStream(streamTypeID))
            {
                //First check if it is a stream
                if (rExistingBox.hasInputSupport(streamTypeID)) { this->addInputSupport(streamTypeID); }
                if (rExistingBox.hasOutputSupport(streamTypeID)) { this->addOutputSupport(streamTypeID); }
            }
        }

        this->enableNotification();

        this->notify(EBoxModification::Initialized);

        return true;
    }

    //___________________________________________________________________//
    //                                                                   //
    bool addInterfacor(const EBoxInterfacorType interfacorType, const CString& newName, const CIdentifier& typeID, const CIdentifier& identifier,
                       const bool notify) override
    {
        switch (interfacorType)
        {
            case Input:
            case Output:
                OV_ERROR_UNLESS_KRF(this->getTypeManager().isStream(typeID),
                                    "While adding " << INTERFACOR_TYPE_TO_NAME.at(interfacorType) << " '" << newName << "' to box '" << this->getName()
                                    << "', unknown stream type identifier " << typeID.str(),
                                    ErrorType::BadArgument);
                break;
            case Setting: break;
            default: break;
        }

        const size_t position = size_t(m_interfacors[interfacorType].size());
        switch (interfacorType)
        {
            case Input:
            case Output:
                m_interfacors[interfacorType].push_back(std::make_shared<CInputOutput>(newName, typeID, identifier));
                break;
            case Setting:
                // addSetting will deal with everything below for the setting type, and is more clever about handling enum types.
                return this->addSetting(newName, typeID, "", size_t(-1), false, identifier, true);
            default: break;
        }

        if (identifier != CIdentifier::undefined()) { m_interfacorIDToIdx[interfacorType][identifier] = position; }

        const CString uniqueName = this->getUnusedName(m_interfacorNameToIdx[interfacorType], newName);

        m_interfacors[interfacorType][position]->setName(uniqueName);
        m_interfacorNameToIdx[interfacorType][uniqueName] = position;

        OV_ERROR_UNLESS_KRF(m_interfacorNameToIdx[interfacorType].size() == m_interfacors[interfacorType].size(),
                            "Box " << m_name << " has corrupted name map storage", Kernel::ErrorType::BadResourceCreation);

        if (notify)
        {
            auto newCount = this->getInterfacorCount(interfacorType) - 1;
            switch (interfacorType)
            {
                case Input:
                    this->notify(EBoxModification::InputAdded, newCount);
                    break;
                case Output:
                    this->notify(EBoxModification::OutputAdded, newCount);
                    break;
                case Setting:
                    this->notify(EBoxModification::SettingAdded, newCount);
                    break;
                default: break;
            }
        }
        return true;
    }

    bool removeInterfacor(const EBoxInterfacorType interfacorType, const size_t index, const bool shouldNotify = true) override
    {
        switch (interfacorType)
        {
            case Input: return this->removeInput(index, shouldNotify);
            case Output: return this->removeOutput(index, shouldNotify);
            case Setting: return this->removeSetting(index, shouldNotify);
            default: return false;
        }
    }

    size_t getInterfacorCount(const EBoxInterfacorType interfacorType) const override
    {
        auto interfacors = m_interfacors.at(interfacorType);
        return size_t(std::count_if(interfacors.begin(), interfacors.end(), [](const std::shared_ptr<CInterfacor>& i) { return !i->m_Deprecated; }));
    }

    size_t getInterfacorCountIncludingDeprecated(const EBoxInterfacorType interfacorType) const override { return m_interfacors.at(interfacorType).size(); }

    bool getInterfacorIdentifier(const EBoxInterfacorType interfacorType, const size_t index, CIdentifier& identifier) const override
    {
        identifier = CIdentifier::undefined();
        OV_ERROR_UNLESS_KRF(index < m_interfacors.at(interfacorType).size(),
                            INTERFACOR_TYPE_TO_NAME.at(interfacorType) << " index = [" << index << "] is out of range (max index = [" << m_interfacors.
                            at(
                                interfacorType).size() - 1 << "])",
                            ErrorType::OutOfBound);

        identifier = m_interfacors.at(interfacorType)[index]->m_ID;
        return true;
    }

    bool getInterfacorIndex(const EBoxInterfacorType interfacorType, const CIdentifier& identifier, size_t& index) const override
    {
        index         = size_t(-1);
        const auto it = m_interfacorIDToIdx.at(interfacorType).find(identifier);
        OV_ERROR_UNLESS_KRF(it != m_interfacorIDToIdx.at(interfacorType).end(),
                            "Failed to find " << INTERFACOR_TYPE_TO_NAME.at(interfacorType) << " with identifier " << identifier.str(),
                            ErrorType::ResourceNotFound);

        index = it->second;
        return true;
    }

    bool getInterfacorIndex(const EBoxInterfacorType interfacorType, const CString& name, size_t& index) const override
    {
        index         = size_t(-1);
        const auto it = m_interfacorNameToIdx.at(interfacorType).find(name);
        OV_ERROR_UNLESS_KRF(it != m_interfacorNameToIdx.at(interfacorType).end(),
                            "Failed to find " << INTERFACOR_TYPE_TO_NAME.at(interfacorType) << " with name " << name, Kernel::ErrorType::ResourceNotFound);

        index = it->second;
        return true;
    }

    bool getInterfacorType(const EBoxInterfacorType interfacorType, const size_t index, CIdentifier& typeID) const override
    {
        OV_ERROR_UNLESS_KRF(index < m_interfacors.at(interfacorType).size(),
                            INTERFACOR_TYPE_TO_NAME.at(interfacorType) << " index = [" << index << "] is out of range (max index = ["
                            << m_interfacors.at(interfacorType).size() - 1 << "])", Kernel::ErrorType::OutOfBound);

        typeID = m_interfacors.at(interfacorType)[index]->m_TypeID;
        return true;
    }

    bool getInterfacorType(const EBoxInterfacorType interfacorType, const CIdentifier& identifier, CIdentifier& typeID) const override
    {
        const auto it = m_interfacorIDToIdx.at(interfacorType).find(identifier);
        OV_ERROR_UNLESS_KRF(it != m_interfacorIDToIdx.at(interfacorType).end(),
                            "Failed to find " << INTERFACOR_TYPE_TO_NAME.at(interfacorType) << " with id " << identifier.str(),
                            ErrorType::ResourceNotFound);

        return this->getInterfacorType(interfacorType, it->second, typeID);
    }

    bool getInterfacorType(const EBoxInterfacorType interfacorType, const CString& name, CIdentifier& typeID) const override
    {
        const auto it = m_interfacorNameToIdx.at(interfacorType).find(name);
        OV_ERROR_UNLESS_KRF(it != m_interfacorNameToIdx.at(interfacorType).end(),
                            "Failed to find " << INTERFACOR_TYPE_TO_NAME.at(interfacorType) << " with name " << name, Kernel::ErrorType::ResourceNotFound);

        return this->getInterfacorType(interfacorType, it->second, typeID);
    }


    bool getInterfacorName(const EBoxInterfacorType interfacorType, const size_t index, CString& name) const override
    {
        OV_ERROR_UNLESS_KRF(index < m_interfacors.at(interfacorType).size(),
                            INTERFACOR_TYPE_TO_NAME.at(interfacorType) << " index = [" << index << "] is out of range (max index = [" << m_interfacors.
                            at(
                                interfacorType).size() - 1 << "])",
                            ErrorType::OutOfBound);

        name = m_interfacors.at(interfacorType)[index]->m_Name;
        return true;
    }

    bool getInterfacorName(const EBoxInterfacorType interfacorType, const CIdentifier& identifier, CString& name) const override
    {
        const auto it = m_interfacorIDToIdx.at(interfacorType).find(identifier);
        OV_ERROR_UNLESS_KRF(it != m_interfacorIDToIdx.at(interfacorType).end(),
                            "Failed to find " << INTERFACOR_TYPE_TO_NAME.at(interfacorType) << " with id " << identifier.str(),
                            ErrorType::ResourceNotFound);
        if (interfacorType == Input) {
            return this->getInputName(it->second, name);
        }
        else if (interfacorType == Output) {
            return this->getOutputName(it->second, name);
        }
        else if (interfacorType == Setting) {
            return this->getSettingName(it->second, name);
        }
        return false;
    }

    bool getInterfacorDeprecatedStatus(const EBoxInterfacorType interfacorType, const size_t index, bool& value) const override
    {
        if (index >= m_interfacors.at(interfacorType).size()) { OV_WARNING_K("DUH"); }
        OV_ERROR_UNLESS_KRF(index < m_interfacors.at(interfacorType).size(),
                            INTERFACOR_TYPE_TO_NAME.at(interfacorType) << " index = [" << index << "] is out of range (max index = [" << m_interfacors.
                            at(
                                interfacorType).size() - 1 << "])",
                            ErrorType::OutOfBound);

        value = m_interfacors.at(interfacorType)[index]->m_Deprecated;
        return true;
    }

    bool getInterfacorDeprecatedStatus(const EBoxInterfacorType interfacorType, const CIdentifier& identifier, bool& value) const override
    {
        const auto it = m_interfacorIDToIdx.at(interfacorType).find(identifier);
        OV_ERROR_UNLESS_KRF(it != m_interfacorIDToIdx.at(interfacorType).end(),
                            "Failed to find " << INTERFACOR_TYPE_TO_NAME.at(interfacorType) << " with id " << identifier.str(),
                            ErrorType::ResourceNotFound);

        return this->getInterfacorDeprecatedStatus(interfacorType, it->second, value);
    }

    bool hasInterfacorWithIdentifier(const EBoxInterfacorType interfacorType, const CIdentifier& identifier) const override
    {
        return m_interfacorIDToIdx.at(interfacorType).find(identifier) != m_interfacorIDToIdx.at(interfacorType).end();
    }

    bool hasInterfacorWithNameAndType(const EBoxInterfacorType interfacorType, const CString& name, const CIdentifier& /*typeID*/) const override
    {
        return m_interfacorNameToIdx.at(interfacorType).find(name) != m_interfacorNameToIdx.at(interfacorType).end();
    }

    bool hasInterfacorWithType(const EBoxInterfacorType interfacorType, const size_t index, const CIdentifier& typeID) const override
    {
        if (index < this->getInterfacorCount(interfacorType))
        {
            CIdentifier type = CIdentifier::undefined();
            this->getInterfacorType(interfacorType, index, type);
            return (type == typeID);
        }
        return false;
    }


    bool setInterfacorType(const EBoxInterfacorType interfacorType, const size_t index, const CIdentifier& typeID) override
    {
        switch (interfacorType)
        {
            case Input:
            case Output:
                OV_ERROR_UNLESS_KRF(this->getTypeManager().isStream(typeID),
                                    "While changing box '" << this->getName() << "' " << INTERFACOR_TYPE_TO_NAME.at(interfacorType) <<
                                    " type, unknown stream type identifier " << typeID.str(),
                                    ErrorType::BadArgument);
                break;
            case Setting:
                // return false; settings cannot be changed
                OV_ERROR_KRF("While changing box '" << this->getName() << "' " << INTERFACOR_TYPE_TO_NAME.at(interfacorType) <<
                         " type: Setting type cannot be changed anymore. Please remove the setting and create a new one with desired type.",
                         ErrorType::BadArgument);
                break;
        }

        OV_ERROR_UNLESS_KRF(index < m_interfacors.at(interfacorType).size(),
                            INTERFACOR_TYPE_TO_NAME.at(interfacorType) << " index = [" << index << "] is out of range (max index = [" << m_interfacors.
                            at(
                                interfacorType).size() - 1 << "])",
                            ErrorType::OutOfBound);

        if (m_interfacors[interfacorType][index]->m_TypeID == typeID) { return true; }

        m_interfacors[interfacorType][index]->m_TypeID = typeID;

        switch (interfacorType)
        {
            case Input:
                this->notify(EBoxModification::InputTypeChanged, index);
                break;
            case Output:
                this->notify(EBoxModification::OutputTypeChanged, index);
                break;
            case Setting:
                this->notify(EBoxModification::SettingTypeChanged, index);
                this->notifySettingChange(SettingChange, int(index));
                break;
        }

        return true;
    }

    bool setInterfacorType(const EBoxInterfacorType interfacorType, const CIdentifier& identifier, const CIdentifier& typeID) override
    {
        const auto it = m_interfacorIDToIdx[interfacorType].find(identifier);
        OV_ERROR_UNLESS_KRF(it != m_interfacorIDToIdx[interfacorType].end(),
                            "Failed to find " << INTERFACOR_TYPE_TO_NAME.at(interfacorType) << " with id " << identifier.str(),
                            ErrorType::ResourceNotFound);

        return this->setInterfacorType(interfacorType, it->second, typeID);
    }

    bool setInterfacorType(const EBoxInterfacorType interfacorType, const CString& name, const CIdentifier& typeID) override
    {
        const auto it = m_interfacorNameToIdx[interfacorType].find(name);
        OV_ERROR_UNLESS_KRF(it != m_interfacorNameToIdx[interfacorType].end(),
                            "Failed to find " << INTERFACOR_TYPE_TO_NAME.at(interfacorType) << " with name " << name, Kernel::ErrorType::ResourceNotFound);

        return this->setInterfacorType(interfacorType, it->second, typeID);
    }


    bool setInterfacorName(const EBoxInterfacorType interfacorType, const size_t index, const CString& newName) override
    {
        OV_ERROR_UNLESS_KRF(index < m_interfacors[interfacorType].size(),
                            INTERFACOR_TYPE_TO_NAME.at(interfacorType) << " index = [" << index << "] is out of range (max index = [" << m_interfacors[
                                interfacorType].size() - 1 << "])",
                            ErrorType::OutOfBound);

        if (m_interfacors[interfacorType][index]->m_Name == newName)
        {
            // no change, don't bother notifying
            return true;
        }

        // remove entry from name key map
        const auto it = m_interfacorNameToIdx[interfacorType].find(m_interfacors[interfacorType][index]->m_Name);
        OV_ERROR_UNLESS_KRF(it != m_interfacorNameToIdx[interfacorType].end(),
                            "Failed to find " << INTERFACOR_TYPE_TO_NAME.at(interfacorType) << " with name " << m_interfacors[interfacorType][index]->
                            m_Name,
                            ErrorType::ResourceNotFound);
        m_interfacorNameToIdx[interfacorType].erase(it);

        // check for duplicated name key and update if necessary
        const CString uniqueName                          = this->getUnusedName(m_interfacorNameToIdx[interfacorType], newName);
        m_interfacorNameToIdx[interfacorType][uniqueName] = index;
        m_interfacors[interfacorType][index]->setName(uniqueName);

        OV_ERROR_UNLESS_KRF(m_interfacorNameToIdx[interfacorType].size() == m_interfacors[interfacorType].size(),
                            "Box " << m_name << " has corrupted name map storage", Kernel::ErrorType::BadResourceCreation);

        switch (interfacorType)
        {
            case Input:
                this->notify(EBoxModification::InputNameChanged, index);
                break;
            case Output:
                this->notify(EBoxModification::OutputNameChanged, index);
                break;
            case Setting:
                this->notify(EBoxModification::SettingNameChanged, index);
                break;
        }

        return true;
    }

    bool setInterfacorName(const EBoxInterfacorType interfacorType, const CIdentifier& identifier, const CString& newName) override
    {
        const auto it = m_interfacorIDToIdx[interfacorType].find(identifier);
        OV_ERROR_UNLESS_KRF(it != m_interfacorIDToIdx[interfacorType].end(),
                            "Failed to find " << INTERFACOR_TYPE_TO_NAME.at(interfacorType) << " with id " << identifier.str(),
                            ErrorType::ResourceNotFound);

        return this->setInterfacorName(interfacorType, it->second, newName);
    }

    bool setInterfacorDeprecatedStatus(const EBoxInterfacorType interfacorType, const size_t index, const bool newValue) override
    {
        OV_ERROR_UNLESS_KRF(index < m_interfacors[interfacorType].size(),
                            INTERFACOR_TYPE_TO_NAME.at(interfacorType) << " index = [" << index << "] is out of range (max index = [" << m_interfacors[
                                interfacorType].size() - 1 << "])",
                            ErrorType::OutOfBound);

        m_interfacors[interfacorType][index]->m_Deprecated = newValue;

        return true;
    }

    bool setInterfacorDeprecatedStatus(const EBoxInterfacorType interfacorType, const CIdentifier& identifier, const bool newValue) override
    {
        const auto it = m_interfacorIDToIdx[interfacorType].find(identifier);
        OV_ERROR_UNLESS_KRF(it != m_interfacorIDToIdx[interfacorType].end(),
                            "Failed to find " << INTERFACOR_TYPE_TO_NAME.at(interfacorType) << "  with id " << identifier.str(),
                            ErrorType::ResourceNotFound);

        return this->setInterfacorDeprecatedStatus(interfacorType, it->second, newValue);
    }

    //___________________________________________________________________//
    //                                                                   //

    bool addInput(const CString& name, const CIdentifier& typeID, const CIdentifier& identifier, const bool notify) override
    {
        return this->addInterfacor(Input, name, typeID, identifier, notify);
    }

    bool removeInput(const size_t index, const bool notify = true) override
    {
        OV_ERROR_UNLESS_KRF(index < m_interfacors[Input].size(),
                            "Input index = [" << index << "] is out of range (max index = [" << m_interfacors[Input].size() - 1 << "])",
                            ErrorType::OutOfBound);
        {
            CIdentifier* listID = nullptr;
            size_t nbElems      = 0;
            m_ownerScenario->getLinkIdentifierToBoxInputList(m_identifier, index, &listID, &nbElems);
            for (size_t i = 0; i < nbElems; ++i) { m_ownerScenario->disconnect(listID[i]); }
            m_ownerScenario->releaseIdentifierList(listID);
        }

        // $$$
        // The way the links are removed here
        // is not correct because they are all
        // collected and then all removed. In case
        // the box listener callback on box removal,
        // the nextcoming links would potentially be
        // invalid
        std::vector<std::pair<std::pair<uint64_t, size_t>, std::pair<uint64_t, size_t>>> links;

        {
            CIdentifier* listID = nullptr;
            size_t nbElems      = 0;
            m_ownerScenario->getLinkIdentifierToBoxList(m_identifier, &listID, &nbElems);
            for (size_t i = 0; i < nbElems; ++i)
            {
                CIdentifier id = listID[i];
                ILink* link    = m_ownerScenario->getLinkDetails(id);
                if (link->getTargetBoxInputIndex() > index)
                {
                    links.push_back(std::make_pair(std::make_pair(link->getSourceBoxIdentifier().id(), link->getSourceBoxOutputIndex()),
                                                   std::make_pair(link->getTargetBoxIdentifier().id(), link->getTargetBoxInputIndex())));

                    if (m_ownerScenario->isLink(id)) { m_ownerScenario->disconnect(id); }
                }
            }
            m_ownerScenario->releaseIdentifierList(listID);
        }

        // This reorganizes the parent's scenario links if this box is not actually a scenario itself
        if (m_identifier != CIdentifier::undefined())
        {
            std::vector<std::pair<size_t, std::pair<uint64_t, size_t>>> scenarioLinks;
            for (size_t scenarioInputIdx = 0; scenarioInputIdx < m_ownerScenario->getInterfacorCount(Input); ++scenarioInputIdx)
            {
                CIdentifier boxID      = CIdentifier::undefined();
                size_t boxConnectorIdx = size_t(-1);
                m_ownerScenario->getScenarioInputLink(scenarioInputIdx, boxID, boxConnectorIdx);
                if (boxID == m_identifier)
                {
                    if (boxConnectorIdx > index) { scenarioLinks.push_back(std::make_pair(scenarioInputIdx, std::make_pair(boxID.id(), boxConnectorIdx))); }
                    if (boxConnectorIdx >= index) { m_ownerScenario->removeScenarioInputLink(scenarioInputIdx, boxID, boxConnectorIdx); }
                }
            }

            // Reconnects scenario links
            for (const auto& link : scenarioLinks) { m_ownerScenario->setScenarioInputLink(link.first, link.second.first, link.second.second - 1); }
        }

        const CIdentifier toBeRemovedId = m_interfacors[Input][index]->m_ID;
        CString toBeRemovedName         = m_interfacors[Input][index]->m_Name;

        // Erases actual input
        m_interfacors[Input].erase(m_interfacors[Input].begin() + index);

        // Reconnects box links
        for (const auto& link : links)
        {
            CIdentifier newId = CIdentifier::undefined();
            m_ownerScenario->connect(newId, link.first.first, link.first.second, link.second.first, link.second.second - 1, CIdentifier::undefined());
        }

        // erase name key
        const auto itName = m_interfacorNameToIdx[Input].find(toBeRemovedName);
        OV_ERROR_UNLESS_KRF(itName != m_interfacorNameToIdx[Input].end(), "No input found with name " << toBeRemovedName,
                            ErrorType::ResourceNotFound);
        m_interfacorNameToIdx[Input].erase(itName);

        // erase identifier key if defined
        if (toBeRemovedId != CIdentifier::undefined())
        {
            const auto itIdent = m_interfacorIDToIdx[Input].find(toBeRemovedId);
            OV_ERROR_UNLESS_KRF(itIdent != m_interfacorIDToIdx[Input].end(), "No input found with id " << toBeRemovedId.str(),
                                ErrorType::ResourceNotFound);
            m_interfacorIDToIdx[Input].erase(itIdent);
        }

        if (notify) { this->notify(EBoxModification::InputRemoved, index); }

        return true;
    }

    size_t getInputCount() const override { return this->getInterfacorCount(Input); }
    bool getInputType(const size_t index, CIdentifier& typeID) const override { return this->getInterfacorType(Input, index, typeID); }
    bool getInputName(const size_t index, CString& name) const override { return this->getInterfacorName(Input, index, name); }

    bool getInputName(const CIdentifier& rInputIdentifier, CString& name) const { return this->getInterfacorName(Input, rInputIdentifier, name); }

    bool setInputType(const size_t index, const CIdentifier& typeID) override { return this->setInterfacorType(Input, index, typeID); }
    bool setInputName(const size_t index, const CString& name) override { return this->setInterfacorName(Input, index, name); }

    //___________________________________________________________________//
    //                                                                   //

    bool addOutput(const CString& name, const CIdentifier& typeID, const CIdentifier& identifier, const bool notify) override
    {
        return this->addInterfacor(Output, name, typeID, identifier, notify);
    }

    bool removeOutput(const size_t index, const bool notify = true) override
    {
        OV_ERROR_UNLESS_KRF(index < m_interfacors[Output].size(),
                            "Output index = [" << index << "] is out of range (max index = [" << size_t(m_interfacors[Output].size() - 1) << "])",
                            ErrorType::OutOfBound);

        std::vector<std::pair<std::pair<uint64_t, size_t>, std::pair<uint64_t, size_t>>> links;

        if (m_ownerScenario)
        {
            CIdentifier* listID = nullptr;
            size_t nbElems      = 0;

            m_ownerScenario->getLinkIdentifierFromBoxOutputList(m_identifier, index, &listID, &nbElems);
            for (size_t i = 0; i < nbElems; ++i) { m_ownerScenario->disconnect(listID[i]); }
            m_ownerScenario->releaseIdentifierList(listID);

            // $$$
            // The way the links are removed here is not correct because they are all collected and then all removed. In case
            // the box listener callback on box removal, the nextcoming links would potentially be invalid
            {
                listID  = nullptr;
                nbElems = 0;
                m_ownerScenario->getLinkIdentifierFromBoxOutputList(m_identifier, index, &listID, &nbElems);
                for (size_t i = 0; i < nbElems; ++i)
                {
                    const CIdentifier& curID = listID[i];
                    ILink* link              = m_ownerScenario->getLinkDetails(curID);
                    if (link->getSourceBoxOutputIndex() > index)
                    {
                        links.push_back(std::make_pair(std::make_pair(link->getSourceBoxIdentifier().id(), link->getSourceBoxOutputIndex()),
                                                       std::make_pair(link->getTargetBoxIdentifier().id(), link->getTargetBoxInputIndex())));
                        if (m_ownerScenario->isLink(curID)) { m_ownerScenario->disconnect(curID); }
                    }
                }
                m_ownerScenario->releaseIdentifierList(listID);
            }

            // This reorganizes the parent's scenario links if this box is not actually a scenario
            if (m_identifier != CIdentifier::undefined())
            {
                std::vector<std::pair<size_t, std::pair<uint64_t, size_t>>> scenarioLinks;
                for (size_t scenarioOutputIdx = 0; scenarioOutputIdx < m_ownerScenario->getOutputCount(); ++scenarioOutputIdx)
                {
                    CIdentifier boxID      = CIdentifier::undefined();
                    size_t boxConnectorIdx = size_t(-1);
                    m_ownerScenario->getScenarioOutputLink(scenarioOutputIdx, boxID, boxConnectorIdx);
                    if (boxID == m_identifier)
                    {
                        if (boxConnectorIdx > index)
                        {
                            scenarioLinks.push_back(std::make_pair(scenarioOutputIdx, std::make_pair(boxID.id(), boxConnectorIdx)));
                        }
                        if (boxConnectorIdx >= index) { m_ownerScenario->removeScenarioOutputLink(scenarioOutputIdx, boxID, boxConnectorIdx); }
                    }
                }

                // Reconnects scenario links
                for (const auto& link : scenarioLinks) { m_ownerScenario->setScenarioOutputLink(link.first, link.second.first, link.second.second - 1); }
            }
        }
        const CIdentifier toBeRemovedId = m_interfacors.at(Output)[index]->m_ID;
        CString toBeRemovedName         = m_interfacors.at(Output)[index]->m_Name;

        // Erases actual output
        m_interfacors[Output].erase(m_interfacors.at(Output).begin() + index);

        // Reconnects box links
        if (m_ownerScenario)
        {
            for (const auto& link : links)
            {
                CIdentifier newId = CIdentifier::undefined();
                m_ownerScenario->connect(newId, link.first.first, link.first.second - 1, link.second.first, link.second.second,
                                         CIdentifier::undefined());
            }
        }


        // erase name key
        const auto itName = m_interfacorNameToIdx.at(Output).find(toBeRemovedName);
        OV_ERROR_UNLESS_KRF(itName != m_interfacorNameToIdx.at(Output).end(), "No output found with name " << toBeRemovedName,
                            ErrorType::ResourceNotFound);
        m_interfacorNameToIdx.at(Output).erase(itName);

        // erase identifier key if defined
        if (toBeRemovedId != CIdentifier::undefined())
        {
            const auto itIdent = m_interfacorIDToIdx.at(Output).find(toBeRemovedId);
            OV_ERROR_UNLESS_KRF(itIdent != m_interfacorIDToIdx.at(Output).end(), "No output found with id " << toBeRemovedId.str(),
                                ErrorType::ResourceNotFound);

            m_interfacorIDToIdx.at(Output).erase(itIdent);
        }

        if (notify) { this->notify(EBoxModification::OutputRemoved, index); }

        return true;
    }

    size_t getOutputCount() const override { return this->getInterfacorCount(Output); }
    bool getOutputType(const size_t index, CIdentifier& typeID) const override { return this->getInterfacorType(Output, index, typeID); }
    bool getOutputName(const size_t index, CString& name) const override { return this->getInterfacorName(Output, index, name); }
    bool setOutputType(const size_t index, const CIdentifier& typeID) override { return this->setInterfacorType(Output, index, typeID); }
    bool setOutputName(const size_t index, const CString& name) override { return this->setInterfacorName(Output, index, name); }

    bool addInterfacorTypeSupport(const EBoxInterfacorType interfacorType, const CIdentifier& typeID) override
    {
        if (interfacorType == Input) { m_supportInputTypes.push_back(typeID); }
        else if (interfacorType == Output) { m_supportOutputTypes.push_back(typeID); }

        return false;
    }

    bool hasInterfacorTypeSupport(const EBoxInterfacorType interfacorType, const CIdentifier& typeID) const override
    {
        if (interfacorType == Input)
        {
            if (m_supportInputTypes.empty()) { return true; }

            for (size_t i = 0; i < m_supportInputTypes.size(); ++i) { if (m_supportInputTypes[i] == typeID) { return true; } }
        }
        else if (interfacorType == Output)
        {
            //If there is no type specify, we allow all
            if (m_supportOutputTypes.empty()) { return true; }

            for (size_t i = 0; i < m_supportOutputTypes.size(); ++i) { if (m_supportOutputTypes[i] == typeID) { return true; } }
        }
        else
        {
            // Settings are always all supported, in a way
            return true;
        }

        return false;
    }

    bool addInputSupport(const CIdentifier& typeID) override { return this->addInterfacorTypeSupport(Input, typeID); }
    bool hasInputSupport(const CIdentifier& typeID) const override { return this->hasInterfacorTypeSupport(Input, typeID); }
    bool addOutputSupport(const CIdentifier& typeID) override { return this->addInterfacorTypeSupport(Output, typeID); }
    bool hasOutputSupport(const CIdentifier& typeID) const override { return this->hasInterfacorTypeSupport(Output, typeID); }

    bool setSupportTypeFromAlgorithmIdentifier(const CIdentifier& typeID) override
    {
        const Plugins::IPluginObjectDesc* pluginObjectDesc = this->getKernelContext().getPluginManager().getPluginObjectDescCreating(typeID);
        const Plugins::IBoxAlgorithmDesc* boxAlgorithmDesc = dynamic_cast<const Plugins::IBoxAlgorithmDesc*>(pluginObjectDesc);

        OV_ERROR_UNLESS_KRF(boxAlgorithmDesc, "Tried to initialize with an unregistered algorithm", Kernel::ErrorType::Internal);

        //We use the neutralized version of CBoxProto to just initialize the stream restriction mecanism
        CBoxProtoRestriction oTempProto(this->getKernelContext(), *this);
        boxAlgorithmDesc->getBoxPrototype(oTempProto);
        return true;
    }

    std::vector<CIdentifier> getInputSupportTypes() const override { return m_supportInputTypes; }
    std::vector<CIdentifier> getOutputSupportTypes() const override { return m_supportOutputTypes; }
    void clearOutputSupportTypes() override { m_supportOutputTypes.clear(); }
    void clearInputSupportTypes() override { m_supportInputTypes.clear(); }

private:
    CIdentifier getUnusedInterfacorIdentifier(const EBoxInterfacorType interfacorType,
                                              const CIdentifier& suggestedIdentifier = CIdentifier::undefined()) const
    {
        uint64_t identifier = CIdentifier::random().id();
        if (suggestedIdentifier != CIdentifier::undefined()) { identifier = suggestedIdentifier.id(); }

        CIdentifier resultIdentifier = CIdentifier::undefined();
        std::map<CIdentifier, size_t>::const_iterator it;
        do
        {
            resultIdentifier = CIdentifier(identifier);
            it               = m_interfacorIDToIdx.at(interfacorType).find(resultIdentifier);
            identifier++;
        } while (it != m_interfacorIDToIdx.at(interfacorType).end() || resultIdentifier == CIdentifier::undefined());
        return resultIdentifier;
    }

public:

    CIdentifier getUnusedSettingIdentifier(const CIdentifier& /*suggestedID*/ = CIdentifier::undefined()) const override
    {
        return this->getUnusedInterfacorIdentifier(Setting);
    }

    CIdentifier getUnusedInputIdentifier(const CIdentifier& /*suggestedID*/ = CIdentifier::undefined()) const override
    {
        return this->getUnusedInterfacorIdentifier(Input);
    }

    CIdentifier getUnusedOutputIdentifier(const CIdentifier& /*suggestedID*/ = CIdentifier::undefined()) const override
    {
        return this->getUnusedInterfacorIdentifier(Output);
    }

    bool addSetting(const CString& name, const CIdentifier& typeID, const CString& sDefaultValue, const size_t index,
                    const bool bModifiability, const CIdentifier& identifier, const bool notify) override
    {
        CString value(sDefaultValue);

        CSetting s;
        s.m_Name         = name;
        s.m_TypeID       = typeID;
        s.m_DefaultValue = sDefaultValue;
        s.m_Mod          = bModifiability;
        s.m_ID           = identifier;

        std::vector<std::string> availableValues;
        if (this->getTypeManager().isEnumeration(typeID))
        {
            if (this->getTypeManager().getEnumerationEntryValueFromName(typeID, sDefaultValue) == CIdentifier::undefined().id())
            {
                if (this->getTypeManager().getEnumerationEntryCount(typeID) != 0)
                {
                    // get value to the first enum entry
                    // and eventually correct this after
                    uint64_t v = 0;
                    this->getTypeManager().getEnumerationEntry(typeID, 0, value, v);

                    // Find if the default value string actually is an identifier, otherwise just keep the zero index name as default.
                    CIdentifier defaultValueID = CIdentifier::undefined();
                    defaultValueID.fromString(sDefaultValue);

                    // Finally, if it is an identifier, then a name should be found
                    // from the type manager ! Otherwise value is left to the default.
                    const CString candidateValue = this->getTypeManager().getEnumerationEntryNameFromValue(typeID, defaultValueID.id());
                    if (candidateValue != CString("")) {
                        value = candidateValue;
                    }
                }
            }

            CString enumName;
            uint64_t enumValue;
            for (size_t i = 0; i < this->getTypeManager().getEnumerationEntryCount(typeID); ++i) {
                this->getTypeManager().getEnumerationEntry(typeID, i, enumName, enumValue);
                availableValues.push_back(enumName.toASCIIString());
            }

            s.m_parameter = std::make_unique<Parameters::ParameterListString>(name.toASCIIString(), "", availableValues, value.toASCIIString());

        } else if (this->getTypeManager().isBitMask(typeID)) {
            CString entryName;
            uint64_t entryValue;
            for (size_t i = 0; i < this->getTypeManager().getBitMaskEntryCount(typeID); ++i) {
                this->getTypeManager().getBitMaskEntry(typeID, i, entryName, entryValue);
                availableValues.push_back(entryName.toASCIIString());
            }

            auto parameter = std::make_unique<Parameters::ParameterMultiSelect>(name.toASCIIString(), "", availableValues);
            // fromString() splits on OV_Value_EnumeratedStringSeparator and setValue() drops
            // entries absent from availableValues, which is exactly the bitmask composition
            // format and the validation the default value needs.
            parameter->fromString(sDefaultValue.toASCIIString());
            s.m_parameter = std::move(parameter);

        } else {
            s.m_parameter = std::unique_ptr<Parameters::ParameterTraits>(Parameters::ParameterFactory::create(Parameters::getTypeFromTypeId(typeID)));
            s.m_parameter->setName(name.toASCIIString());
            s.m_parameter->setValue(value.toASCIIString());
        }

        const size_t idx = index;
        size_t insertLocation;

        if (index == size_t(-1) || index == size_t(m_interfacors[Setting].size()))
        {
            m_interfacors[Setting].push_back(std::make_shared<CSetting>(s));
            insertLocation = (size_t(m_interfacors[Setting].size())) - 1;
        }
        else
        {
            OV_ERROR_UNLESS_KRF(index <= m_interfacors[Setting].size(),
                                "Tried to push '" << name << "' to slot " << index << " with the array size being " << m_interfacors[Setting].size(),
                                ErrorType::OutOfBound);

            auto it = m_interfacors[Setting].begin();
            it += idx;
            m_interfacors[Setting].insert(it, std::make_shared<CSetting>(s));
            insertLocation = index;

            // update all insertLocations after this insert
            for (auto it = m_interfacorIDToIdx[Setting].begin(); it != m_interfacorIDToIdx[Setting].end(); ++it) {
                if (it->second >= index) {
                    it->second += 1;
                }
            }
            for (auto it = m_interfacorNameToIdx[Setting].begin(); it != m_interfacorNameToIdx[Setting].end(); ++it) {
                if (it->second >= index) {
                    it->second += 1;
                }
            }
            for (auto it = m_modifiableSettingIndexes.begin(); it != m_modifiableSettingIndexes.end(); ++it) {
                if (*it >= index) {
                    *it += 1;
                }
            }
        }

        if (s.m_ID != CIdentifier::undefined())
        {
            // add access by CIdentifier key if defined so that size differs from m_settings
            m_interfacorIDToIdx[Setting][s.m_ID] = insertLocation;
        }
        // add access by name key (always done so that synchronized with m_settings
        const CString newName = this->getUnusedName(m_interfacorNameToIdx.at(Setting), s.m_Name);
        m_interfacors[Setting][insertLocation]->setName(newName);
        m_interfacorNameToIdx[Setting][newName] = insertLocation;

        OV_ERROR_UNLESS_KRF(m_interfacorNameToIdx.at(Setting).size() == m_interfacors[Setting].size(),
                            "Box " << m_name << " has corrupted name map storage", Kernel::ErrorType::BadResourceCreation);

        //if this setting is modifiable, keep its index
        if (bModifiability) { m_modifiableSettingIndexes.push_back(idx); }

        this->getLogManager() << LogLevel_Debug
                << "Pushed '" << m_interfacors.at(Setting)[insertLocation]->m_Name << "' : '"
                << std::static_pointer_cast<CSetting>(m_interfacors.at(Setting)[insertLocation])->getValue()
                << "' to slot " << insertLocation << " with the array size now " << m_interfacors.at(Setting).size() << "\n";

        if (notify)
        {
            this->notify(EBoxModification::SettingAdded, insertLocation);
            this->notifySettingChange(SettingAdd, int(insertLocation));
        }

        // New parameters are used in UI and bypass setters methods from this class.
        // Connect on the parameter actually stored in m_interfacors (a clone made by CSetting's copy
        // constructor above), not on the local s.m_parameter which is destroyed when this function returns.
        const CString settingName = s.m_Name;
        std::static_pointer_cast<CSetting>(m_interfacors.at(Setting)[insertLocation])->m_parameter->connect(
            [this, settingName](std::any value) {
                const size_t idx = m_interfacorNameToIdx[Setting][settingName];
                this->notify(EBoxModification::SettingValueChanged, idx);
                this->notifySettingChange(SettingValueUpdate, int(idx));
            });

        return true;
    }

    bool removeSetting(const size_t index, const bool notify = true) override
    {
        auto it = m_interfacors[Setting].begin() + index;
        OV_ERROR_UNLESS_KRF(it != m_interfacors[Setting].end(), "No setting found at index " << index, Kernel::ErrorType::ResourceNotFound);

        const CIdentifier toBeRemovedId = m_interfacors[Setting][index]->m_ID;
        CString toBeRemovedName         = m_interfacors[Setting][index]->m_Name;

        it = m_interfacors[Setting].erase(it);

        //update the modifiable setting indexes
        for (auto it2 = m_modifiableSettingIndexes.begin(); it2 != m_modifiableSettingIndexes.end();)
        {
            if (*it2 == index) {
                it2 = m_modifiableSettingIndexes.erase(it2);
            } else if (*it2 > index)
            {
                *it2 -= 1;
                ++it2;
            } else
            {
                ++it2;
            }
        }

        // Update the interfacorNameToIdx setting indexes used when accessing settings by name
        const auto itName = m_interfacorNameToIdx.at(Setting).find(toBeRemovedName);
        OV_ERROR_UNLESS_KRF(itName != m_interfacorNameToIdx.at(Setting).end(), "No setting found with name " << toBeRemovedName,
                            ErrorType::ResourceNotFound);
        OV_ERROR_UNLESS_KRF(itName->first == toBeRemovedName, "Setting name mismatch: " << itName->first << " != " << toBeRemovedName,
                            ErrorType::Internal);
        for (auto it = m_interfacorNameToIdx.at(Setting).begin(); it != m_interfacorNameToIdx.at(Setting).end();)
        {
            if (it->second == index)
            {
                it = m_interfacorNameToIdx.at(Setting).erase(it);
            }
            else if (it->second > index)
            {
                it->second -= 1;
                ++it;
            }
            else
            {
                ++it;
            }
        }

        // erase identifier key if defined
        if (toBeRemovedId != CIdentifier::undefined())
        {
            const auto itIdent = m_interfacorIDToIdx.at(Setting).find(toBeRemovedId);
            OV_ERROR_UNLESS_KRF(itIdent != m_interfacorIDToIdx.at(Setting).end(), "No setting found with id " << toBeRemovedId.str(),
                                ErrorType::ResourceNotFound);
            m_interfacorIDToIdx.at(Setting).erase(itIdent);
        }

        if (notify)
        {
            this->notify(EBoxModification::SettingRemoved, index);
            this->notifySettingChange(SettingDelete, int(index));
        }

        return true;
    }

    size_t getSettingCount() const override { return this->getInterfacorCount(Setting); }

    bool hasSettingWithName(const CString& name) const override
    {
        return m_interfacorNameToIdx.at(Setting).find(name) != m_interfacorNameToIdx.at(Setting).end();
    }

    bool getSettingType(const size_t index, CIdentifier& typeID) const override {
        return this->getInterfacorType(Setting, index, typeID);
    }

    bool getSettingName(const size_t index, CString& name) const override {
        return this->getInterfacorName(Setting, index, name);
    }

    bool getSettingDefaultValue(const size_t index, CString& defaultValue) const override
    {
        OV_ERROR_UNLESS_KRF(index < m_interfacors.at(Setting).size(),
                            "Setting index = [" << index << "] is out of range (max index = [" << m_interfacors.at(Setting).size() - 1 << "])",
                            ErrorType::OutOfBound);

        defaultValue = std::static_pointer_cast<CSetting>(m_interfacors.at(Setting)[index])->m_DefaultValue;
        return true;
    }

    bool getSettingDefaultValue(const CIdentifier& identifier, CString& defaultValue) const override
    {
        const auto it = m_interfacorIDToIdx.at(Setting).find(identifier);
        OV_ERROR_UNLESS_KRF(it != m_interfacorIDToIdx.at(Setting).end(), "Failed to find setting with id " << identifier.str(),
                            ErrorType::ResourceNotFound);

        return this->getSettingDefaultValue(it->second, defaultValue);
    }

    bool getSettingDefaultValue(const CString& name, CString& defaultValue) const override
    {
        const auto it = m_interfacorNameToIdx.at(Setting).find(name);
        OV_ERROR_UNLESS_KRF(it != m_interfacorNameToIdx.at(Setting).end(), "Failed to find setting with name " << name,
                            ErrorType::ResourceNotFound);

        return this->getSettingDefaultValue(it->second, defaultValue);
    }

    bool getSettingValue(const size_t index, CString& value) const override
    {
        OV_ERROR_UNLESS_KRF(index < m_interfacors.at(Setting).size(),
                            "Setting index = [" << index << "] is out of range (max index = [" << m_interfacors.at(Setting).size() - 1 << "])",
                            ErrorType::OutOfBound);

        std::shared_ptr<CSetting> setting = std::static_pointer_cast<CSetting>(m_interfacors.at(Setting)[index]);
        value = setting->m_parameter->toString().c_str();
        return true;
    }

    bool getSettingValue(const CIdentifier& identifier, CString& value) const override
    {
        const auto it = m_interfacorIDToIdx.at(Setting).find(identifier);
        OV_ERROR_UNLESS_KRF(it != m_interfacorIDToIdx.at(Setting).end(), "Failed to find setting with id " << identifier.str(),
                            ErrorType::ResourceNotFound);

        return this->getSettingValue(it->second, value);
    }

    bool getSettingValue(const CString& name, CString& value) const override
    {
        const auto it = m_interfacorNameToIdx.at(Setting).find(name);
        OV_ERROR_UNLESS_KRF(it != m_interfacorNameToIdx.at(Setting).end(), "Failed to find setting with name " << name,
                            ErrorType::ResourceNotFound);

        return this->getSettingValue(it->second, value);
    }

    bool getSettingValue(const size_t index, Parameters::ParameterTraits*& outParam) const override {
        OV_ERROR_UNLESS_KRF(index < m_interfacors.at(Setting).size(),
                            "Setting index = [" << index << "] is out of range (max index = [" << m_interfacors.at(Setting).size() - 1 << "])",
                            ErrorType::OutOfBound);

        std::shared_ptr<CSetting> setting = std::static_pointer_cast<CSetting>(m_interfacors.at(Setting)[index]);
        OV_ERROR_UNLESS_KRF(setting, "Failed to get setting value", ErrorType::Internal);

        outParam = setting->m_parameter.get();
        return true;
    }

    bool getSettingValue(const CString& name, Parameters::ParameterTraits*& param) const override {
        const auto it = m_interfacorNameToIdx.at(Setting).find(name);
        OV_ERROR_UNLESS_KRF(it != m_interfacorNameToIdx.at(Setting).end(), "Failed to find setting with name " << name,
                            ErrorType::ResourceNotFound);

        getSettingValue(it->second, param);
        return true;
    }

    bool getSettingMod(const size_t index, bool& value) const override
    {
        OV_ERROR_UNLESS_KRF(index < m_interfacors.at(Setting).size(),
                            "Setting index = [" << index << "] is out of range (max index = [" << m_interfacors.at(Setting).size() - 1 << "])",
                            ErrorType::OutOfBound);

        value = std::static_pointer_cast<CSetting>(m_interfacors.at(Setting)[index])->m_Mod;
        return true;
    }

    bool getSettingMod(const CIdentifier& identifier, bool& value) const override
    {
        const auto it = m_interfacorIDToIdx.at(Setting).find(identifier);
        OV_ERROR_UNLESS_KRF(it != m_interfacorIDToIdx.at(Setting).end(),
                            "Failed to find setting with id " << identifier.str(), Kernel::ErrorType::ResourceNotFound);

        return this->getSettingMod(it->second, value);
    }

    bool getSettingMod(const CString& name, bool& value) const override
    {
        const auto it = m_interfacorNameToIdx.at(Setting).find(name);
        OV_ERROR_UNLESS_KRF(it != m_interfacorNameToIdx.at(Setting).end(),
                            "Failed to find setting with name " << name, Kernel::ErrorType::ResourceNotFound);

        return this->getSettingMod(it->second, value);
    }

    bool setSettingType(const size_t index, const CIdentifier& typeID) override { return this->setInterfacorType(Setting, index, typeID); }

    bool setSettingName(const size_t index, const CString& name) override { return this->setInterfacorName(Setting, index, name); }

    bool setSettingDefaultValue(const size_t index, const CString& defaultValue) override
    {
        OV_ERROR_UNLESS_KRF(index < m_interfacors.at(Setting).size(),
                            "Setting index = [" << index << "] is out of range (max index = [" << m_interfacors.at(Setting).size() - 1 << "])",
                            ErrorType::OutOfBound);

        std::static_pointer_cast<CSetting>(m_interfacors[Setting][index])->m_DefaultValue = defaultValue;

        this->notify(EBoxModification::SettingDefaultValueChanged, index);

        return true;
    }

    bool setSettingDefaultValue(const CIdentifier& identifier, const CString& defaultValue) override
    {
        const auto it = m_interfacorIDToIdx.at(Setting).find(identifier);
        OV_ERROR_UNLESS_KRF(it != m_interfacorIDToIdx.at(Setting).end(), "Failed to find setting with id " << identifier.str(),
                            ErrorType::ResourceNotFound);

        return this->setSettingDefaultValue(it->second, defaultValue);
    }

    bool setSettingDefaultValue(const CString& name, const CString& defaultValue) override
    {
        const auto it = m_interfacorNameToIdx.at(Setting).find(name);
        OV_ERROR_UNLESS_KRF(it != m_interfacorNameToIdx.at(Setting).end(), "Failed to find setting with name " << name,
                            ErrorType::ResourceNotFound);
        return this->setSettingDefaultValue(it->second, defaultValue);
    }

    bool setSettingValue(const size_t index, const CString& value, const bool notify = true) override
    {
        OV_ERROR_UNLESS_KRF(index < m_interfacors.at(Setting).size(),
                            "Setting index = [" << index << "] is out of range (max index = [" << m_interfacors.at(Setting).size() - 1 << "])",
                            ErrorType::OutOfBound);

        auto setting = std::static_pointer_cast<CSetting>(m_interfacors[Setting][index]);
        if (setting->getValue() != value)
        {
            setting->setValue(value);

            if (notify)
            {
                this->notify(EBoxModification::SettingValueChanged, index);
                this->notifySettingChange(SettingValueUpdate, int(index));
            }
        }

        return true;
    }

    bool setSettingValue(const CIdentifier& identifier, const CString& value) override
    {
        const auto it = m_interfacorIDToIdx.at(Setting).find(identifier);
        OV_ERROR_UNLESS_KRF(it != m_interfacorIDToIdx.at(Setting).end(), "Failed to find setting with id " << identifier.str(),
                            ErrorType::ResourceNotFound);

        return this->setSettingValue(it->second, value);
    }

    bool setSettingValue(const CString& name, const CString& value) override
    {
        const auto it = m_interfacorNameToIdx.at(Setting).find(name);
        OV_ERROR_UNLESS_KRF(it != m_interfacorNameToIdx.at(Setting).end(), "Failed to find setting with name " << name, Kernel::ErrorType::ResourceNotFound);

        return this->setSettingValue(it->second, value);
    }

    bool setSettingMod(const size_t index, const bool value) override
    {
        OV_ERROR_UNLESS_KRF(index < m_interfacors.at(Setting).size(),
                            "Setting index = [" << index << "] is out of range (max index = [" << m_interfacors.at(Setting).size() - 1 << "])",
                            ErrorType::OutOfBound);

        std::static_pointer_cast<CSetting>(m_interfacors[Setting][index])->m_Mod = value;

        //this->notify(EBoxModification::SettingNameChanged, index);
        return true;
    }

    bool setSettingMod(const CString& name, const bool value) override
    {
        const auto it = m_interfacorNameToIdx.at(Setting).find(name);
        OV_ERROR_UNLESS_KRF(it != m_interfacorNameToIdx.at(Setting).end(), "Failed to find setting with name " << name,
                            ErrorType::ResourceNotFound);

        return this->setSettingMod(it->second, value);
    }

    bool setSettingMod(const CIdentifier& identifier, const bool value) override
    {
        const auto it = m_interfacorIDToIdx.at(Setting).find(identifier);
        OV_ERROR_UNLESS_KRF(it != m_interfacorIDToIdx.at(Setting).end(), "Failed to find setting with id " << identifier.str(),
                            ErrorType::ResourceNotFound);

        return this->setSettingMod(it->second, value);
    }

    bool swapInterfacors(const EBoxInterfacorType interfacorType, const size_t indexA, const size_t indexB)
    {
        OV_ERROR_UNLESS_KRF(indexA < m_interfacors.at(interfacorType).size(),
                            INTERFACOR_TYPE_TO_NAME.at(interfacorType) << " index = [" << indexA << "] is out of range (max index = [" << m_interfacors.
                            at(
                                interfacorType).size() - 1 << "])",
                            ErrorType::OutOfBound);
        OV_ERROR_UNLESS_KRF(indexB < m_interfacors.at(interfacorType).size(),
                            INTERFACOR_TYPE_TO_NAME.at(interfacorType) << " index = [" << indexB << "] is out of range (max index = [" << m_interfacors.
                            at(
                                interfacorType).size() - 1 << "])",
                            ErrorType::OutOfBound);

        CString nameA;
        CIdentifier identifierA = CIdentifier::undefined();
        CString nameB;
        CIdentifier identifierB = CIdentifier::undefined();

        this->getInterfacorName(interfacorType, indexA, nameA);
        this->getInterfacorIdentifier(interfacorType, indexA, identifierA);
        this->getInterfacorName(interfacorType, indexB, nameB);
        this->getInterfacorIdentifier(interfacorType, indexB, identifierB);

        const auto itA = m_interfacors[interfacorType].begin() + indexA;
        const auto itB = m_interfacors[interfacorType].begin() + indexB;
        // swap settings
        std::iter_swap(itA, itB);
        // update associated maps
        m_interfacorNameToIdx.at(interfacorType)[nameA]     = indexB;
        m_interfacorNameToIdx.at(interfacorType)[nameB]     = indexA;
        m_interfacorIDToIdx.at(interfacorType)[identifierA] = indexB;
        m_interfacorIDToIdx.at(interfacorType)[identifierB] = indexA;

        return true;
    }

    bool swapSettings(const size_t indexA, const size_t indexB) override { return this->swapInterfacors(Setting, indexA, indexB); }
    bool swapInputs(const size_t indexA, const size_t indexB) override { return this->swapInterfacors(Input, indexA, indexB); }
    bool swapOutputs(const size_t indexA, const size_t indexB) override { return this->swapInterfacors(Output, indexA, indexB); }

    void notifySettingChange(const EBoxEventMessageType eType, const int firstIdx = -1, const int secondIdx = -1)
    {
        if (m_isObserverNotificationActive)
        {
            BoxEventMessage event;
            event.m_Type      = eType;
            event.m_FirstIdx  = firstIdx;
            event.m_SecondIdx = secondIdx;

            this->setChanged();
            this->notifyObservers(&event);
        }
    }

    bool hasModifiableSettings() const override
    {
        for (const auto& setting : m_interfacors.at(Setting)) {
            if (std::static_pointer_cast<CSetting>(setting)->m_Mod) {
                return true;
            }
        }
        return false;
    }

    size_t* getModifiableSettings(size_t& count) const override
    {
        size_t* res = nullptr;
        count       = m_modifiableSettingIndexes.size();
        return res;
    }

    bool updateInterfacorIdentifier(const EBoxInterfacorType interfacorType, const size_t index, const CIdentifier& newID) override
    {
        OV_ERROR_UNLESS_KRF(index < m_interfacors.at(interfacorType).size(),
                            INTERFACOR_TYPE_TO_NAME.at(interfacorType) << " index = [" << index << "] is out of range (max index = [" << m_interfacors.
                            at(
                                Setting).size() - 1 << "])",
                            ErrorType::OutOfBound);

        OV_ERROR_UNLESS_KRF(newID != CIdentifier::undefined(), INTERFACOR_TYPE_TO_NAME.at(interfacorType) << " identifier can not be undefined",
                            ErrorType::BadArgument);

        CIdentifier oldIdentifier = CIdentifier::undefined();
        this->getInterfacorIdentifier(interfacorType, index, oldIdentifier);

        if (oldIdentifier != newID)
        {
            // identifier key update is necessary
            const auto it = m_interfacorIDToIdx.at(interfacorType).find(newID);
            OV_ERROR_UNLESS_KRF(it == m_interfacorIDToIdx.at(interfacorType).end(),
                                "Conflict in " << INTERFACOR_TYPE_TO_NAME.at(interfacorType) <<
                                " identifiers. An entity with the same identifier exists.",
                                ErrorType::ResourceNotFound);
            m_interfacors[interfacorType][index]->m_ID = newID;
            m_interfacorIDToIdx[interfacorType][newID] = index;
            // remove the old identifier key
            const auto itOld = m_interfacorIDToIdx[interfacorType].find(oldIdentifier);
            if (itOld != m_interfacorIDToIdx[interfacorType].end()) { m_interfacorIDToIdx[interfacorType].erase(itOld); }
        }
        return true;
    }

    //*/

    bool acceptVisitor(IObjectVisitor& rObjectVisitor) override
    {
        CObjectVisitorContext context(this->getKernelContext());
        return rObjectVisitor.processBegin(context, *this) && rObjectVisitor.processEnd(context, *this);
    }

protected:

    void clearBox()
    {
        m_boxAlgorithmDesc = nullptr;
        m_algorithmClassID = CIdentifier::undefined();
        m_name             = "";
        m_interfacors[Input].clear();
        m_interfacors[Output].clear();
        m_interfacors[Setting].clear();
        m_interfacorIDToIdx[Input].clear();
        m_interfacorNameToIdx[Input].clear();
        m_interfacorIDToIdx[Output].clear();
        m_interfacorNameToIdx[Output].clear();
        m_interfacorIDToIdx.at(Setting).clear();
        m_interfacorNameToIdx.at(Setting).clear();

        this->removeAllAttributes();
    }

    void enableNotification() { m_isNotificationActive = true; }
    void disableNotification() { m_isNotificationActive = false; }

    void notify(const EBoxModification eBoxModificationType, const size_t index)
    {
        if (m_boxListener && !m_isNotifyingDesc && m_isNotificationActive)
        {
            CBoxListenerContext context(this->getKernelContext(), *this, index);
            m_isNotifyingDesc = true;
            m_boxListener->process(context, eBoxModificationType);
            m_isNotifyingDesc = false;
        }
    }

    void notify(const EBoxModification boxModification) { this->notify(boxModification, 0xffffffff); }

    _IsDerivedFromClass_Final_(TAttributable<TKernelObject<T>>, OVK_ClassId_Kernel_Scenario_Box)

    CString getUnusedName(const std::map<CString, size_t>& nameToIndex, const CString& suggestedName) const
    {
        size_t idx = 1;
        CString newName;
        auto it = nameToIndex.find(suggestedName);
        do
        {
            newName = suggestedName;
            if (it != nameToIndex.end())
            {
                newName += "(" + CString(std::to_string(idx).c_str()) + ")";
                it = nameToIndex.find(newName);
                idx++;
            }
        } while (it != nameToIndex.end());
        return newName;
    }

    IScenario* m_ownerScenario                           = nullptr;
    const Plugins::IBoxAlgorithmDesc* m_boxAlgorithmDesc = nullptr;
    Plugins::IBoxListener* m_boxListener                 = nullptr;
    bool m_isNotifyingDesc                               = false;
    bool m_isNotificationActive                          = true;
    bool m_isObserverNotificationActive                  = true;

    CIdentifier m_identifier       = CIdentifier::undefined();
    CIdentifier m_algorithmClassID = CIdentifier::undefined();
    CString m_name                 = "Unnamed";

    std::map<EBoxInterfacorType, std::map<CIdentifier, size_t>> m_interfacorIDToIdx;
    std::map<EBoxInterfacorType, std::map<CString, size_t>> m_interfacorNameToIdx;

    //to avoid having to recheck every setting every time
    //careful to update at each setting modification
    std::vector<size_t> m_modifiableSettingIndexes;

    std::vector<CIdentifier> m_supportInputTypes;
    std::vector<CIdentifier> m_supportOutputTypes;
private:
    std::map<EBoxInterfacorType, std::vector<std::shared_ptr<CInterfacor>>> m_interfacors;
};
}  // namespace Kernel
}  // namespace OpenViBE
