#include "OVMaster.h"

#include <QQmlContext>
#include <QQmlEngine>

#include <system/ovCTime.h>
#include <vector>

#include <openvibe/CIdentifier.hpp>
#include <ovasCPluginLSLOutput.h>
#include <ovasCPluginFiddler.h>

#include "ovasIDriver.h"
#include "ovasCAcquisitionServer.h"
#include "ovasCAcquisitionServerThread.h"
#include "ovasCDriftCorrection.h"
#include <OVQmlLogListener.h>

#include <openvibe/parameters/parameters.h>
#include <parametersObject.h>

#include "ovasCPluginExternalStimulations.h"
#include "ovasCPluginTCPTagging.h"
//#include "ovasCDriverBrainmasterDiscovery.h"
//#include "ovasCDriverBrainProductsBrainVisionRecorder.h"
//#include "ovasCDriverCognionics.h"
//#include "ovasCDriverCtfVsmMeg.h"
//#include "ovasCDriverEncephalan.h"
//#include "CDriverGTecGUSBamp.hpp"
//#include "ovasCDriverGTecGUSBampLegacy.h"
//#include "ovasCDriverGTecGUSBampLinux.h"
//#include "CDriverGTecUnicorn.hpp"
//#include "ovasCDriverGTecGMobiLabPlus.h"
//#include "ovasCDrivergNautilusInterface.h"
//#include "ovasCDriverMBTSmarting.h"
//#include "ovasCDriverMitsarEEG202A.h"
//#include "ovasCDriverOpenALAudioCapture.h"
//#include "ovasCDriverOpenEEGModularEEG.h"
//#include "ovasCDriverOpenBCI.h"
//#include "ovasCDriverEEGO.h"
//
//#if !defined(WIN32) || defined(TARGET_PLATFORM_i386)
//#include "ovasCDriverFieldtrip.h"

// Simulation drivers
#include "egi-ampserver/ovasCDriverEGIAmpServer.h"
#include "generic-oscillator/ovasCDriverGenericOscillator.h"
#include "generic-time-signal/ovasCDriverGenericTimeSignal.h"
#include "simulated-deviator/ovasCDriverSimulatedDeviator.h"
#include "generic-raw-reader/ovasCDriverGenericRawFileReader.h"
#include "generic-raw-reader/ovasCDriverGenericRawTelnetReader.h"
#include "generic-sawtooth/ovasCDriverGenericSawTooth.h"

/*

#include "biosemi-activetwo/ovasCDriverBioSemiActiveTwo.h"
#include "brainproducts-actichamp/ovasCDriverBrainProductsActiCHamp.h"
#include "brainproducts-brainampseries/ovasCDriverBrainProductsBrainampSeries.h"
#include "brainproducts-vamp/ovasCDriverBrainProductsVAmp.h"
#include "brainproducts-liveamp/ovasCDriverBrainProductsLiveAmp.h"
#include "emotiv-epoc/ovasCDriverEmotivEPOC.h"
#include "labstreaminglayer/ovasCDriverLabStreamingLayer.h"
#include "micromed-systemplusevolution/ovasCDriverMicromedSystemPlusEvolution.h"
#include "mindmedia-nexus32b/ovasCDriverMindMediaNeXus32B.h"
#include "mcs-nvx/ovasCDriverMCSNVXDriver.h"
#include "neuroelectrics-enobio3g/ovasCDriverEnobio3G.h"
#include "neuroservo/ovasCDriverNeuroServoHid.h"
#include "neurosky-mindset/ovasCDriverNeuroskyMindset.h"
#include "tmsi/ovasCDriverTMSi.h"
#include "tmsi-refa32b/ovasCDriverTMSiRefa32B.h"

#include "mensia-acquisition/ovasCDriverMensiaAcquisition.h"

#include "shimmer-gsr/ovasCDriverShimmerGSR.hpp"
*/

namespace OpenViBE {
namespace AcquisitionServer {

struct OVMasterPrivate {

    OVMasterPrivate(const Kernel::IKernelContext& kernelCtx, OVMaster *q);
    ~OVMasterPrivate();

    void initDrivers();
    void launchThread();
    QJSValue toJSParams(const OpenViBE::Parameters::ParameterObjects& params);

    std::vector<IDriver*> m_drivers;
    CHeader m_header;
    const Kernel::IKernelContext& m_kernelCtx;
    CAcquisitionServer* m_acquisitionServer = nullptr;
    CAcquisitionServerThread* m_acquisitionServerThread = nullptr;
    std::thread* m_thread = nullptr;
    OVQmlLogListener* m_qmlLogListener = nullptr;

