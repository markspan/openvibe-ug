#pragma once

#include "../ovtk_base.h"
#include <limits>

namespace OpenViBE {
namespace Toolkit {
template <class TBoxAlgorithmParentClass>
class TBoxAlgorithm : public TBoxAlgorithmParentClass
{
public:
    TBoxAlgorithm() { }

    // ====================================================================================================================================

private:
    uint64_t getClockFrequency(Kernel::IBoxAlgorithmContext& ctx) override
    {
        CScopedBoxAlgorithm scopedBoxAlgorithm(m_boxAlgorithmCtx, &ctx);
        return getClockFrequency();
    }

    bool initialize(Kernel::IBoxAlgorithmContext& ctx) override
    {
        CScopedBoxAlgorithm scopedBoxAlgorithm(m_boxAlgorithmCtx, &ctx);
        return initialize();
    }

    bool uninitialize(Kernel::IBoxAlgorithmContext& ctx) override
    {
        CScopedBoxAlgorithm scopedBoxAlgorithm(m_boxAlgorithmCtx, &ctx);
        return uninitialize();
    }

    bool processClock(Kernel::IBoxAlgorithmContext& ctx, Kernel::CMessageClock& msg) override
    {
        CScopedBoxAlgorithm scopedBoxAlgorithm(m_boxAlgorithmCtx, &ctx);
        return processClock(msg);
    }

    bool processInput(Kernel::IBoxAlgorithmContext& ctx, const size_t index) override
    {
        CScopedBoxAlgorithm scopedBoxAlgorithm(m_boxAlgorithmCtx, &ctx);
        return processInput(index);
    }

    bool process(Kernel::IBoxAlgorithmContext& ctx) override
    {
        CScopedBoxAlgorithm oScopedBoxAlgorithm(m_boxAlgorithmCtx, &ctx);
        return process();
    }

    // ====================================================================================================================================

public:
    virtual uint64_t getClockFrequency() { return 0; }
    virtual bool initialize() { return true; }
    virtual bool uninitialize() { return true; }
    virtual bool processClock(Kernel::CMessageClock& /*msg*/) { return false; }
    virtual bool processInput(const size_t /*index*/) { return false; }
    virtual bool process() = 0;

    // ====================================================================================================================================

    virtual Kernel::IBoxAlgorithmContext* getBoxAlgorithmContext() { return m_boxAlgorithmCtx; }
    // virtual Kernel::IBoxAlgorithmContext& getBoxAlgorithmContext() { return *m_boxAlgorithmCtx; } this one should replace !
    virtual const Kernel::IBox& getStaticBoxContext() { return *m_boxAlgorithmCtx->getStaticBoxContext(); }
    virtual Kernel::IBoxIO& getDynamicBoxContext() { return *m_boxAlgorithmCtx->getDynamicBoxContext(); }
    virtual Kernel::IPlayerContext& getPlayerContext() { return *m_boxAlgorithmCtx->getPlayerContext(); }

    virtual Kernel::IAlgorithmManager& getAlgorithmManager() { return getPlayerContext().getAlgorithmManager(); }
    virtual Kernel::IConfigurationManager& getConfigurationManager() { return getPlayerContext().getConfigurationManager(); }
    virtual Kernel::ILogManager& getLogManager() { return getPlayerContext().getLogManager(); }
    virtual Kernel::CErrorManager& getErrorManager() { return getPlayerContext().getErrorManager(); }
    virtual Kernel::IScenarioManager& getScenarioManager() { return getPlayerContext().getScenarioManager(); }
    virtual Kernel::ITypeManager& getTypeManager() { return getPlayerContext().getTypeManager(); }

    virtual bool canCreatePluginObject(const CIdentifier& pluginID) { return getPlayerContext().canCreatePluginObject(pluginID); }

    virtual Plugins::IPluginObject* createPluginObject(const CIdentifier& pluginID) { return getPlayerContext().createPluginObject(pluginID); }

    virtual bool releasePluginObject(Plugins::IPluginObject* pluginObject) { return getPlayerContext().releasePluginObject(pluginObject); }

    // ====================================================================================================================================

