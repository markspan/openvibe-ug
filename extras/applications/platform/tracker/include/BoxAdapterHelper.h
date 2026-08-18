/*
 * This file implements various contexts we need in order to run a box.
 *
 * @fixme most of the implementation is very primitive and may be even missing altogether
 *
 */
#pragma once

#include <thread>
#include <mutex>
#include <condition_variable>
#include <stack>
#include <map>
#include <vector>

#include <openvibe/ov_all.h>
#include <openvibe/CMatrix.hpp>

#include <fs/Files.h>

#include <ebml/CReader.h>
#include <ebml/CReaderHelper.h>

#include <algorithm> // std::find on Ubuntu

#include "Contexted.h"
#include "EncodedChunk.h"

namespace OpenViBE {
namespace Tracker {

/**
 * \class TrackerPlayerContext
 * \brief Implements Kernel::IPlayerContext
 * \author J. T. Lindgren
 *
 */
class TrackerPlayerContext final : protected Contexted, public Kernel::IPlayerContext
{
public:
    explicit TrackerPlayerContext(const Kernel::IKernelContext& ctx) : Contexted(ctx) { }

    uint64_t getCurrentTime() const override { return 0; }
    uint64_t getCurrentLateness() const override { return 0; }
    double getCurrentCPUUsage() const override { return 0; }
    double getCurrentFastForwardMaximumFactor() const override { return 0; }

    bool stop() override { return true; }
    bool pause() override { return true; }
    bool play() override { return true; }
    bool forward() override { return true; }

    Kernel::EPlayerStatus getStatus() const override { return Kernel::EPlayerStatus::Stop; }
    Kernel::IConfigurationManager& getConfigurationManager() const override { return m_kernelCtx.getConfigurationManager(); }
    Kernel::IAlgorithmManager& getAlgorithmManager() const override { return m_kernelCtx.getAlgorithmManager(); }
    Kernel::ILogManager& getLogManager() const override { return m_kernelCtx.getLogManager(); }
    Kernel::CErrorManager& getErrorManager() const override { return m_kernelCtx.getErrorManager(); }
    Kernel::IScenarioManager& getScenarioManager() const override { return m_kernelCtx.getScenarioManager(); }
    Kernel::ITypeManager& getTypeManager() const override { return m_kernelCtx.getTypeManager(); }

    bool canCreatePluginObject(const CIdentifier& /*pluginID*/) const override { return false; }
    Plugins::IPluginObject* createPluginObject(const CIdentifier& /*pluginID*/) const override { return nullptr; }
    bool releasePluginObject(Plugins::IPluginObject* /*pluginObject*/) const override { return true; }

    CIdentifier getClassIdentifier() const override { return CIdentifier(); }
};

/**
 * \class TrackerBox
 * \brief Implements Kernel::IBox
 * \author J. T. Lindgren
 *
 */
class TrackerBox : protected Contexted, public Kernel::IBox
{
public:
    explicit TrackerBox(const Kernel::IKernelContext& ctx) : Contexted(ctx) { }
    CIdentifier getIdentifier() const override { return m_ID; }
    CString getName() const override { return m_Name; }

    /*
    CIdentifier getAlgorithmClassIdentifier() override { return m_AlgorithmClassID; };
    */

    // @f
    CIdentifier getUnusedSettingIdentifier(const CIdentifier& id = CIdentifier::undefined()) const override { return id; }
    CIdentifier getUnusedInputIdentifier(const CIdentifier& id = CIdentifier::undefined()) const override { return id; }
    CIdentifier getUnusedOutputIdentifier(const CIdentifier& id = CIdentifier::undefined()) const override { return id; }

    bool addInterfacor(const Kernel::EBoxInterfacorType /*type*/, const CString& /*name*/, const CIdentifier& /*id1*/,
                       const CIdentifier& /*id2*/, const bool /*notify*/  = true) override { return false; }

    bool removeInterfacor(const Kernel::EBoxInterfacorType /*type*/, const size_t /*idx*/, const bool /*notify*/  = true) override { return false; }

    size_t getInterfacorCount(const Kernel::EBoxInterfacorType interfacor) const override
    {
        if (interfacor == Kernel::Setting) { return getSettingCount(); }
        if (interfacor == Kernel::Input) { return getInputCount(); }
        if (interfacor == Kernel::Output) { return getOutputCount(); }

        return 0;
    }

