#include "ovkCKernelContext.h"
#include "ovkCKernelObjectFactory.h"
#include "ovkCTypeManager.h"

#include <openvibe/CIdentifier.hpp>

#include "algorithm/ovkCAlgorithmManager.h"
#include "configuration/ovkCConfigurationManager.h"
#include "player/ovkCPlayerManager.h"
#include "plugins/ovkCPluginManager.h"
#include "metabox/ovkCMetaboxManager.h"
#include "scenario/ovkCScenarioManager.h"
#include "log/ovkCLogManager.h"
#include "log/ovkCLogListenerConsole.h"
#include "log/ovkCLogListenerFile.h"

#include <cassert>
#include <string>
#include <algorithm>
#include <functional>
#include <cctype>

#include <fs/Files.h>

namespace OpenViBE {
namespace Kernel {

CKernelContext::CKernelContext(const IKernelContext* masterKernelCtx, const CString& applicationName, const CString& configFile)
    : m_masterKernelCtx(masterKernelCtx ? *masterKernelCtx : *this), m_algorithmManager(nullptr), m_configManager(nullptr),
      m_kernelObjectFactory(nullptr), m_playerManager(nullptr), m_pluginManager(nullptr), m_metaboxManager(nullptr), m_scenarioManager(nullptr),
      m_typeManager(nullptr), m_logManager(nullptr), m_errorManager(nullptr), m_applicationName(applicationName), m_configFile(configFile),
      m_logListenerConsole(nullptr), m_logListenerFile(nullptr) {}

CKernelContext::~CKernelContext() { this->uninitialize(); }

bool CKernelContext::initialize(const char* const* tokenList, size_t nToken)
{
    std::map<std::string, std::string> initializationTokens;
    const auto tokens = tokenList;
    while (tokens && nToken > 0) {
        const auto key   = tokenList++;
        const auto value = tokenList++;
        nToken--;
        initializationTokens[*key] = *value;
    }

    m_errorManager.reset(new CErrorManager());

    m_kernelObjectFactory.reset(new CKernelObjectFactory(m_masterKernelCtx));

    m_logManager.reset(new CLogManager(m_masterKernelCtx));
    m_logManager->activate(true);

    m_configManager.reset(new CConfigurationManager(m_masterKernelCtx));
    // We create the configuration manager very soon to be able to deactivate the console log listener
    if (initializationTokens.count("Kernel_SilentConsole")) {
        m_configManager->createConfigurationToken("Kernel_SilentConsole", initializationTokens.at("Kernel_SilentConsole").c_str());
    }

    if (!m_configManager->expandAsBoolean("${Kernel_SilentConsole}", false)) {
        m_logListenerConsole.reset(new CLogListenerConsole(m_masterKernelCtx, m_applicationName));
        m_logListenerConsole->activate(false);
        m_logListenerConsole->activate(LogLevel_Info, LogLevel_Last, true);
        this->getLogManager().addListener(m_logListenerConsole.get());
    }


    m_configManager->createConfigurationToken("ApplicationName", m_applicationName);
    m_configManager->createConfigurationToken("Path_UserData", Directories::getUserDataDir());
    m_configManager->createConfigurationToken("Path_Log", Directories::getLogDir());
    m_configManager->createConfigurationToken("Path_Tmp", "${Path_UserData}/tmp");
    m_configManager->createConfigurationToken("Path_Lib", Directories::getLibDir());
    m_configManager->createConfigurationToken("Path_Bin", Directories::getBinDir());
    m_configManager->createConfigurationToken("Path_Data", Directories::getDataDir());

#if defined TARGET_OS_Windows
    m_configManager->createConfigurationToken("OperatingSystem", "Windows");
#elif defined TARGET_OS_Linux
    m_configManager->createConfigurationToken("OperatingSystem", "Linux");
#elif defined TARGET_OS_MacOS
    m_configManager->createConfigurationToken("OperatingSystem", "MacOS");
#else
    m_configManager->createConfigurationToken("OperatingSystem", "Unknown");
#endif

    m_configManager->createConfigurationToken("Kernel_PluginsPatternMacOS", "libopenvibe-plugins-*.dylib");
    m_configManager->createConfigurationToken("Kernel_PluginsPatternLinux", "libopenvibe-plugins-*.so");
    m_configManager->createConfigurationToken("Kernel_PluginsPatternWindows", "openvibe-plugins-*.dll");
    m_configManager->createConfigurationToken("Kernel_Plugins", "${Path_Lib}/${Kernel_PluginsPattern${OperatingSystem}}");

    m_configManager->createConfigurationToken("Kernel_Metabox", "${Path_Data}/metaboxes/;${Path_UserData}/metaboxes/");

    m_configManager->createConfigurationToken("Kernel_MainLogLevel", "Debug");
    m_configManager->createConfigurationToken("Kernel_ConsoleLogLevel", "Information");
    m_configManager->createConfigurationToken("Kernel_FileLogLevel", "Debug");
    m_configManager->createConfigurationToken("Kernel_PlayerFrequency", "128");
    // Add this two tokens to be used to know what documentation should be loaded
    m_configManager->createConfigurationToken("Brand_Name", BRAND_NAME);
    m_configManager->createConfigurationToken("Application_Name", OV_PROJECT_NAME);
    m_configManager->createConfigurationToken("Application_Version", OV_VERSION_FULL);

    for (auto& token : initializationTokens) { m_configManager->addOrReplaceConfigurationToken(token.first.c_str(), token.second.c_str()); }

    this->getLogManager() << LogLevel_Info << "Adding kernel configuration file [" << m_configFile << "]\n";

    OV_ERROR_UNLESS_KRF(m_configManager->addConfigurationFromFile(m_configFile),
                        "Problem parsing config file [" << m_configFile << "]", Kernel::ErrorType::Internal);

    CString pathTmp = m_configManager->expand("${Path_UserData}");
    FS::Files::createPath(pathTmp.toASCIIString());
    pathTmp = m_configManager->expand("${Path_Tmp}");
    FS::Files::createPath(pathTmp.toASCIIString());
    pathTmp = m_configManager->expand("${Path_Log}");
    FS::Files::createPath(pathTmp);
    const CString logFile = pathTmp + "/openvibe-" + m_applicationName + ".log";

    // We do this here to allow user to set the Path_Log in the .conf. The downside is that earlier log messages will not appear in the file log.
    m_logListenerFile.reset(new CLogListenerFile(m_masterKernelCtx, m_applicationName, logFile));
    m_logListenerFile->activate(true);
    this->getLogManager().addListener(m_logListenerFile.get());

    const ELogLevel mainLogLevel    = this->earlyGetLogLevel(m_configManager->expand("${Kernel_MainLogLevel}"));
    const ELogLevel consoleLogLevel = this->earlyGetLogLevel(m_configManager->expand("${Kernel_ConsoleLogLevel}"));
    const ELogLevel fileLogLevel    = this->earlyGetLogLevel(m_configManager->expand("${Kernel_FileLogLevel}"));

    m_logManager->activate(false);
    m_logManager->activate(mainLogLevel, LogLevel_Last, true);
    m_logListenerFile->activate(false);
    m_logListenerFile->activate(fileLogLevel, LogLevel_Last, true);
    m_logListenerFile->configure(*m_configManager);
    m_logListenerFile->configure(*m_configManager);

    if (m_logListenerConsole) {
        m_logListenerConsole->activate(false);
        m_logListenerConsole->activate(consoleLogLevel, LogLevel_Last, true);
        m_logListenerConsole->configure(*m_configManager);
    }

    m_algorithmManager.reset(new CAlgorithmManager(m_masterKernelCtx));
    m_playerManager.reset(new CPlayerManager(m_masterKernelCtx));
    m_typeManager.reset(new CTypeManager(m_masterKernelCtx));

    m_typeManager->registerType(OV_TypeId_Boolean, "Boolean");
    m_typeManager->registerType(OV_TypeId_Integer, "Integer");
    m_typeManager->registerType(OV_TypeId_Float, "Float");
    m_typeManager->registerType(OV_TypeId_String, "String");
    m_typeManager->registerType(OV_TypeId_Filename, "Filename");
    m_typeManager->registerType(OV_TypeId_Script, "Script");
    m_typeManager->registerType(OV_TypeId_Color, "Color");
    m_typeManager->registerType(OV_TypeId_ColorGradient, "Color Gradient");

    m_typeManager->registerEnumerationType(OV_TypeId_Stimulation, "Stimulation");

    m_typeManager->registerEnumerationType(OV_TypeId_MeasurementUnit, "Measurement unit");
    m_typeManager->registerEnumerationType(OV_TypeId_Factor, "Factor");

    m_typeManager->registerEnumerationType(OV_TypeId_LogLevel, "Log level");
    m_typeManager->registerEnumerationEntry(OV_TypeId_LogLevel, "None", LogLevel_None);
    m_typeManager->registerEnumerationEntry(OV_TypeId_LogLevel, "Debug", LogLevel_Debug);
    m_typeManager->registerEnumerationEntry(OV_TypeId_LogLevel, "Benchmarking / Profiling", LogLevel_Benchmark);
    m_typeManager->registerEnumerationEntry(OV_TypeId_LogLevel, "Trace", LogLevel_Trace);
    m_typeManager->registerEnumerationEntry(OV_TypeId_LogLevel, "Information", LogLevel_Info);
    m_typeManager->registerEnumerationEntry(OV_TypeId_LogLevel, "Warning", LogLevel_Warning);
    m_typeManager->registerEnumerationEntry(OV_TypeId_LogLevel, "Important warning", LogLevel_ImportantWarning);
    m_typeManager->registerEnumerationEntry(OV_TypeId_LogLevel, "Error", LogLevel_Error);
    m_typeManager->registerEnumerationEntry(OV_TypeId_LogLevel, "Fatal error", LogLevel_Fatal);

    m_typeManager->registerStreamType(OV_TypeId_EBMLStream, "EBML stream", CIdentifier::undefined());
    m_typeManager->registerStreamType(OV_TypeId_ExperimentInfo, "Experiment information", OV_TypeId_EBMLStream);
    m_typeManager->registerStreamType(OV_TypeId_Stimulations, "Stimulations", OV_TypeId_EBMLStream);
    m_typeManager->registerStreamType(OV_TypeId_StreamedMatrix, "Streamed matrix", OV_TypeId_EBMLStream);
    m_typeManager->registerStreamType(OV_TypeId_ChannelLocalisation, "Channel localisation", OV_TypeId_StreamedMatrix);
    m_typeManager->registerStreamType(OV_TypeId_ChannelUnits, "Channel units", OV_TypeId_StreamedMatrix);
    m_typeManager->registerStreamType(OV_TypeId_FeatureVector, "Feature vector", OV_TypeId_StreamedMatrix);
    m_typeManager->registerStreamType(OV_TypeId_Signal, "Signal", OV_TypeId_StreamedMatrix);
    m_typeManager->registerStreamType(OV_TypeId_Spectrum, "Spectrum", OV_TypeId_StreamedMatrix);
    m_typeManager->registerStreamType(OV_TypeId_TimeFrequency, "Time-frequency", OV_TypeId_StreamedMatrix);
    m_typeManager->registerStreamType(OV_TypeId_CovarianceMatrix, "Covariance Matrix", OV_TypeId_StreamedMatrix);


    m_typeManager->registerType(OV_TypeId_Message, "Message");

    m_scenarioManager.reset(new CScenarioManager(m_masterKernelCtx));
    m_pluginManager.reset(new CPluginManager(m_masterKernelCtx));
    m_metaboxManager.reset(new CMetaboxManager(m_masterKernelCtx));

    return true;
}

bool CKernelContext::uninitialize()
{
    // As releaseScenario() can call into Plugin Manager we have to clear the scenario manager
    // before destroying the Plugin Manager. We can not destroy the Scenario Manager first
    // before Plugin Manager destructor needs it.
    CIdentifier scenarioID;
    while ((scenarioID = m_scenarioManager->getNextScenarioIdentifier(CIdentifier::undefined())) != CIdentifier::undefined()) {
        m_scenarioManager->releaseScenario(scenarioID);
    }

    CIdentifier algorithmIdentifier;
    while ((algorithmIdentifier = m_algorithmManager->getNextAlgorithmIdentifier(CIdentifier::undefined())) != CIdentifier::undefined()) {
        m_algorithmManager->releaseAlgorithm(algorithmIdentifier);
    }

    m_pluginManager.reset();
    m_metaboxManager.reset();
    m_scenarioManager.reset();
    m_typeManager.reset();
    m_playerManager.reset();
    m_algorithmManager.reset();
    m_configManager.reset();

    this->getLogManager().removeListener(m_logListenerConsole.get());
    this->getLogManager().removeListener(m_logListenerFile.get());

    m_logManager.reset();
    m_logListenerConsole.reset();
    m_logListenerFile.reset();

    m_kernelObjectFactory.reset();

    m_errorManager.reset();

    return true;
}

IAlgorithmManager& CKernelContext::getAlgorithmManager() const
{
    assert(m_algorithmManager);
    return *m_algorithmManager;
}

IConfigurationManager& CKernelContext::getConfigurationManager() const
{
    assert(m_configManager);
    return *m_configManager;
}

IKernelObjectFactory& CKernelContext::getKernelObjectFactory() const
{
    assert(m_kernelObjectFactory);
    return *m_kernelObjectFactory;
}

IPlayerManager& CKernelContext::getPlayerManager() const
{
    assert(m_playerManager);
    return *m_playerManager;
}

IPluginManager& CKernelContext::getPluginManager() const
{
    assert(m_pluginManager);
    return *m_pluginManager;
}

IMetaboxManager& CKernelContext::getMetaboxManager() const
{
    assert(m_metaboxManager);
    return *m_metaboxManager;
}


IScenarioManager& CKernelContext::getScenarioManager() const
{
    assert(m_scenarioManager);
    return *m_scenarioManager;
}

ITypeManager& CKernelContext::getTypeManager() const
{
    assert(m_typeManager);
    return *m_typeManager;
}

ILogManager& CKernelContext::getLogManager() const
{
    assert(m_logManager);
    return *m_logManager;
}

CErrorManager& CKernelContext::getErrorManager() const
{
    assert(m_errorManager);
    return *m_errorManager;
}

ELogLevel CKernelContext::earlyGetLogLevel(const CString& rLogLevelName)
{
    assert(m_logManager);

    std::string value(rLogLevelName.toASCIIString());
    std::transform(value.begin(), value.end(), value.begin(), [](const char c) { return char(std::tolower(c)); });

    if (value == "none") { return LogLevel_None; }
    if (value == "debug") { return LogLevel_Debug; }
    if (value == "benchmarking / profiling") { return LogLevel_Benchmark; }
    if (value == "trace") { return LogLevel_Trace; }
    if (value == "information") { return LogLevel_Info; }
    if (value == "warning") { return LogLevel_Warning; }
    if (value == "important warning") { return LogLevel_ImportantWarning; }
    if (value == "error") { return LogLevel_Error; }
    if (value == "fatal error") { return LogLevel_Fatal; }

    OV_WARNING("Invalid log level " << rLogLevelName << " specified in configuration file, falling back to " << CString("Debug"), (*m_logManager));

    return LogLevel_Debug;
}

}  // namespace Kernel
}  // namespace OpenViBE