    virtual void appendOutputChunkData(const size_t outputIdx, const void* buffer, const size_t size)
    {
        Kernel::IBoxAlgorithmContext* context = this->getBoxAlgorithmContext();
        if (context) {
            Kernel::IBoxIO* boxContext = context->getDynamicBoxContext();
            if (boxContext) { boxContext->appendOutputChunkData(outputIdx, static_cast<const uint8_t*>(buffer), size); }
        }
    }

    template <size_t TOutputIdx>
    void appendOutputChunkData(const void* buffer, const size_t size) { appendOutputChunkData(TOutputIdx, buffer, size); }

    _IsDerivedFromClass_(TBoxAlgorithmParentClass, OVTK_ClassId_)

protected:
    class FSettingValueAutoCast
    {
    public:
        FSettingValueAutoCast(Kernel::IBoxAlgorithmContext& ctx, const size_t index)
            : m_logManager(ctx.getPlayerContext()->getLogManager()),
              m_errorManager(ctx.getPlayerContext()->getErrorManager()),
              m_typeManager(ctx.getPlayerContext()->getTypeManager()),
              m_configManager(ctx.getPlayerContext()->getConfigurationManager())
        {
            ctx.getStaticBoxContext()->getSettingValue(index, m_settingValue);
            ctx.getStaticBoxContext()->getSettingType(index, m_settingType);
        }

        FSettingValueAutoCast(Kernel::IBoxAlgorithmContext& ctx, const CString& name)
            : m_logManager(ctx.getPlayerContext()->getLogManager()),
              m_errorManager(ctx.getPlayerContext()->getErrorManager()),
              m_typeManager(ctx.getPlayerContext()->getTypeManager()),
              m_configManager(ctx.getPlayerContext()->getConfigurationManager())
        {
            ctx.getStaticBoxContext()->getSettingValue(name, m_settingValue);
            ctx.getStaticBoxContext()->getInterfacorType(Kernel::EBoxInterfacorType::Setting, name, m_settingType);
        }

        FSettingValueAutoCast(Kernel::IBoxAlgorithmContext& ctx, const CIdentifier& identifier)
            : m_logManager(ctx.getPlayerContext()->getLogManager()),
              m_errorManager(ctx.getPlayerContext()->getErrorManager()),
              m_typeManager(ctx.getPlayerContext()->getTypeManager()),
              m_configManager(ctx.getPlayerContext()->getConfigurationManager())
        {
            ctx.getStaticBoxContext()->getSettingValue(identifier, m_settingValue);
            ctx.getStaticBoxContext()->getInterfacorType(Kernel::EBoxInterfacorType::Setting, identifier, m_settingType);
        }

        operator uint32_t() const
        {
            double result;
            const CString value = m_configManager.expand(m_settingValue);

            OV_ERROR_UNLESS(m_typeManager.evaluateSettingValue(value, result),
                            "Could not expand numeric expression [" << m_settingValue << "] to unsigned integer 32bits.",
                            Kernel::ErrorType::BadParsing, std::numeric_limits<uint32_t>::max(), m_errorManager, m_logManager);

            return uint32_t(result);
        }