    size_t getInterfacorCountIncludingDeprecated(const Kernel::EBoxInterfacorType type) const override { return getInterfacorCount(type); }

    bool getInterfacorIdentifier(const Kernel::EBoxInterfacorType /*type*/, const size_t /*idx*/, CIdentifier& /*id*/) const override { return false; }

    bool getInterfacorIndex(const Kernel::EBoxInterfacorType /*type*/, const CIdentifier& /*id*/, size_t& /*idx*/) const override { return false; }

    bool getInterfacorIndex(const Kernel::EBoxInterfacorType /*type*/, const CString& /*name*/, size_t& /*idx*/) const override { return false; }

    bool getInterfacorType(const Kernel::EBoxInterfacorType /*type*/, const size_t /*idx*/, CIdentifier& /*id*/) const override { return false; }

    bool getInterfacorType(const Kernel::EBoxInterfacorType /*type*/, const CIdentifier& /*id1*/, CIdentifier& /*id2*/) const override { return false; }

    bool getInterfacorType(const Kernel::EBoxInterfacorType /*type*/, const CString& /*name*/, CIdentifier& /*id*/) const override { return false; }

    bool getInterfacorName(const Kernel::EBoxInterfacorType /*type*/, const size_t /*idx*/, CString& /*name*/) const override { return false; }

    bool getInterfacorName(const Kernel::EBoxInterfacorType /*type*/, const CIdentifier& /*id*/, CString& /*name*/) const override { return false; }

    bool getInterfacorDeprecatedStatus(const Kernel::EBoxInterfacorType /*type*/, const size_t /*idx*/, bool& /*value*/) const override { return false; }

    bool getInterfacorDeprecatedStatus(const Kernel::EBoxInterfacorType /*type*/, const CIdentifier& /*id*/, bool& /*value*/) const override { return false; }

    bool hasInterfacorWithIdentifier(const Kernel::EBoxInterfacorType /*type*/, const CIdentifier& /*id*/) const override { return false; }

    bool hasInterfacorWithType(const Kernel::EBoxInterfacorType /*type*/, const size_t /*idx*/, const CIdentifier& /*id*/) const override { return false; }

    bool hasInterfacorWithNameAndType(const Kernel::EBoxInterfacorType /*type*/, const CString& /*name*/, const CIdentifier& /*id*/) const override
    {
        return false;
    }

    bool setInterfacorType(const Kernel::EBoxInterfacorType /*type*/, const size_t /*idx*/, const CIdentifier& /*id*/) override { return false; }

    bool setInterfacorType(const Kernel::EBoxInterfacorType /*type*/, const CIdentifier& /*id1*/, const CIdentifier& /*id2*/) override { return false; }

    bool setInterfacorType(const Kernel::EBoxInterfacorType /*type*/, const CString& /*name*/, const CIdentifier& /*id*/) override { return false; }

    bool setInterfacorName(const Kernel::EBoxInterfacorType /*type*/, const size_t /*idx*/, const CString& /*name*/) override { return false; }

    bool setInterfacorName(const Kernel::EBoxInterfacorType /*type*/, const CIdentifier& /*id*/, const CString& /*name*/) override { return false; }

    bool setInterfacorDeprecatedStatus(const Kernel::EBoxInterfacorType /*type*/, const size_t /*idx*/, const bool /*value*/) override { return false; }

    bool setInterfacorDeprecatedStatus(const Kernel::EBoxInterfacorType /*type*/, const CIdentifier& /*id*/, const bool /*value*/) override { return false; }

    bool updateInterfacorIdentifier(const Kernel::EBoxInterfacorType /*type*/, const size_t /*idx*/, const CIdentifier& /*newID*/) override { return false; }

    bool addInterfacorTypeSupport(const Kernel::EBoxInterfacorType /*type*/, const CIdentifier& /*id*/) override { return false; }
    bool hasInterfacorTypeSupport(const Kernel::EBoxInterfacorType /*type*/, const CIdentifier& /*id*/) const override { return false; }

    bool setIdentifier(const CIdentifier& id) override
    {
        m_ID = id;
        return true;
    }

    bool setName(const CString& name) override
    {
        m_Name = name;
        return true;
    }

    bool setAlgorithmClassIdentifier(const CIdentifier& algorithmClassID) override
    {
        m_AlgorithmClassID = algorithmClassID;
        return true;
    }

