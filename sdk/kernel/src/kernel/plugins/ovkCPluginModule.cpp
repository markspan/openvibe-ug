#include "ovkCPluginModule.h"

#include <map>
#include <vector>

#if defined TARGET_OS_Linux || defined TARGET_OS_MacOS
    #include <dlfcn.h>
#elif defined TARGET_OS_Windows
#include <windows.h>
#else
#endif

namespace OpenViBE {
namespace Kernel {
class CPluginModuleBase : public TKernelObject<IPluginModule>
{
public:
    explicit CPluginModuleBase(const IKernelContext& ctx)
        : TKernelObject<IPluginModule>(ctx), m_onInitializeCB(nullptr), m_onGetPluginObjectDescCB(nullptr), m_onUninitializeCB(nullptr) {}

    ~CPluginModuleBase() override { }
    bool initialize() override;
    bool getPluginObjectDescription(size_t index, Plugins::IPluginObjectDesc*& pluginObjectDesc) override;
    bool uninitialize() override;
    bool getFileName(CString& fileName) const override;

    _IsDerivedFromClass_Final_(TKernelObject<IPluginModule>, CIdentifier::undefined())

protected:

    virtual bool isOpen() const = 0;

    std::vector<Plugins::IPluginObjectDesc*> m_pluginObjectDescs;
    CString m_filename;
    bool m_gotDesc = false;

    bool (*m_onInitializeCB)(const IPluginModuleContext&);
    bool (*m_onGetPluginObjectDescCB)(const IPluginModuleContext&, size_t, Plugins::IPluginObjectDesc*&);
    bool (*m_onUninitializeCB)(const IPluginModuleContext&);
};

namespace {
class CPluginModuleContext final : public TKernelObject<IPluginModuleContext>
{
public:

    explicit CPluginModuleContext(const IKernelContext& ctx)
        : TKernelObject<IPluginModuleContext>(ctx), m_logManager(ctx.getLogManager()), m_typeManager(ctx.getTypeManager()),
          m_scenarioManager(ctx.getScenarioManager()) { }

    ILogManager& getLogManager() const override { return m_logManager; }
    ITypeManager& getTypeManager() const override { return m_typeManager; }
    IScenarioManager& getScenarioManager() const override { return m_scenarioManager; }

    _IsDerivedFromClass_Final_(TKernelObject<IPluginModuleContext>, OVK_ClassId_Kernel_Plugins_PluginModuleContext)

protected:

    ILogManager& m_logManager;
    ITypeManager& m_typeManager;
    IScenarioManager& m_scenarioManager;
};
}  // namespace

bool CPluginModuleBase::initialize()
{
    if (!isOpen()) { return false; }
    if (!m_onInitializeCB) { return true; }
    return m_onInitializeCB(CPluginModuleContext(getKernelContext()));
}

bool CPluginModuleBase::getPluginObjectDescription(const size_t index, Plugins::IPluginObjectDesc*& pluginObjectDesc)
{
    if (!m_gotDesc)
    {
        if (!isOpen() || !m_onGetPluginObjectDescCB) { return false; }

        size_t idx                      = 0;
        Plugins::IPluginObjectDesc* pod = nullptr;
        while (m_onGetPluginObjectDescCB(CPluginModuleContext(getKernelContext()), idx, pod))
        {
            if (pod) { m_pluginObjectDescs.push_back(pod); }
            idx++;
        }

        m_gotDesc = true;
    }

    if (index >= m_pluginObjectDescs.size())
    {
        pluginObjectDesc = nullptr;
        return false;
    }

    pluginObjectDesc = m_pluginObjectDescs[index];
    return true;
}

bool CPluginModuleBase::uninitialize()
{
    if (!isOpen()) { return false; }
    if (!m_onUninitializeCB) { return true; }
    return m_onUninitializeCB(CPluginModuleContext(getKernelContext()));
}

bool CPluginModuleBase::getFileName(CString& fileName) const
{
    if (!isOpen()) { return false; }
    fileName = m_filename;
    return true;
}

#if defined TARGET_OS_Linux || defined TARGET_OS_MacOS
class CPluginModuleLinux : public CPluginModuleBase
{
public:

    CPluginModuleLinux(const IKernelContext& ctx);

    virtual bool load(const CString& filename, CString* pError);
    virtual bool unload(CString* pError);
    virtual bool isOpen() const;

protected:

    void* m_fileHandle;
};

#elif defined TARGET_OS_Windows
class CPluginModuleWindows final : public CPluginModuleBase
{
public:

    explicit CPluginModuleWindows(const IKernelContext& ctx);
    bool load(const CString& filename, CString* pError) override;
    bool unload(CString* pError) override;

protected:
    bool isOpen() const override { return m_fileHandle != nullptr; }

    HMODULE m_fileHandle;

private:

    static CString getLastErrorMessageString();
};

#else
class CPluginModuleDummy : public CPluginModuleBase
{
public:

    explicit CPluginModuleDummy(const IKernelContext& ctx);

    virtual bool load(const CString& filename, CString* pError);
    virtual bool unload(CString* pError);

protected:

    virtual bool isOpen() const;
};
#endif

// 
//___________________________________________________________________//
//                                                                   //

#if defined TARGET_OS_Linux || defined TARGET_OS_MacOS

CPluginModuleLinux::CPluginModuleLinux(const IKernelContext& ctx) :CPluginModuleBase(ctx),m_fileHandle(NULL) { }

bool CPluginModuleLinux::load(const CString& filename, CString* pError)
{
    if(m_fileHandle)
    {
        if(pError) *pError="plugin module already loaded";
        return false;
    }

    // m_fileHandle=dlopen(filename, RTLD_NOW|RTLD_LOCAL);
#if defined OV_LOCAL_SYMBOLS
    m_fileHandle = dlopen(filename, RTLD_LAZY|RTLD_LOCAL);
#else
    m_fileHandle = dlopen(filename, RTLD_LAZY|RTLD_GLOBAL);
#endif
    if(!m_fileHandle)
    {
        if(pError) *pError=dlerror();
        return false;
    }

    m_onInitializeCB=(bool (*)(const IPluginModuleContext&))dlsym(m_fileHandle, "onInitialize");
    m_onUninitializeCB=(bool (*)(const IPluginModuleContext&))dlsym(m_fileHandle, "onUninitialize");
    m_onGetPluginObjectDescCB=(bool (*)(const IPluginModuleContext&, size_t, Plugins::IPluginObjectDesc*&))dlsym(m_fileHandle, "onGetPluginObjectDescription");

    if(!m_onGetPluginObjectDescCB)
    {
        if(pError) *pError=dlerror();

        dlclose(m_fileHandle);
        m_fileHandle=NULL;
        m_onInitializeCB=NULL;
        m_onUninitializeCB=NULL;
        m_onGetPluginObjectDescCB=NULL;
        return false;
    }

    m_filename=filename;
    return true;
}

bool CPluginModuleLinux::unload(CString* error)
{
    if(!m_fileHandle)
    {
        if (error) { *error = "no plugin module currently loaded"; }
        return false;
    }

    dlclose(m_fileHandle);
    m_fileHandle              = NULL;
    m_onInitializeCB          = NULL;
    m_onUninitializeCB        = NULL;
    m_onGetPluginObjectDescCB = NULL;
    return true;
}

bool CPluginModuleLinux::isOpen() const { return m_fileHandle != nullptr; }

#elif defined TARGET_OS_Windows

CPluginModuleWindows::CPluginModuleWindows(const IKernelContext& ctx) : CPluginModuleBase(ctx), m_fileHandle(nullptr) {}

bool CPluginModuleWindows::load(const CString& filename, CString* pError)
{
    if (m_fileHandle)
    {
        if (pError) { *pError = "plugin module already loaded"; }
        return false;
    }

    m_fileHandle = LoadLibrary(filename);
    if (!m_fileHandle)
    {
        if (pError) { *pError = this->getLastErrorMessageString(); }
        return false;
    }

    m_onInitializeCB          = reinterpret_cast<bool (*)(const IPluginModuleContext&)>(GetProcAddress(m_fileHandle, "onInitialize"));
    m_onUninitializeCB        = reinterpret_cast<bool (*)(const IPluginModuleContext&)>(GetProcAddress(m_fileHandle, "onUninitialize"));
    m_onGetPluginObjectDescCB = reinterpret_cast<bool (*)(const IPluginModuleContext&, size_t, Plugins::IPluginObjectDesc*&)>(GetProcAddress(
        m_fileHandle, "onGetPluginObjectDescription"));
    if (!m_onGetPluginObjectDescCB)
    {
        if (pError) { *pError = this->getLastErrorMessageString(); }

        FreeLibrary(m_fileHandle);
        m_fileHandle              = nullptr;
        m_onInitializeCB          = nullptr;
        m_onGetPluginObjectDescCB = nullptr;
        m_onUninitializeCB        = nullptr;
        return false;
    }

    m_filename = filename;
    return true;
}

bool CPluginModuleWindows::unload(CString* pError)
{
    if (!m_fileHandle)
    {
        if (pError) { *pError = "no plugin module currently loaded"; }
        return false;
    }

    FreeLibrary(m_fileHandle);
    m_fileHandle              = nullptr;
    m_onInitializeCB          = nullptr;
    m_onGetPluginObjectDescCB = nullptr;
    m_onUninitializeCB        = nullptr;
    return true;
}

CString CPluginModuleWindows::getLastErrorMessageString()
{
    CString res;

    char* buffer = nullptr;
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, nullptr, GetLastError(), 0,
                  reinterpret_cast<LPTSTR>(&buffer), 0, nullptr);
    if (buffer)
    {
        const size_t length = strlen(buffer);
        for (size_t i = 0; i < length; ++i) { if (buffer[i] == '\n' || buffer[i] == '\r') { buffer[i] = ' '; } }
        res = buffer;
    }
    LocalFree(LPVOID(buffer));

    return res;
}

#else

#endif

//___________________________________________________________________//
//                                                                   //

CPluginModule::CPluginModule(const IKernelContext& ctx)
    : TKernelObject<IPluginModule>(ctx)
{
#if defined TARGET_OS_Linux || defined TARGET_OS_MacOS
    m_impl=new CPluginModuleLinux(getKernelContext());
#elif defined TARGET_OS_Windows
    m_impl = new CPluginModuleWindows(getKernelContext());
#else
#endif
}

CPluginModule::~CPluginModule() { delete m_impl; }

bool CPluginModule::load(const CString& filename, CString* error) { return !m_impl ? false : m_impl->load(filename, error); }
bool CPluginModule::unload(CString* error) { return !m_impl ? false : m_impl->unload(error); }
bool CPluginModule::initialize() { return !m_impl ? false : m_impl->initialize(); }

bool CPluginModule::getPluginObjectDescription(const size_t index, Plugins::IPluginObjectDesc*& desc)
{
    return !m_impl ? false : m_impl->getPluginObjectDescription(index, desc);
}

bool CPluginModule::uninitialize() { return !m_impl ? false : m_impl->uninitialize(); }
bool CPluginModule::getFileName(CString& rFileName) const { return !m_impl ? false : m_impl->getFileName(rFileName); }

}  // namespace Kernel
}  // namespace OpenViBE