        operator uint64_t() const
        {
            uint64_t stimId     = std::numeric_limits<uint64_t>::max();
            const CString value = m_configManager.expand(m_settingValue);
            double result;
            if (m_typeManager.isEnumeration(m_settingType)) {
                stimId = m_typeManager.getEnumerationEntryValueFromName(m_settingType, value);

                OV_ERROR_UNLESS(stimId != std::numeric_limits<uint64_t>::max(),
                                "Did not find an enumeration value for [" << m_typeManager.getTypeName(m_settingType) << "] = [" << m_settingValue << "]",
                                Kernel::ErrorType::BadParsing, std::numeric_limits<uint64_t>::max(), m_errorManager, m_logManager);
            }
            else if (m_typeManager.evaluateSettingValue(value, result)) { return uint64_t(result); }

            // Seems like currently some plugins use FSettingValueAutoCast without knowing then setting type.
            // In this case, to avoid to pollute the console with useless messages, throw a message only if the
            // setting should be an integer.
            OV_ERROR_UNLESS(stimId != std::numeric_limits<uint64_t>::max() || m_settingType != OV_TypeId_Integer,
                            "Could not expand numeric expression [" << m_settingValue << "] to unsigned integer 64bits.",
                            Kernel::ErrorType::BadParsing, std::numeric_limits<uint64_t>::max(), m_errorManager, m_logManager);

            return stimId;
        }

#if defined TARGET_OS_MacOS
        operator size_t() const
        {
            size_t stimId     = std::numeric_limits<uint64_t>::max();
            const CString value = m_configManager.expand(m_settingValue);
            double result;
            if (m_typeManager.isEnumeration(m_settingType)) {
                stimId = m_typeManager.getEnumerationEntryValueFromName(m_settingType, value);

                OV_ERROR_UNLESS(stimId != std::numeric_limits<uint64_t>::max(),
                                "Did not find an enumeration value for [" << m_typeManager.getTypeName(m_settingType) << "] = [" << m_settingValue << "]",
                                Kernel::ErrorType::BadParsing, std::numeric_limits<uint64_t>::max(), m_errorManager, m_logManager);
            }
            else if (m_typeManager.evaluateSettingValue(value, result)) {
                return size_t(result);
            }

            // Seems like currently some plugins use FSettingValueAutoCast without knowing then setting type.
            // In this case, to avoid to pollute the console with useless messages, throw a message only if the
            // setting should be an integer.
            OV_ERROR_UNLESS(stimId != std::numeric_limits<uint64_t>::max() || m_settingType != OV_TypeId_Integer,
                            "Could not expand numeric expression [" << m_settingValue << "] to unsigned integer 64bits.",
                            Kernel::ErrorType::BadParsing, std::numeric_limits<uint64_t>::max(), m_errorManager, m_logManager);

            return stimId;
        }
#endif

        operator int() const
        {
            double res;
            const CString value = m_configManager.expand(m_settingValue);

            OV_ERROR_UNLESS(m_typeManager.evaluateSettingValue(value, res),
                            "Could not expand numeric expression [" << m_settingValue << "] to integer 32bits.",
                            Kernel::ErrorType::BadParsing, std::numeric_limits<int>::max(), m_errorManager, m_logManager);

            return int(res);
        }

        operator int64_t() const
        {
            double res;
            const CString value = m_configManager.expand(m_settingValue);

            OV_ERROR_UNLESS(m_typeManager.evaluateSettingValue(value, res),
                            "Could not expand numeric expression [" << m_settingValue << "] to integer 64bits.",
                            Kernel::ErrorType::BadParsing, std::numeric_limits<int64_t>::max(), m_errorManager, m_logManager);

            return int64_t(res);
        }

        operator double() const
        {
            double res;
            const CString value = m_configManager.expand(m_settingValue);

            OV_ERROR_UNLESS(m_typeManager.evaluateSettingValue(value, res),
                            "Could not expand numeric expression [" << m_settingValue << "] to double.",
                            Kernel::ErrorType::BadParsing, std::numeric_limits<double>::max(), m_errorManager, m_logManager);

            return double(res);
        }

        operator bool() const { return m_configManager.expandAsBoolean(m_settingValue); }

        operator CString() const { return m_configManager.expand(m_settingValue); }

        explicit operator std::string() const { return m_configManager.expand(m_settingValue).toASCIIString(); }

    private:
        Kernel::ILogManager& m_logManager;
        Kernel::CErrorManager& m_errorManager;
        Kernel::ITypeManager& m_typeManager;
        Kernel::IConfigurationManager& m_configManager;
        CString m_settingValue;
        CIdentifier m_settingType = CIdentifier::undefined();
    };

private:
    class CScopedBoxAlgorithm final
    {
    public:
        CScopedBoxAlgorithm(Kernel::IBoxAlgorithmContext*& ctxRef, Kernel::IBoxAlgorithmContext* ctx)
            : m_boxAlgorithmCtx(ctxRef) { m_boxAlgorithmCtx = ctx; }

        ~CScopedBoxAlgorithm() { m_boxAlgorithmCtx = nullptr; }

    protected:
        Kernel::IBoxAlgorithmContext*& m_boxAlgorithmCtx;
    };