    bool initializeFromAlgorithmClassIdentifier(const CIdentifier& /*algorithmClassID*/) override { return false; }

    bool initializeFromExistingBox(const IBox& existingBox) override
    {
        //  this->clearBox();
        this->setName(existingBox.getName());
        this->setAlgorithmClassIdentifier(existingBox.getAlgorithmClassIdentifier());

        for (size_t i = 0; i < existingBox.getInputCount(); ++i) {
            CIdentifier typeID = CIdentifier::undefined();
            CString name;
            existingBox.getInputType(i, typeID);
            existingBox.getInputName(i, name);
            addInput(name, typeID);
        }

        for (size_t i = 0; i < existingBox.getOutputCount(); ++i) {
            CIdentifier typeID = CIdentifier::undefined();
            CString name;
            existingBox.getOutputType(i, typeID);
            existingBox.getOutputName(i, name);
            addOutput(name, typeID);
        }

        for (size_t i = 0; i < existingBox.getSettingCount(); ++i) {
            CIdentifier typeID = CIdentifier::undefined();
            CString name;
            CString value;
            CString defaultValue;
            bool modifiability = false;
            const bool notify  = false;
            existingBox.getSettingType(i, typeID);
            existingBox.getSettingName(i, name);
            existingBox.getSettingValue(i, value);
            existingBox.getSettingDefaultValue(i, defaultValue);
            existingBox.getSettingMod(i, modifiability);
            addSetting(name, typeID, defaultValue, -1, modifiability);
            setSettingValue(i, value, notify);
        }

        CIdentifier id = existingBox.getNextAttributeIdentifier(CIdentifier::undefined());
        while (id != CIdentifier::undefined()) {
            this->addAttribute(id, existingBox.getAttributeValue(id));
            id = existingBox.getNextAttributeIdentifier(id);
        }

        CIdentifier streamTypeID = CIdentifier::undefined();
        while ((streamTypeID = this->getKernelContext().getTypeManager().getNextTypeIdentifier(streamTypeID)) != CIdentifier::undefined()) {
            if (this->getKernelContext().getTypeManager().isStream(streamTypeID)) {
                //First check if it is a stream
                if (existingBox.hasInputSupport(streamTypeID)) { this->addInputSupport(streamTypeID); }
                if (existingBox.hasOutputSupport(streamTypeID)) { this->addOutputSupport(streamTypeID); }
            }
        }
        //      this->enableNotification();
        //      this->notify(EBoxModification::Initialized);
        return true;
    }

    bool addInput(const CString& name, const CIdentifier& typeID, const CIdentifier& /*id*/ = CIdentifier::undefined(), const bool /*notify*/  = true) override
    {
        IOEntry tmp;
        tmp.name = name;
        tmp.id   = typeID;
        m_Inputs.push_back(tmp);

        return true;
    }

    bool removeInput(const size_t /*idx*/, const bool /*notify*/  = true) override { return false; }

    size_t getInputCount() const override { return m_Inputs.size(); }

    bool getInputType(const size_t index, CIdentifier& typeID) const override
    {
        typeID = m_Inputs[index].id;
        return true;
    }

    bool getInputName(const size_t index, CString& name) const override
    {
        name = m_Inputs[index].name;
        return true;
    }

    bool setInputType(const size_t index, const CIdentifier& typeID) override
    {
        if (m_Inputs.size() <= index) { m_Inputs.resize(index + 1); }
        m_Inputs[index].id = typeID;
        return true;
    }

    bool setInputName(const size_t index, const CString& name) override
    {
        if (m_Inputs.size() <= index) { m_Inputs.resize(index + 1); }
        m_Inputs[index].name = name;
        return true;
    }


    bool addOutput(const CString& name, const CIdentifier& typeID, const CIdentifier& /*id*/ = CIdentifier::undefined(), const bool /*notify*/  = true) override
    {
        IOEntry tmp;
        tmp.name = name;
        tmp.id   = typeID;
        m_Outputs.push_back(tmp);
        return true;
    }

    bool removeOutput(const size_t /*index*/, const bool /*notify*/  = true) override { return false; }

    size_t getOutputCount() const override { return m_Outputs.size(); }

    bool getOutputType(const size_t index, CIdentifier& typeID) const override
    {
        typeID = m_Outputs[index].id;
        return true;
    }

