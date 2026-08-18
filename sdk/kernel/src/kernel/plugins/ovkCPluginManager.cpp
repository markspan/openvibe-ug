#include "ovkCPluginManager.h"
#include "ovkCPluginModule.h"

#include <fs/IEntryEnumerator.h>
#include <fs/Files.h>

#include <cinttypes>
#include <cstdio>
#include <iostream>
#include <map>
#include <algorithm>

#include "../../tools/ovkSBoxProto.h"

namespace OpenViBE {
namespace Kernel {
class CPluginManagerEntryEnumeratorCallBack final : public TKernelObject<IObject>, public FS::IEntryEnumeratorCallBack
{
public:

    CPluginManagerEntryEnumeratorCallBack(const IKernelContext& ctx, std::vector<IPluginModule*>& pluginModule,
                                          std::map<Plugins::IPluginObjectDesc*, IPluginModule*>& pluginObjectDesc, bool& haveAllPluginsLoadedCorrectly)
        : TKernelObject<IObject>(ctx), m_rPluginManager(ctx.getPluginManager()), m_rPluginModule(pluginModule),
          m_rPluginObjectDesc(pluginObjectDesc), m_HaveAllPluginsLoadedCorrectly(haveAllPluginsLoadedCorrectly) { }

    bool callback(FS::IEntryEnumerator::IEntry& entry, FS::IEntryEnumerator::IAttributes& /*attributes*/) override
    {
        for (auto& pluginModule : m_rPluginModule)
        {
            CString name;
            if (!pluginModule->getFileName(name)) { return true; }

            if (FS::Files::equals(entry.getName(), name.toASCIIString()))
            {
                OV_WARNING_K(std::string("Module [") + entry.getName() + "] has already been loaded");
                return true;
            }
        }

        IPluginModule* module = new CPluginModule(this->getKernelContext());
        CString loadError;
        if (!module->load(entry.getName(), &loadError))
        {
            delete module;
            OV_WARNING_K(std::string("File [") + entry.getName() + "] is not a plugin module (error:" + loadError.toASCIIString() + ")");
            m_HaveAllPluginsLoadedCorrectly = false;
            return true;
        }

        if (!module->initialize())
        {
            module->uninitialize();
            module->unload();
            delete module;
            OV_WARNING_K(std::string("Module [") + entry.getName() + "] did not initialize correctly");
            m_HaveAllPluginsLoadedCorrectly = false;
            return true;
        }

        bool pluginObjectDescAdded       = false;
        size_t index                     = 0;
        size_t n                         = 0;
        Plugins::IPluginObjectDesc* desc = nullptr;
        while (module->getPluginObjectDescription(index, desc))
        {
            bool found = false;

            for (const auto& pluginObjectDesc : m_rPluginObjectDesc)
            {
                if (pluginObjectDesc.first->getClassIdentifier() == desc->getClassIdentifier())
                {
                    OV_WARNING_K("Duplicate plugin object descriptor class identifier [" + pluginObjectDesc.first->getName()
                        + "] and [" + desc->getName() + "]... second one is ignored");
                    found = true;
                    break;
                }
            }

            if (!found)
            {
                if (!pluginObjectDescAdded)
                {
                    m_rPluginModule.push_back(module);
                    pluginObjectDescAdded = true;
                }
                m_rPluginObjectDesc[desc] = module;
                n++;
            }
            index++;
            desc = nullptr;
        }

        OV_WARNING_UNLESS_K(pluginObjectDescAdded,
                            std::string("No 'plugin object descriptor' found from [") + entry.getName() + "] even if it looked like a plugin module\n");

        this->getLogManager() << LogLevel_Info << "Added " << n << " plugin object descriptor(s) from [" << CString(entry.getName()) << "]\n";

        return true;
    }

    _IsDerivedFromClass_Final_(TKernelObject<IObject>, CIdentifier::undefined())

protected:

    IPluginManager& m_rPluginManager;
    std::vector<IPluginModule*>& m_rPluginModule;
    std::map<Plugins::IPluginObjectDesc*, IPluginModule*>& m_rPluginObjectDesc;
    bool& m_HaveAllPluginsLoadedCorrectly;
};

CPluginManager::~CPluginManager()
{
    std::unique_lock<std::mutex> lock(m_mutex);

    for (auto& pluginObjectVector : m_pluginObjects)
    {
        for (auto& pluginObject : pluginObjectVector.second)
        {
            OV_WARNING_K(
                "Trying to release plugin object with class id " + pluginObject->getClassIdentifier().str() + " and plugin object descriptor "
                + pluginObjectVector.first->getName().toASCIIString() + " at plugin manager destruction time");
            pluginObject->release();
        }
    }
    m_pluginObjects.clear();

    for (auto& pluginObjectDesc : m_pluginObjectDescs) { pluginObjectDesc.first->release(); }
    m_pluginObjectDescs.clear();

    for (auto k = m_pluginModules.begin(); k != m_pluginModules.end(); ++k)
    {
        this->TKernelObject<IPluginManager>::getLogManager() << LogLevel_Trace << "Releasing plugin module with class id " << (*k)->getClassIdentifier() <<
                "\n";
        (*k)->uninitialize();
        delete (*k);
    }
    m_pluginModules.clear();
}

bool CPluginManager::addPluginsFromFiles(const CString& rFileNameWildCard)
{
    std::unique_lock<std::mutex> lock(m_mutex);

    this->getLogManager() << LogLevel_Info << "Adding plugins from [" << rFileNameWildCard << "]\n";

    bool res                           = true;
    bool haveAllPluginsLoadedCorrectly = true;
    CPluginManagerEntryEnumeratorCallBack cb(this->getKernelContext(), m_pluginModules, m_pluginObjectDescs, haveAllPluginsLoadedCorrectly);
    FS::IEntryEnumerator* entryEnumerator = createEntryEnumerator(cb);

    std::stringstream ss(rFileNameWildCard.toASCIIString());
    std::string path;

    while (getline(ss, path, ';'))
    {
        res &= entryEnumerator->enumerate(path.c_str());
        if (!res) { break; }
    }

    entryEnumerator->release();

    // Just return res. Error handling is performed within CPluginManagerEntryEnumeratorCallBack.
    return res && haveAllPluginsLoadedCorrectly;
}

bool CPluginManager::registerPluginDesc(const Plugins::IPluginObjectDesc& rPluginObjectDesc)
{
    std::unique_lock<std::mutex> lock(m_mutex);

    m_pluginObjectDescs[const_cast<Plugins::IPluginObjectDesc*>(&rPluginObjectDesc)] = nullptr;
    return true;
}

CIdentifier CPluginManager::getNextPluginObjectDescIdentifier(const CIdentifier& previousID) const
{
    std::unique_lock<std::mutex> lock(m_mutex);

    bool foundPrevious = (previousID == CIdentifier::undefined());
    for (const auto& elem : m_pluginObjectDescs)
    {
        if (!foundPrevious) { if (elem.first->getClassIdentifier() == previousID) { foundPrevious = true; } }
        else { return elem.first->getClassIdentifier(); }
    }
    return CIdentifier::undefined();
}

CIdentifier CPluginManager::getNextPluginObjectDescIdentifier(const CIdentifier& previousID, const CIdentifier& baseClassID) const
{
    std::unique_lock<std::mutex> lock(m_mutex);

    bool foundPrevious = (previousID == CIdentifier::undefined());
    for (const auto& elem : m_pluginObjectDescs)
    {
        if (!foundPrevious) { if (elem.first->getClassIdentifier() == previousID) { foundPrevious = true; } }
        else { if (elem.first->isDerivedFromClass(baseClassID)) { return elem.first->getClassIdentifier(); } }
    }
    return CIdentifier::undefined();
}

bool CPluginManager::canCreatePluginObject(const CIdentifier& classID)
{
    std::unique_lock<std::mutex> lock(m_mutex);

    // this->getLogManager() << Kernel::LogLevel_Debug << "Searching if can build plugin object\n";

    return std::any_of(m_pluginObjectDescs.begin(), m_pluginObjectDescs.end(), [classID](const std::pair<Plugins::IPluginObjectDesc*, IPluginModule*>& v)
    {
        return v.first->getCreatedClass() == classID;
    });
}

const Plugins::IPluginObjectDesc* CPluginManager::getPluginObjectDesc(const CIdentifier& classID) const
{
    std::unique_lock<std::mutex> lock(m_mutex);

    // this->getLogManager() << Kernel::LogLevel_Debug << "Searching plugin object descriptor\n";

    for (auto& pluginObject : m_pluginObjectDescs) { if (pluginObject.first->getClassIdentifier() == classID) { return pluginObject.first; } }

    this->getLogManager() << LogLevel_Debug << "Plugin object descriptor class identifier " << classID << " not found\n";
    return nullptr;
}

const Plugins::IPluginObjectDesc* CPluginManager::getPluginObjectDescCreating(const CIdentifier& classID) const
{
    std::unique_lock<std::mutex> lock(m_mutex);

    // this->getLogManager() << Kernel::LogLevel_Debug << "Searching plugin object descriptor\n";

    const auto elem = std::find_if(m_pluginObjectDescs.begin(), m_pluginObjectDescs.end(),
                                   [classID](const std::pair<Plugins::IPluginObjectDesc*, IPluginModule*>& v) { return v.first->getCreatedClass() == classID; });
    if (elem != m_pluginObjectDescs.end()) { return elem->first; }
    this->getLogManager() << LogLevel_Debug << "Plugin object descriptor class identifier " << classID << " not found\n";
    return nullptr;
}

CIdentifier CPluginManager::getPluginObjectHashValue(const CIdentifier& classID) const
{
    // std::unique_lock<std::mutex> lock(m_mutex);

    const Plugins::IPluginObjectDesc* pluginObjectDesc = this->getPluginObjectDescCreating(classID);
    const Plugins::IBoxAlgorithmDesc* boxAlgorithmDesc = dynamic_cast<const Plugins::IBoxAlgorithmDesc*>(pluginObjectDesc);
    if (boxAlgorithmDesc)
    {
        SBoxProto prototype(getKernelContext().getTypeManager());
        boxAlgorithmDesc->getBoxPrototype(prototype);
        return prototype.m_hash;
    }
    return CIdentifier::undefined();
}

CIdentifier CPluginManager::getPluginObjectHashValue(const Plugins::IBoxAlgorithmDesc& boxAlgorithmDesc) const
{
    std::unique_lock<std::mutex> lock(m_mutex);

    SBoxProto prototype(getKernelContext().getTypeManager());
    boxAlgorithmDesc.getBoxPrototype(prototype);
    return prototype.m_hash;
}

bool CPluginManager::isPluginObjectFlaggedAsDeprecated(const CIdentifier& classID) const
{
    // std::unique_lock<std::mutex> lock(m_mutex);

    const Plugins::IPluginObjectDesc* pluginObjectDesc = this->getPluginObjectDescCreating(classID);
    const Plugins::IBoxAlgorithmDesc* boxAlgorithmDesc = dynamic_cast<const Plugins::IBoxAlgorithmDesc*>(pluginObjectDesc);
    if (boxAlgorithmDesc)
    {
        SBoxProto prototype(getKernelContext().getTypeManager());
        boxAlgorithmDesc->getBoxPrototype(prototype);
        return prototype.m_isDeprecated;
    }
    return false;
}

Plugins::IPluginObject* CPluginManager::createPluginObject(const CIdentifier& classID)
{
    return createPluginObjectT<Plugins::IPluginObject, Plugins::IPluginObjectDesc>(classID, nullptr);
}

bool CPluginManager::releasePluginObject(Plugins::IPluginObject* pluginObject)
{
    this->getLogManager() << LogLevel_Debug << "Releasing plugin object\n";

    OV_ERROR_UNLESS_KRF(pluginObject, "Plugin object value is null", Kernel::ErrorType::BadProcessing);

    {
        std::unique_lock<std::mutex> lock(m_mutex);

        for (auto& elem : m_pluginObjects)
        {
            auto pluginObjectIt = std::find(elem.second.begin(), elem.second.end(), pluginObject);
            if (pluginObjectIt != elem.second.end())
            {
                elem.second.erase(pluginObjectIt);
                pluginObject->release();
                return true;
            }
        }
    }

    OV_ERROR_KRF("Plugin object has not been created by this plugin manager (class id was " << pluginObject->getClassIdentifier().str() << ")",
                 ErrorType::ResourceNotFound);
}

Plugins::IAlgorithm* CPluginManager::createAlgorithm(const CIdentifier& classID, const Plugins::IAlgorithmDesc** algorithmDesc)
{
    return createPluginObjectT<Plugins::IAlgorithm, Plugins::IAlgorithmDesc>(classID, algorithmDesc);
}

Plugins::IAlgorithm* CPluginManager::createAlgorithm(const Plugins::IAlgorithmDesc& algorithmDesc)
{
    Plugins::IAlgorithmDesc* desc        = const_cast<Plugins::IAlgorithmDesc*>(&algorithmDesc);
    Plugins::IPluginObject* pluginObject = desc->create();

    OV_ERROR_UNLESS_KRN(pluginObject,
                        "Could not create plugin object from " << algorithmDesc.getName() << " plugin object descriptor",
                        ErrorType::BadResourceCreation);

    Plugins::IAlgorithmDesc* pluginObjectDescT = dynamic_cast<Plugins::IAlgorithmDesc*>(desc);
    Plugins::IAlgorithm* pluginObjectT         = dynamic_cast<Plugins::IAlgorithm*>(pluginObject);

    OV_ERROR_UNLESS_KRN(pluginObjectDescT && pluginObjectT,
                        "Could not downcast plugin object and/or plugin object descriptor for " << algorithmDesc.getName() << " plugin object descriptor",
                        ErrorType::BadResourceCreation);

    {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_pluginObjects[pluginObjectDescT].push_back(pluginObjectT);
    }

    return pluginObjectT;
}

Plugins::IBoxAlgorithm* CPluginManager::createBoxAlgorithm(const CIdentifier& classID, const Plugins::IBoxAlgorithmDesc** boxAlgorithmDesc)
{
    return createPluginObjectT<Plugins::IBoxAlgorithm, Plugins::IBoxAlgorithmDesc>(classID, boxAlgorithmDesc);
}

template <class IPluginObjectT, class IPluginObjectDescT>
IPluginObjectT* CPluginManager::createPluginObjectT(const CIdentifier& classID, const IPluginObjectDescT** ppPluginObjectDescT)
{
    std::unique_lock<std::mutex> lock(m_mutex);

    if (ppPluginObjectDescT) { *ppPluginObjectDescT = nullptr; }

    CIdentifier substitutionTokenID;
    char substitutionTokenName[1024];
    const uint64_t srcClassID = classID.id();
    uint64_t dstClassID       = srcClassID;
    sprintf(substitutionTokenName, "Kernel_PluginSubstitution_%0" PRIx64, srcClassID);
    if ((substitutionTokenID = this->getConfigurationManager().lookUpConfigurationTokenIdentifier(substitutionTokenName)) !=
        CIdentifier::undefined())
    {
        CString value = this->getConfigurationManager().getConfigurationTokenValue(substitutionTokenID);
        value         = this->getConfigurationManager().expand(value);

        try { dstClassID = std::stoull(value.toASCIIString(), nullptr, 16); }
        catch (const std::invalid_argument& exception)
        {
            OV_ERROR_KRN("Received exception while converting class identifier from string to number: " << exception.what(), Kernel::ErrorType::BadArgument);
        }
        catch (const std::out_of_range& exception)
        {
            OV_ERROR_KRN("Received exception while converting class identifier from string to number: " << exception.what(), Kernel::ErrorType::OutOfBound);
        }
    }
    if (dstClassID != srcClassID)
    {
        this->getLogManager() << LogLevel_Trace << "Substituting plugin class identifier " << CIdentifier(srcClassID) <<
                " with new class identifier " << CIdentifier(dstClassID) << "\n";
    }
    else
    {
        this->getLogManager() << LogLevel_Debug << "Not substitute plugin found for class identifier " << CIdentifier(srcClassID) <<
                " (configuration token name was " << CString(substitutionTokenName) << ")\n";
    }

    Plugins::IPluginObjectDesc* pod = nullptr;
    for (auto i = m_pluginObjectDescs.begin(); i != m_pluginObjectDescs.end(); ++i)
    {
        if (i->first->getCreatedClass() == CIdentifier(dstClassID)) { pod = i->first; }
    }

    OV_ERROR_UNLESS_KRN(pod,
                        "Did not find the plugin object descriptor with requested class identifier " << CIdentifier(srcClassID).str() <<
                        " in registered plugin object descriptors",
                        ErrorType::BadResourceCreation);

    Plugins::IPluginObject* pluginObject = pod->create();

    OV_ERROR_UNLESS_KRN(pluginObject,
                        "Could not create plugin object from " << pod->getName() << " plugin object descriptor",
                        ErrorType::BadResourceCreation);

    IPluginObjectDescT* pluginObjectDescT = dynamic_cast<IPluginObjectDescT*>(pod);
    IPluginObjectT* pluginObjectT         = dynamic_cast<IPluginObjectT*>(pluginObject);

    OV_ERROR_UNLESS_KRN(pluginObjectDescT && pluginObjectT,
                        "Could not downcast plugin object and/or plugin object descriptor for " << pod->getName() << " plugin object descriptor",
                        ErrorType::BadResourceCreation);

    if (ppPluginObjectDescT) { *ppPluginObjectDescT = pluginObjectDescT; }

    m_pluginObjects[pluginObjectDescT].push_back(pluginObjectT);

    return pluginObjectT;
}

}  // namespace Kernel
}  // namespace OpenViBE