    Kernel::IBoxAlgorithmContext* m_boxAlgorithmCtx = nullptr;
};

template <class TBoxListenerParentClass>
class TBoxListener : public TBoxListenerParentClass
{
public:
    TBoxListener() { }

private:
    virtual bool initialize(Kernel::IBoxListenerContext& ctx)
    {
        CScopedBoxListener scopedBoxListener(m_boxListenerCtx, &ctx);
        return initialize();
    }

    virtual bool uninitialize(Kernel::IBoxListenerContext& ctx)
    {
        CScopedBoxListener scopedBoxListener(m_boxListenerCtx, &ctx);
        return uninitialize();
    }

    virtual void setCallback(const std::function<void(Kernel::EBoxInterfacorType, int)>& callback)
    {
        m_callback = callback;
    }

    virtual bool process(Kernel::IBoxListenerContext& ctx, const Kernel::EBoxModification boxModificationType)
    {
        CScopedBoxListener scopedBoxListener(m_boxListenerCtx, &ctx);
        m_watcherListener=false;
        bool res = false;

        switch (boxModificationType) {
            case Kernel::EBoxModification::Initialized: 
                res = this->onInitialized(m_boxListenerCtx->getBox());
                break;
            case Kernel::EBoxModification::DefaultInitialized:
                res = this->onDefaultInitialized(m_boxListenerCtx->getBox());
                break;
            case Kernel::EBoxModification::NameChanged: 
                res = this->onNameChanged(m_boxListenerCtx->getBox());
                break;
            case Kernel::EBoxModification::IdentifierChanged:
                res = this->onIdentifierChanged(m_boxListenerCtx->getBox());
                break;
            case Kernel::EBoxModification::AlgorithmClassIdentifierChanged:
                res = this->onAlgorithmClassIdentifierChanged(m_boxListenerCtx->getBox());
                break;
            case Kernel::EBoxModification::InputConnected:
                res = this->onInputConnected(m_boxListenerCtx->getBox(), m_boxListenerCtx->getIndex());
                break;
            case Kernel::EBoxModification::InputDisconnected:
                res = this->onInputDisconnected(m_boxListenerCtx->getBox(), m_boxListenerCtx->getIndex());
                break;
            case Kernel::EBoxModification::InputAdded:
                res = this->onInputAdded(m_boxListenerCtx->getBox(), m_boxListenerCtx->getIndex());
                if(!m_watcherListener && m_callback) {
                    m_callback(Kernel::Input, -1);
                };
                break;
            case Kernel::EBoxModification::InputRemoved:
                res = this->onInputRemoved(m_boxListenerCtx->getBox(), m_boxListenerCtx->getIndex());
                if(!m_watcherListener && m_callback) {
                    m_callback(Kernel::Input, m_boxListenerCtx->getIndex());
                };
                break;
            case Kernel::EBoxModification::InputTypeChanged:
                res = this->onInputTypeChanged(m_boxListenerCtx->getBox(), m_boxListenerCtx->getIndex());
                if(!m_watcherListener && m_callback) {
                    m_callback(Kernel::Input, m_boxListenerCtx->getIndex());
                };
                break;
            case Kernel::EBoxModification::InputNameChanged:
                res = this->onInputNameChanged(m_boxListenerCtx->getBox(), m_boxListenerCtx->getIndex());
                if(!m_watcherListener && m_callback) {
                    m_callback(Kernel::Input, m_boxListenerCtx->getIndex());
                };
                break;
            case Kernel::EBoxModification::OutputConnected:
                res = this->onOutputConnected(m_boxListenerCtx->getBox(), m_boxListenerCtx->getIndex());
                break;
            case Kernel::EBoxModification::OutputDisconnected:
                res = this->onOutputDisconnected(m_boxListenerCtx->getBox(), m_boxListenerCtx->getIndex());
                break;
            case Kernel::EBoxModification::OutputAdded:
                res = this->onOutputAdded(m_boxListenerCtx->getBox(), m_boxListenerCtx->getIndex());
                if(!m_watcherListener && m_callback) {
                    m_callback(Kernel::Output, -1);
                };
                break;
            case Kernel::EBoxModification::OutputRemoved:
                res = this->onOutputRemoved(m_boxListenerCtx->getBox(), m_boxListenerCtx->getIndex());
                if(!m_watcherListener && m_callback) {
                    m_callback(Kernel::Output, m_boxListenerCtx->getIndex());
                };
                break;
            case Kernel::EBoxModification::OutputTypeChanged:
                res = this->onOutputTypeChanged(m_boxListenerCtx->getBox(), m_boxListenerCtx->getIndex());
                if(!m_watcherListener && m_callback) {
                    m_callback(Kernel::Output, m_boxListenerCtx->getIndex());
                };
                break;
            case Kernel::EBoxModification::OutputNameChanged:
                res = this->onOutputNameChanged(m_boxListenerCtx->getBox(), m_boxListenerCtx->getIndex());
                if(!m_watcherListener && m_callback) {
                    m_callback(Kernel::Output, m_boxListenerCtx->getIndex());
                };
                break;
            case Kernel::EBoxModification::SettingAdded:
                res = this->onSettingAdded(m_boxListenerCtx->getBox(), m_boxListenerCtx->getIndex());
                if(!m_watcherListener && m_callback) {
                    m_callback(Kernel::Setting, -1);
                };
                break;
            case Kernel::EBoxModification::SettingRemoved:
                res = this->onSettingRemoved(m_boxListenerCtx->getBox(), m_boxListenerCtx->getIndex());
                if(!m_watcherListener && m_callback) {
                    m_callback(Kernel::Setting, m_boxListenerCtx->getIndex());
                };
                break;
            case Kernel::EBoxModification::SettingTypeChanged:
                res = this->onSettingTypeChanged(m_boxListenerCtx->getBox(), m_boxListenerCtx->getIndex());
                if(!m_watcherListener && m_callback) {
                    m_callback(Kernel::Setting, m_boxListenerCtx->getIndex());
                };
                break;
            case Kernel::EBoxModification::SettingNameChanged:
                res = this->onSettingNameChanged(m_boxListenerCtx->getBox(), m_boxListenerCtx->getIndex());
                if(!m_watcherListener && m_callback) {
                    m_callback(Kernel::Setting, m_boxListenerCtx->getIndex());
                };
                break;
            case Kernel::EBoxModification::SettingDefaultValueChanged:
                res = this->onSettingDefaultValueChanged(m_boxListenerCtx->getBox(), m_boxListenerCtx->getIndex());
                if(!m_watcherListener && m_callback) {
                    m_callback(Kernel::Setting, m_boxListenerCtx->getIndex());
                };
                break;
            case Kernel::EBoxModification::SettingValueChanged: 
                res = this->onSettingValueChanged(m_boxListenerCtx->getBox(), m_boxListenerCtx->getIndex());
                if(!m_watcherListener && m_callback) {
                    m_callback(Kernel::Setting, m_boxListenerCtx->getIndex());
                };
                break;
            default: OV_ERROR_KRF("Unhandled box modification type " << size_t(boxModificationType), Kernel::ErrorType::BadArgument);
        }
        return res;
    }