    bool getOutputName(const size_t index, CString& name) const override
    {
        name = m_Outputs[index].name;
        return true;
    }

    bool setOutputType(const size_t index, const CIdentifier& typeID) override
    {
        if (m_Outputs.size() <= index) { m_Outputs.resize(index + 1); }
        m_Outputs[index].id = typeID;
        return true;
    }

    bool setOutputName(const size_t index, const CString& name) override
    {
        if (m_Outputs.size() <= index) { m_Outputs.resize(index + 1); }
        m_Outputs[index].name = name;
        return true;
    }

    bool addSetting(const CString& name, const CIdentifier& typeID, const CString& defaultValue,
                    const size_t index                = size_t(-1), const bool /*modiafiability*/        = false,
                    const CIdentifier& /*identifier*/ = CIdentifier::undefined(), const bool /*notify*/  = true) override
    {
        Kernel::ITypeManager& typeManager = m_kernelCtx.getTypeManager();

        CString value(defaultValue);
        if (typeManager.isEnumeration(typeID)) {
            if (typeManager.getEnumerationEntryValueFromName(typeID, defaultValue) == CIdentifier::undefined().id()) {
                if (typeManager.getEnumerationEntryCount(typeID) != 0) {
                    // get value to the first enum entry
                    // and eventually correct this after
                    uint64_t tmp = 0;
                    typeManager.getEnumerationEntry(typeID, 0, value, tmp);

                    // Find if the default value string actually is an identifier, otherwise just keep the zero index name as default.
                    CIdentifier id = CIdentifier::undefined();
                    id.fromString(defaultValue);

                    // Finally, if it is an identifier, then a name should be found
                    // from the type manager ! Otherwise value is left to the default.
                    const CString candidateValue = typeManager.getEnumerationEntryNameFromValue(typeID, id.id());
                    if (candidateValue != CString("")) { value = candidateValue; }
                }
            }
        }

        Setting tmp;
        tmp.name         = name;
        tmp.id           = typeID;
        tmp.value        = value;
        tmp.defaultValue = value;

        const size_t idx = index;

        if (index == size_t(-1) || index == m_Settings.size()) { m_Settings.push_back(tmp); }
        else {
            auto it = m_Settings.begin();
            it += idx;
            m_Settings.insert(it, tmp);
        }

        return true;
    }

    bool removeSetting(const size_t index, const bool /*notify*/  = true) override
    {
        auto it = m_Settings.begin() + index;
        if (it == m_Settings.end()) {
            getLogManager() << Kernel::LogLevel_Error << "Error: No setting found\n";
            return false;
        }
        it = m_Settings.erase(it);
        return true;
    }

    size_t getSettingCount() const override { return m_Settings.size(); }

    bool hasSettingWithName(const CString& /*name*/) const override { return false; }

    //virtual int getSettingIndex(const CString& name) const override { return -1; }

    bool getSettingType(const size_t index, CIdentifier& typeID) const override
    {
        typeID = m_Settings[index].id;
        return true;
    }

    bool getSettingName(const size_t index, CString& name) const override
    {
        name = m_Settings[index].name;
        return true;
    }

    bool getSettingDefaultValue(const CIdentifier& /*identifier*/, CString& /*rDefaultValue*/) const override { return false; }

    bool getSettingDefaultValue(const size_t index, CString& rDefaultValue) const override
    {
        rDefaultValue = m_Settings[index].defaultValue;
        return true;
    }

    bool getSettingDefaultValue(const CString& /*name*/, CString& /*rDefaultValue*/) const override { return false; }

    bool getSettingValue(const size_t index, CString& value) const override
    {
        if (m_Settings.size() < index) { return false; }

        value = m_Settings[index].value;
        return true;
    }

    bool setSettingType(const size_t index, const CIdentifier& typeID) override
    {
        if (m_Settings.size() <= index) { m_Settings.resize(index + 1); }
        m_Settings[index].id = typeID;
        return true;
    }

    bool setSettingName(const size_t index, const CString& name) override
    {
        if (m_Settings.size() <= index) { m_Settings.resize(index + 1); }
        m_Settings[index].name = name;
        return true;
    }

    bool setSettingDefaultValue(const size_t index, const CString& rDefaultValue) override
    {
        if (m_Settings.size() <= index) { m_Settings.resize(index + 1); }
        m_Settings[index].defaultValue = rDefaultValue;
        return true;
    }