    OVMaster *q = nullptr;

    //config parameters
    OpenViBE::Parameters::ParameterObjects m_configParams;
    OpenViBE::Parameters::ParameterListStringObject m_driftCorrectionPolicy;
    OpenViBE::Parameters::ParameterIntObject m_driftTolerance;
    OpenViBE::Parameters::ParameterIntObject m_jitterEstimation;
    OpenViBE::Parameters::ParameterIntObject m_oversamplingFactor;
    OpenViBE::Parameters::ParameterBoolObject m_selectOnlyNamedChannels;
    OpenViBE::Parameters::ParameterListStringObject m_NaNReplacement;

    OpenViBE::Parameters::ParameterObjects m_deviceParams;
    OpenViBE::Parameters::ParameterObjects m_driverParams;

    int m_connectionPort;
    size_t m_sampleCountPerBlock;
    QList<int> m_sampleCountPerBlockList;
    std::vector<double> m_impedances;
    QList<ImpedanceItem*> m_impedancesItems;

    Kernel::ILogManager& getLogManager() {
        return m_kernelCtx.getLogManager();
    }
};

OVMasterPrivate::OVMasterPrivate(const Kernel::IKernelContext& kernelCtx, OVMaster *q)
    : m_kernelCtx(kernelCtx), 
     m_acquisitionServer(new CAcquisitionServer(kernelCtx)),
     q(q),
     m_driftCorrectionPolicy("Drift Correction Policy", "Drift correction Policy", {"Let the Driver Decide", "Forced", "Disabled"}, "Disabled"),
     m_driftTolerance("Drift Tolerance", "Drift tolerance in ms", 2, 0, 100),
     m_jitterEstimation("Jitter Estimation", "Jitter estimation count for drift in ms", 16, 0, 1000),
     m_oversamplingFactor("Oversampling Factor", "Oversampling factor", 1, 0, 100),
     m_selectOnlyNamedChannels("Select Only Named Channels", "Select only named channels", false),
     m_NaNReplacement("NaN value replacement", "How to replace NaN values", {"Disabled", "Zero", "LastCorrectValue"}, "Disabled"),
     m_connectionPort(1024),
     m_sampleCountPerBlock(32),
     m_sampleCountPerBlockList({4, 8, 16, 32, 64, 128, 256, 512})

{
    m_acquisitionServer->setDriftCallback([this](void) {
        this->q->driftChanged();
    });

    // Config Parameters
    // -------------------------------------------------------------------------
    m_driftCorrectionPolicy.setValue(m_acquisitionServer->m_DriftCorrection.getDriftCorrectionPolicyStr().toASCIIString());
    m_driftCorrectionPolicy.parameter()->connect([this](std::any any_value) {
        std::string value = std::any_cast<std::string>(any_value);
        if(value == "Forced") {
            m_acquisitionServer->m_DriftCorrection.setDriftCorrectionPolicy(EDriftCorrectionPolicies::Forced);
        } else if(value == "Disabled") {
            m_acquisitionServer->m_DriftCorrection.setDriftCorrectionPolicy(EDriftCorrectionPolicies::Disabled);
        } else {
            m_acquisitionServer->m_DriftCorrection.setDriftCorrectionPolicy(EDriftCorrectionPolicies::DriverChoice);
        }
    });
    m_configParams.push_back(&m_driftCorrectionPolicy);


    m_driftTolerance.setValue((int)m_acquisitionServer->m_DriftCorrection.getDriftToleranceDurationMs());
    m_driftTolerance.parameter()->connect([this](std::any value) {
        m_acquisitionServer->m_DriftCorrection.setDriftToleranceDurationMs(std::any_cast<int>(value));
    });
    m_configParams.push_back(&m_driftTolerance);

    m_jitterEstimation.setValue((int)m_acquisitionServer->m_DriftCorrection.getJitterEstimationCountForDrift());
    m_jitterEstimation.parameter()->connect([this](std::any value) {
        m_acquisitionServer->m_DriftCorrection.setJitterEstimationCountForDrift(std::any_cast<int>(value));
    });
    m_configParams.push_back(&m_jitterEstimation);

    m_oversamplingFactor.setValue((int)m_acquisitionServer->getOversamplingFactor());
    m_oversamplingFactor.parameter()->connect([this](std::any value) {
        m_acquisitionServer->setOversamplingFactor(std::any_cast<int>(value));
    });
    m_configParams.push_back(&m_oversamplingFactor);

    m_selectOnlyNamedChannels.setValue(m_acquisitionServer->isChannelSelectionRequested());
    m_selectOnlyNamedChannels.parameter()->connect([this](std::any value) {
        m_acquisitionServer->setChannelSelectionRequest(std::any_cast<bool>(value));
    });
    m_configParams.push_back(&m_selectOnlyNamedChannels);

    m_NaNReplacement.setValue(QString::fromStdString(toString(m_acquisitionServer->getNaNReplacementPolicy())));
    m_NaNReplacement.parameter()->connect([this](std::any any_value) {
        std::string value = std::any_cast<std::string>(any_value);
        if(value == "Zero")
            m_acquisitionServer->setNaNReplacementPolicy(ENaNReplacementPolicy::Zero);
        else if(value == "LastCorrectValue")
            m_acquisitionServer->setNaNReplacementPolicy(ENaNReplacementPolicy::LastCorrectValue);
        else
            m_acquisitionServer->setNaNReplacementPolicy(ENaNReplacementPolicy::Disabled);
    });
    m_configParams.push_back(&m_NaNReplacement);

    // Initialize PLugins and get their parameters
    // -------------------------------------------------------------------------

    // Plugins that just send out data must be the last in list (since other plugins may modify the data)
    m_acquisitionServer->m_Plugins.push_back(new Plugins::CPluginExternalStimulations(m_kernelCtx));
    m_acquisitionServer->m_Plugins.push_back(new Plugins::CPluginTCPTagging(m_kernelCtx));

#if defined TARGET_HAS_ThirdPartyLSL
    m_acquisitionServer->m_Plugins.push_back(new Plugins::CPluginLSLOutput(m_kernelCtx));
#endif
    m_acquisitionServer->m_Plugins.push_back(new Plugins::CPluginFiddler(m_kernelCtx));


    std::vector<IAcquisitionServerPlugin*> plugins = m_acquisitionServer->getPlugins();

    for (auto itp = plugins.begin(); itp != plugins.end(); ++itp) {
        for(auto param: (*itp)->parameters()) {
            m_configParams.push_back(OpenViBE::Parameters::ParameterObject::fromParameter(param));
        }
    }

    m_acquisitionServerThread = new CAcquisitionServerThread(kernelCtx, *m_acquisitionServer);
    m_acquisitionServerThread->m_impedanceCallback = [this](const std::vector<double>& impedances) {
        m_impedances = impedances;
        this->q->impedancesChanged();
    };

    m_thread = new std::thread(&CAcquisitionServerThread::main, m_acquisitionServerThread);

}

OVMasterPrivate::~OVMasterPrivate()
{
    for(auto paramObj: m_driverParams) {
        delete paramObj;
    }

    for(auto *obj : m_impedancesItems) {
        delete obj;
    }
    m_impedancesItems.clear();

    // terminate need the logListener
    m_acquisitionServerThread->terminate();
    m_thread->join();

    delete m_acquisitionServer;
    m_acquisitionServer = nullptr;

    if(m_qmlLogListener) {
        delete m_qmlLogListener;
        m_qmlLogListener = nullptr;
    }

    delete m_thread;
    m_thread = nullptr;
    delete m_acquisitionServerThread;
    m_acquisitionServerThread = nullptr;
}

void OVMasterPrivate::initDrivers()
{
    m_drivers.push_back(new CDriverEGIAmpServer(m_acquisitionServer->getDriverContext()));
    m_drivers.push_back(new CDriverGenericOscillator(m_acquisitionServer->getDriverContext()));
    m_drivers.push_back(new CDriverGenericSawTooth(m_acquisitionServer->getDriverContext()));
    m_drivers.push_back(new CDriverGenericTimeSignal(m_acquisitionServer->getDriverContext()));
    m_drivers.push_back(new CDriverSimulatedDeviator(m_acquisitionServer->getDriverContext()));
    m_drivers.push_back(new CDriverGenericRawFileReader(m_acquisitionServer->getDriverContext()));
    m_drivers.push_back(new CDriverGenericRawTelnetReader(m_acquisitionServer->getDriverContext()));
}

void OVMasterPrivate::launchThread()
{

}

QJSValue OVMasterPrivate::toJSParams(const OpenViBE::Parameters::ParameterObjects& params)
{
    QQmlEngine *js_engine = nullptr;
    QQmlContext *context = QQmlEngine::contextForObject(q);
    if (!context) {
        m_kernelCtx.getLogManager() << Kernel::LogLevel_Warning << "Could not obtain QQmlContext for object.\n";
        return QJSValue();
    } else {
        js_engine = context->engine();
    }

    auto nb_params = params.size();
    QJSValue js_params = js_engine->newArray((uint)nb_params);
    for (int i = 0; i < nb_params; ++i) {
        js_params.setProperty(i, js_engine->newQObject(params[i]));
    }

    return js_params;
}

OVMaster::OVMaster() {
    OpenViBE::CKernelLoader kernelLoader;

    std::cout << "[  INF  ] Created kernel loader, trying to load kernel module" << std::endl;
    OpenViBE::CString error;

    OpenViBE::CString kernelFile = OpenViBE::Directories::getLib("kernel");

    if (!kernelLoader.load(kernelFile, &error)) { 
        std::cout << "[ FAILED ] Error loading kernel from [" << kernelFile << "]: " << error << "\n"; 
    }
    else {
        std::cout << "[  INF  ] Kernel module loaded, trying to get kernel descriptor" << std::endl;
        OpenViBE::Kernel::IKernelDesc* kernelDesc = nullptr;
        kernelLoader.initialize();
        kernelLoader.getKernelDesc(kernelDesc);
        if (!kernelDesc) {
            std::cout << "[ FAILED ] No kernel descriptor" << std::endl;
        }
        else {
            std::cout << "[  INF  ] Got kernel descriptor, trying to create kernel" << std::endl;


            OpenViBE::CString configFile = OpenViBE::CString(OpenViBE::Directories::getDataDir() + "/kernel/openvibe.conf");

            Kernel::IKernelContext* kernelCtx = kernelDesc->createKernel("acquisition-server", configFile);
            if (!kernelCtx) {
                std::cout << "[ FAILED ] No kernel created by kernel descriptor" << std::endl;
            }
            else {
                kernelCtx->initialize();

                OpenViBE::Kernel::IConfigurationManager& configManager = kernelCtx->getConfigurationManager();

                bool res = configManager.addConfigurationFromFile(configManager.expand("${Path_Data}/applications/acquisition-server/acquisition-server-defaults.conf"));
                if (! res) {
                    kernelCtx->getLogManager() << OpenViBE::Kernel::LogLevel_Error << "Configuration file not found: " << configManager.expand("${Path_Data}/applications/acquisition-server/acquisition-server-defaults.conf") << ". Please fix the {Path_Data} variable in /kernel/openvibe.conf to point to ${Path_Data}/applications/acquisition-server/acquisition-server-defaults.conf\n";
                }
                // User configuration mods
                res = configManager.addConfigurationFromFile(configManager.expand("${Path_UserData}/openvibe-acquisition-server.conf"));
                if (!res) {
                    kernelCtx->getLogManager() << OpenViBE::Kernel::LogLevel_Error << "Configuration file not found: " << configManager.expand("${Path_UserData}/openvibe-acquisition-server.conf") << ". Please fix the {Path_UserData} variable in /kernel/openvibe.conf to point to ${Path_UserData}/openvibe-acquisition-server.conf\n";
                }
                res = kernelCtx->getPluginManager().addPluginsFromFiles(configManager.expand("${AcquisitionServer_Plugins}"));
                if (!res) {
                    kernelCtx->getLogManager() << OpenViBE::Kernel::LogLevel_Error << "Plugins files not found: " << configManager.expand("${AcquisitionServer_Plugins}") <<" Please fix the {AcquisitionServer_Plugins} variable in " << configManager.expand("${Path_Data}/applications/acquisition-server/acquisition-server-defaults.conf") << " to point to ${Path_Lib}/libopenvibe-plugins-*-stream-codec*\n";
                }

                /*
                Commented out cause config is loaded from argc/argv in legacy, which is not in place here
                for (auto itr = config.tokens.begin(); itr != config.tokens.end(); ++itr) {
                    kernelCtx->getLogManager() << OpenViBE::Kernel::LogLevel_Trace << "Adding command line configuration token ["
                            << (*itr).first.c_str() << " = " << (*itr).second.c_str() << "]\n";
                    configManager.addOrReplaceConfigurationToken((*itr).first.c_str(), (*itr).second.c_str());
                }   */

                // Check the clock
                if (!System::Time::isClockSteady()) {
                    kernelCtx->getLogManager() << OpenViBE::Kernel::LogLevel_Warning
                            << "The system does not seem to have a steady clock. This may affect the acquisition time precision.\n";
                }

                d = new OVMasterPrivate(*kernelCtx, this);
                //create qml log controller
                d->m_qmlLogListener = new OVQmlLogListener();
                kernelCtx->getLogManager().addListener(d->m_qmlLogListener);

                d->initDrivers();
                d->launchThread();
                emit driverChanged();
            }
        }
    }
}

QStringList OVMaster::getDrivers() {
    QStringList drivers;
    for (const auto driver: d->m_drivers) {
        d->getLogManager() << Kernel::LogLevel_Info << "Adding driver: " << driver->getName() << ".\n";
        drivers.append(driver->getName());
    }
    return drivers;
}

OVMaster::~OVMaster() {
    delete d;

}

QJSValue OVMaster::configParameters() {
    return d->toJSParams(d->m_configParams);
}

QJSValue OVMaster::driverParameters() {
    auto * driver = d->m_acquisitionServer->getDriver();
    if(!driver) {
        d->getLogManager() << Kernel::LogLevel_Warning << "No driver set.\n";
        return QJSValue();
    }
    OpenViBE::Parameters::Parameters params = driver->parameters();

    //TODO can be optimized by reusing the same objects
    //clear old params
    for(auto paramObj: d->m_driverParams) {
        delete paramObj;
    }
    d->m_driverParams.clear();

    for (auto param: params) {
        if(param->name() == "Check Impedance") {
            param->connect([this, param](std::any value) {
                bool check = std::any_cast<bool>(value);
                if(check) {
                    emit impedanceRequested();
                    param->setValue(false);
                }
            });
        }

        d->m_driverParams.push_back(OpenViBE::Parameters::ParameterObject::fromParameter(param));
    }
    return d->toJSParams(d->m_driverParams);
}

QJSValue OVMaster::deviceParameters()
{
    d->getLogManager() << Kernel::LogLevel_Info << "Getting device parameters.\n";
    auto * driver = d->m_acquisitionServer->getDriver();
    if(!driver) {
        d->getLogManager() << Kernel::LogLevel_Warning << "No driver set.\n";
        return QJSValue();
    }
    IHeader* header = driver->getHeader();

    for(auto paramObj: d->m_deviceParams) {
        delete paramObj;
    }
    d->m_deviceParams.clear();

    d->m_deviceParams.push_back(OpenViBE::Parameters::ParameterObject::fromParameter(&header->getExperimentIDParameter()));
    d->m_deviceParams.push_back(OpenViBE::Parameters::ParameterObject::fromParameter(&header->getSubjectAgeParameter()));
    d->m_deviceParams.push_back(OpenViBE::Parameters::ParameterObject::fromParameter(&header->getSubjectGenderParameter()));
    d->m_deviceParams.push_back(OpenViBE::Parameters::ParameterObject::fromParameter(&header->getChannelCountParameter()));
    d->m_deviceParams.push_back(OpenViBE::Parameters::ParameterObject::fromParameter(&header->getSamplingFrequencyParameter()));

    d->getLogManager() << Kernel::LogLevel_Info << "device parameters count: " << d->m_deviceParams.size() << ".\n";
    for(auto param: d->m_deviceParams) {
        d->getLogManager() << Kernel::LogLevel_Info << "device parameter: " << param->name().toStdString() << ".\n";
    }

    d->getLogManager() << Kernel::LogLevel_Info << "Returning device parameters.\n";
    return d->toJSParams(d->m_deviceParams);
}

QStringList OVMaster::getChannelNames()
{
    auto * driver = d->m_acquisitionServer->getDriver();
    if(!driver) {
        d->getLogManager() << Kernel::LogLevel_Warning << "OVMaster::getChannelNames: No driver set.\n";
        return QStringList();
    }

    auto header = driver->getHeader();

    QStringList channelNames;
    for (size_t i = 0; i < header->getChannelCount(); ++i) {
        channelNames.append(header->getChannelName(i));
    }
    return channelNames;
}

void OVMaster::setChannelNames(const QStringList& channelNames)
{
    auto driver = d->m_acquisitionServer->getDriver();

    if(!driver) {
        d->getLogManager() << Kernel::LogLevel_Warning << "Impossible to set channels: No driver set.\n";
        return;
    }

    auto header = driver->getHeader();

    d->getLogManager() << Kernel::LogLevel_Info << "Setting channel names.\n";

    for (size_t i = 0; i < channelNames.size(); ++i) {
        header->setChannelName(i, channelNames[i].toStdString().c_str());
    }
}

int OVMaster::connectionPort() const
{
    return d->m_connectionPort;
}

void OVMaster::setConnectionPort(int port)
{
    if (d->m_connectionPort != port) {
        d->getLogManager() << Kernel::LogLevel_Info << "Setting connection port: " << port << ".\n";
        d->m_connectionPort = port;
        emit connectionPortChanged();
    }
}

void OVMaster::setSampleCountPerBlock(int sampleCount)
{
    if (d->m_sampleCountPerBlock != sampleCount) {
        d->getLogManager() << Kernel::LogLevel_Info << "Setting sample count per block: " << sampleCount << ".\n";
        d->m_sampleCountPerBlock = sampleCount;
        emit sampleCountPerBlockListChanged();
    }
}

QList<int> OVMaster::getSampleCountPerBlockList()
{
    return d->m_sampleCountPerBlockList;
}

QQmlListProperty<ImpedanceItem> OVMaster::impedances()
{

    auto nbChannels = d->m_header.getChannelCount();
    double impedanceLimit = d->m_kernelCtx.getConfigurationManager().expandAsFloat("${AcquisitionServer_DefaultImpedanceLimit}", 20000);

    //clear old impedances
    for(auto *obj : d->m_impedancesItems) {
        delete obj;
    }
    d->m_impedancesItems.clear();

    if(nbChannels == d->m_impedances.size()) {
        for(size_t i=0; i < nbChannels; i++) {
            QString name = d->m_header.getChannelName(i);
            double fraction = d->m_impedances[i]/ impedanceLimit;
            if (fraction > 1) { fraction = 1; }
            
            d->m_impedancesItems.append(new ImpedanceItem(name, d->m_impedances[i]/1000., fraction));
        }
    } else {
        d->getLogManager() << Kernel::LogLevel_Warning << "Impedances size mismatch! Received " << d->m_impedances.size() << " impedance values for " << nbChannels << " channels.\n";
    }

    return QQmlListProperty<ImpedanceItem>(this, &d->m_impedancesItems);
}

int OVMaster::getInitSampleCountPerBlockIndex()
{
    d->getLogManager() << Kernel::LogLevel_Debug << "Init index: " << int(d->m_sampleCountPerBlockList.indexOf(d->m_sampleCountPerBlock)) << ".\n";
    return (int)d->m_sampleCountPerBlockList.indexOf(d->m_sampleCountPerBlock);
}

void OVMaster::setCurrentDriver(const QString& driverName)
{
    d->getLogManager() << Kernel::LogLevel_Info << "Setting current driver: " << driverName.toStdString() << ".\n";
    for (const auto driver: d->m_drivers) {
        if(driver->getName() == driverName) {
            d->m_acquisitionServer->setDriver(driver);
            emit deviceParametersChanged();
            emit driverParametersChanged();
            return;
        }
    }
    d->getLogManager() << Kernel::LogLevel_Info << "Driver not found: " << driverName.toStdString() << ".\n";
}

double OVMaster::drift()
{
    return d->m_acquisitionServer->m_DriftCorrection.getDriftMs();
}

bool OVMaster::connectDevice()
{
    d->getLogManager() << Kernel::LogLevel_Debug <<  "Connecting device.\n";
    //TODO: get sampleCountPerBlock and port from UI
    return d->m_acquisitionServerThread->connect(*(d->m_acquisitionServer->getDriver()), d->m_sampleCountPerBlock, d->m_connectionPort);
}

bool OVMaster::disconnectDevice()
{
    d->getLogManager() << Kernel::LogLevel_Info << "Disconnecting device.\n";
    return d->m_acquisitionServerThread->disconnect();
}

bool OVMaster::startAcquisition()
{
    d->getLogManager() << Kernel::LogLevel_Info << "Starting acquisition.\n";
    return d->m_acquisitionServerThread->start();
}

bool OVMaster::stopAcquisition()
{
    d->getLogManager() << Kernel::LogLevel_Info << "Stopping acquisition.\n";
    return d->m_acquisitionServerThread->stop();
}

void OVMaster::configureDeviceDriver()
{
    // Call driver configure
    d->m_acquisitionServer->getDriver()->configure();
    d->getLogManager() << Kernel::LogLevel_Info << "Device driver configuration done.\n";
}

}  // namespace AcquisitionServer
}  // namespace OpenViBE