    // ====================================================================================================================================

public:
    virtual bool initialize() { return true; }
    virtual bool uninitialize() { return true; }
    virtual bool onInitialized(Kernel::IBox& /*box*/) { return true; }
    virtual bool onDefaultInitialized(Kernel::IBox& /*box*/) { return true; }
    virtual bool onNameChanged(Kernel::IBox& /*box*/) { return true; }
    virtual bool onIdentifierChanged(Kernel::IBox& /*box*/) { return true; }
    virtual bool onAlgorithmClassIdentifierChanged(Kernel::IBox& /*box*/) { return true; }
    virtual bool onInputConnected(Kernel::IBox& /*box*/, const size_t /*index*/) { return true; }
    virtual bool onInputDisconnected(Kernel::IBox& /*box*/, const size_t /*index*/) { return true; }
    virtual bool onInputAdded(Kernel::IBox& /*box*/, const size_t /*index*/) {m_watcherListener=true; return true; }
    virtual bool onInputRemoved(Kernel::IBox& /*box*/, const size_t /*index*/) {m_watcherListener=true; return true; }
    virtual bool onInputTypeChanged(Kernel::IBox& /*box*/, const size_t /*index*/) {m_watcherListener=true; return true; }
    virtual bool onInputNameChanged(Kernel::IBox& /*box*/, const size_t /*index*/) {m_watcherListener=true; return true; }
    virtual bool onOutputConnected(Kernel::IBox& /*box*/, const size_t /*index*/) { return true; }
    virtual bool onOutputDisconnected(Kernel::IBox& /*box*/, const size_t /*index*/) { return true; }
    virtual bool onOutputAdded(Kernel::IBox& /*box*/, const size_t /*index*/) {m_watcherListener=true; return true; }
    virtual bool onOutputRemoved(Kernel::IBox& /*box*/, const size_t /*index*/) {m_watcherListener=true; return true; }
    virtual bool onOutputTypeChanged(Kernel::IBox& /*box*/, const size_t /*index*/) {m_watcherListener=true; return true; }
    virtual bool onOutputNameChanged(Kernel::IBox& /*box*/, const size_t /*index*/) {m_watcherListener=true; return true; }
    virtual bool onSettingAdded(Kernel::IBox& /*box*/, const size_t /*index*/) {m_watcherListener=true; return true; }
    virtual bool onSettingRemoved(Kernel::IBox& /*box*/, const size_t /*index*/) {m_watcherListener=true; return true; }
    virtual bool onSettingTypeChanged(Kernel::IBox& /*box*/, const size_t /*index*/) {m_watcherListener=true; return true; }
    virtual bool onSettingNameChanged(Kernel::IBox& /*box*/, const size_t /*index*/) {m_watcherListener=true; return true; }
    virtual bool onSettingDefaultValueChanged(Kernel::IBox& /*box*/, const size_t /*index*/) {m_watcherListener=true; return true; }
    virtual bool onSettingValueChanged(Kernel::IBox& /*box*/, const size_t /*index*/) {m_watcherListener=true; return true; }