    bool setSettingValue(const size_t index, const CString& rValue, const bool /*notify*/  = true) override
    {
        if (m_Settings.size() <= index) { m_Settings.resize(index + 1); }
        m_Settings[index].value = rValue;
        return true;
    }

    bool getSettingMod(const size_t /*index*/, bool& /*value*/) const override { return false; }
    bool setSettingMod(const size_t /*index*/, const bool /*value*/) override { return false; }
    bool getSettingValue(const CIdentifier& /*identifier*/, CString& /*value*/) const override { return false; }
    bool getSettingValue(const CString& /*name*/, CString& /*value*/) const override { return false; }
    bool setSettingDefaultValue(const CIdentifier& /*identifier*/, const CString& /*rDefaultValue*/) override { return false; }
    bool setSettingDefaultValue(const CString& /*name*/, const CString& /*rDefaultValue*/) override { return false; }
    bool setSettingValue(const CIdentifier& /*identifier*/, const CString& /*value*/) override { return false; }
    bool setSettingValue(const CString& /*name*/, const CString& /*value*/) override { return false; }
    bool getSettingMod(const CIdentifier& /*identifier*/, bool& /*value*/) const override { return false; }
    bool getSettingMod(const CString& /*name*/, bool& /*value*/) const override { return false; }
    bool setSettingMod(const CIdentifier& /*identifier*/, const bool /*value*/) override { return false; }
    bool setSettingMod(const CString& /*name*/, const bool /*value*/) override { return false; }
    bool swapSettings(const size_t /*indexA*/, const size_t /*indexB*/) override { return false; }
    bool swapInputs(const size_t /*indexA*/, const size_t /*indexB*/) override { return false; }
    bool swapOutputs(const size_t /*indexA*/, const size_t /*indexB*/) override { return false; }
    bool hasModifiableSettings() const override { return false; }

    std::vector<CIdentifier> getInputSupportTypes() const override
    {
        std::vector<CIdentifier> tmp;
        return tmp;
    }

    std::vector<CIdentifier> getOutputSupportTypes() const override
    {
        std::vector<CIdentifier> tmp;
        return tmp;
    }

    size_t* getModifiableSettings(size_t& rCount) const override
    {
        rCount = 0;
        return nullptr;
    }

    bool addInputSupport(const CIdentifier& typeID) override
    {
        m_InputSupports.push_back(typeID);
        return true;
    }

    bool hasInputSupport(const CIdentifier& typeID) const override
    {
        return (m_InputSupports.empty() || std::find(m_InputSupports.begin(), m_InputSupports.end(), typeID) != m_InputSupports.end());
    }

    bool addOutputSupport(const CIdentifier& typeID) override
    {
        m_OutputSupports.push_back(typeID);
        return true;
    }

    bool hasOutputSupport(const CIdentifier& typeID) const override
    {
        return (m_OutputSupports.empty() || std::find(m_OutputSupports.begin(), m_OutputSupports.end(), typeID) != m_OutputSupports.end());
    }

    bool setSupportTypeFromAlgorithmIdentifier(const CIdentifier& /*typeID*/) override { return true; }

    CIdentifier getClassIdentifier() const override { return CIdentifier(); }

    _IsDerivedFromClass_(Kernel::IAttributable, OV_ClassId_Kernel_Scenario_Box)

    struct Setting
    {
        CIdentifier id = CIdentifier::undefined();
        CString name;
        CString value;
        CString defaultValue;
    };

    struct IOEntry
    {
        CIdentifier id = CIdentifier::undefined();
        CString name;
    };

    std::vector<Setting> m_Settings;
    std::vector<IOEntry> m_Inputs;
    std::vector<IOEntry> m_Outputs;
    std::vector<CIdentifier> m_InputSupports;
    std::vector<CIdentifier> m_OutputSupports;

    // This box
    CString m_Name;
    CIdentifier m_ID               = CIdentifier::undefined();
    CIdentifier m_AlgorithmClassID = CIdentifier::undefined();

    // Attributable
    bool addAttribute(const CIdentifier& /*attributeID*/, const CString& /*sAttributeValue*/) override { return true; }
    bool removeAttribute(const CIdentifier& /*attributeID*/) override { return true; }
    bool removeAllAttributes() override { return true; }
    CString getAttributeValue(const CIdentifier& /*attributeID*/) const override { return ""; }
    bool setAttributeValue(const CIdentifier& /*attributeID*/, const CString& /*sAttributeValue*/) override { return true; }
    bool hasAttribute(const CIdentifier& /*attributeID*/) const override { return false; }
    bool hasAttributes() const override { return false; }

    CIdentifier getNextAttributeIdentifier(const CIdentifier& /*previousID*/) const override { return CIdentifier(); }

    // @f
    void clearOutputSupportTypes() override { }
    void clearInputSupportTypes() override { }

    CIdentifier getAlgorithmClassIdentifier() const override { return CIdentifier(); }
};

/**
 * \class TrackerBoxProto
 * \brief Implements Kernel::IBoxProto
 * \author J. T. Lindgren
 *
 */
class TrackerBoxProto : public TrackerBox, public Kernel::IBoxProto
{
public:
    explicit TrackerBoxProto(const Kernel::IKernelContext& ctx) : TrackerBox(ctx) {}

    virtual bool addSetting(const CString& name, const CIdentifier& typeID, const CString& defaultValue, const bool modifiable = false)
    {
        return TrackerBox::addSetting(name, typeID, defaultValue, -1, modifiable);
    }
};


/**
 * \class TrackerBoxIO
 * \brief Implements Kernel::IBoxIO
 * \author J. T. Lindgren
 *
 */
class TrackerBoxIO final : public Kernel::IBoxIO
{
public:
    struct SBufferWithStamps
    {
        CMemoryBuffer* buffer;
        CTime startTime;
        CTime endTime;
    };

    std::vector<std::vector<SBufferWithStamps>> m_InputChunks; // Queue per input. See code for comments.
    std::vector<CMemoryBuffer*> m_OutputChunks;   // Just one buffer per output
    std::vector<size_t> m_Ready;
    std::vector<CTime> m_StartTime;
    std::vector<CTime> m_EndTime;

    ~TrackerBoxIO() override { for (size_t i = 0; i < m_OutputChunks.size(); ++i) { delete m_OutputChunks[i]; } }

    bool initialize(const Kernel::IBox* boxCtx)
    {
        for (size_t i = 0; i < m_OutputChunks.size(); ++i) { delete m_OutputChunks[i]; }

        m_InputChunks.resize(boxCtx->getInputCount());
        m_OutputChunks.resize(boxCtx->getOutputCount());
        m_Ready.resize(boxCtx->getOutputCount(), 0);
        m_StartTime.resize(boxCtx->getOutputCount());
        m_EndTime.resize(boxCtx->getOutputCount());

        for (size_t i = 0; i < m_OutputChunks.size(); ++i) { m_OutputChunks[i] = new CMemoryBuffer(); }

        return true;
    }

    size_t getInputChunkCount(const size_t index) const override
    {
        if (index >= m_InputChunks.size()) { return 0; }
        return m_InputChunks[index].size();
    }

    bool getInputChunk(const size_t index, const size_t chunkIdx, uint64_t& startTime, uint64_t& endTime, size_t& size, const uint8_t*& buffer) const override
    {
        const std::vector<SBufferWithStamps>& chunks = m_InputChunks[index];
        const SBufferWithStamps& chk                 = chunks[chunkIdx];

        startTime = chk.startTime.time();
        endTime   = chk.endTime.time();
        size      = chk.buffer->getSize();
        buffer    = chk.buffer->getDirectPointer();

        return true;
    }

    // Essentially this function being const and requiring its output as CMemoryBuffer prevents us from simply carrying our data as std::vector<EncodedChunk>. 
    const CMemoryBuffer* getInputChunk(const size_t index, const size_t chunkIdx) const override
    {
        const std::vector<SBufferWithStamps>& chunks = m_InputChunks[index];
        const SBufferWithStamps& chk                 = chunks[chunkIdx];

        return chk.buffer;
    }

    uint64_t getInputChunkStartTime(const size_t index, const size_t chunkIdx) const override
    {
        const std::vector<SBufferWithStamps>& chunks = m_InputChunks[index];
        const SBufferWithStamps& chk                 = chunks[chunkIdx];

        return chk.startTime.time();
    }