    // ====================================================================================================================================

    virtual Kernel::IAlgorithmManager& getAlgorithmManager() const { return m_boxListenerCtx->getAlgorithmManager(); }
    virtual Kernel::IPlayerManager& getPlayerManager() const { return m_boxListenerCtx->getPlayerManager(); }
    virtual Kernel::IPluginManager& getPluginManager() const { return m_boxListenerCtx->getPluginManager(); }
    virtual Kernel::IMetaboxManager& getMetaboxManager() const { return m_boxListenerCtx->getMetaboxManager(); }
    virtual Kernel::IScenarioManager& getScenarioManager() const { return m_boxListenerCtx->getScenarioManager(); }
    virtual Kernel::ITypeManager& getTypeManager() const { return m_boxListenerCtx->getTypeManager(); }
    virtual Kernel::ILogManager& getLogManager() const { return m_boxListenerCtx->getLogManager(); }
    virtual Kernel::CErrorManager& getErrorManager() const { return m_boxListenerCtx->getErrorManager(); }
    virtual Kernel::IConfigurationManager& getConfigurationManager() const { return m_boxListenerCtx->getConfigurationManager(); }

    virtual Kernel::IScenario& getScenario() const { return m_boxListenerCtx->getScenario(); }

    // ====================================================================================================================================

    _IsDerivedFromClass_(TBoxListenerParentClass, OVTK_ClassId_)

private:
    class CScopedBoxListener final
    {
    public:
        CScopedBoxListener(Kernel::IBoxListenerContext*& ctxRef, Kernel::IBoxListenerContext* ctx)
            : m_boxListenerCtx(ctxRef) { m_boxListenerCtx = ctx; }

        ~CScopedBoxListener() { m_boxListenerCtx = nullptr; }

    protected:
        Kernel::IBoxListenerContext*& m_boxListenerCtx;
    };

    Kernel::IBoxListenerContext* m_boxListenerCtx = nullptr;
    bool m_watcherListener=false; //check if their is a listener doing creepy things
    std::function<void(Kernel::EBoxInterfacorType, int)> m_callback = nullptr;
};
}  // namespace Toolkit
}  // namespace OpenViBE