    uint64_t getInputChunkEndTime(const size_t index, const size_t chunkIdx) const override
    {
        const std::vector<SBufferWithStamps>& chunks = m_InputChunks[index];
        const SBufferWithStamps& chk                 = chunks[chunkIdx];

        return chk.endTime.time();
    }

    bool markInputAsDeprecated(const size_t /*index*/, const size_t /*chunkIdx*/) override { return true; }

    size_t getOutputChunkSize(const size_t index) const override
    {
        if (index >= m_OutputChunks.size()) { return false; }
        return m_OutputChunks[index]->getSize();
    }

    bool setOutputChunkSize(const size_t index, const size_t size, const bool discard = true) override
    {
        if (m_OutputChunks.size() <= index) { m_OutputChunks.resize(index + 1); }
        m_OutputChunks[index]->setSize(size, discard);
        return true;
    }

    uint8_t* getOutputChunkBuffer(const size_t index) override
    {
        if (m_OutputChunks.size() >= index) { return nullptr; }
        return m_OutputChunks[index]->getDirectPointer();
    }

    bool appendOutputChunkData(const size_t index, const uint8_t* buffer, const size_t size) override { return m_OutputChunks[index]->append(buffer, size); }

    CMemoryBuffer* getOutputChunk(const size_t index) override { return m_OutputChunks[index]; }

    bool getOutputChunk(const size_t index, EncodedChunk& chk)
    {
        chk.m_Buffer.assign(m_OutputChunks[index]->getDirectPointer(), m_OutputChunks[index]->getDirectPointer() + m_OutputChunks[index]->getSize());
        chk.m_StartTime = m_StartTime[index];
        chk.m_EndTime   = m_EndTime[index];

        return true;
    }


    bool markOutputAsReadyToSend(const size_t index, const uint64_t startTime, const uint64_t endTime) override
    {
        m_StartTime[index] = startTime;
        m_EndTime[index]   = endTime;
        m_Ready[index]     = true;
        return true;
    }


    CIdentifier getClassIdentifier() const override { return CIdentifier(); }

    bool addInputChunk(const size_t index, const EncodedChunk& chk)
    {
        if (m_InputChunks.size() <= index) { m_InputChunks.resize(index + 1); }
        SBufferWithStamps buf;
        buf.startTime = chk.m_StartTime;
        buf.endTime   = chk.m_EndTime;
        // We cannot copy CMemoryBuffer, so have to use new() @fixme SDK should implement working copy or prevent
        buf.buffer = new CMemoryBuffer(&chk.m_Buffer[0], chk.m_Buffer.size());
        m_InputChunks[index].push_back(buf);
        return true;
    }

    bool clearInputChunks()
    {
        for (size_t i = 0; i < m_InputChunks.size(); ++i) {
            for (auto& chk : m_InputChunks[i]) { delete chk.buffer; }
            m_InputChunks[i].clear();
        }

        return true;
    }

    bool isReadyToSend(const size_t outputIdx) { return (m_Ready[outputIdx] != 0); }

    bool deprecateOutput(const size_t outputIdx)
    {
        m_OutputChunks[outputIdx]->setSize(0, true);
        m_Ready[outputIdx] = false;
        return true;
    }
};

/**
 * \class TrackerBoxAlgorithmContext
 * \brief Implements Kernel::IBoxAlgorithmContext
 * \author J. T. Lindgren
 *
 */
class TrackerBoxAlgorithmContext final : protected Contexted, public Kernel::IBoxAlgorithmContext
{
public:
    explicit TrackerBoxAlgorithmContext(const Kernel::IKernelContext& ctx) : Contexted(ctx), m_StaticBoxCtx(ctx), m_PlayerCtx(ctx) { }
    const Kernel::IBox* getStaticBoxContext() override { return &m_StaticBoxCtx; }
    Kernel::IBoxIO* getDynamicBoxContext() override { return &m_DynamicBoxCtx; }
    Kernel::IPlayerContext* getPlayerContext() override { return &m_PlayerCtx; }
    bool markAlgorithmAsReadyToProcess() override { return true; }
    CIdentifier getClassIdentifier() const override { return CIdentifier(); }
    TrackerBoxIO* getTrackerBoxIO() { return &m_DynamicBoxCtx; }

    TrackerBox m_StaticBoxCtx;
    TrackerBoxIO m_DynamicBoxCtx;
    TrackerPlayerContext m_PlayerCtx;
};
}  // namespace Tracker
}  // namespace OpenViBE
