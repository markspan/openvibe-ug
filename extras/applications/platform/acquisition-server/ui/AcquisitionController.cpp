/**
 * SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
 */

#include "AcquisitionController.h"
#include "Charmeleon.h"  // UG fork: impedances in Charmeleon

#include <QWinEventNotifier>
#include <Windows.h>

#include <QCoreApplication>
#include <QQmlContext>
#include <QQmlEngine>
#include <QTimer>

#include <system/ovCTime.h>
#include <algorithm>
#include <vector>

#include <openvibe/CIdentifier.hpp>
#include <openvibe/ov_all.h>

#include <OVQmlLogListener.h>

#include <openvibe/parameters/parameters.h>
#include <parametersObject.h>
#include <parameterStringVectorObject.h>

#include "AbstractDriver.h"
#include "AbstractWriter.h"
#include "AcquisitionManager.h"
#include "Acquisition.h"
#include "NetService.h"
#include "SessionManager.h"
#include "TcpTaggingService.h"

namespace OpenViBE {
namespace AcquisitionServer {

struct AcquisitionControllerPrivate {

    AcquisitionControllerPrivate(const Kernel::IKernelContext& kernelCtx, AcquisitionController *q);
    ~AcquisitionControllerPrivate();

    void launchThread();
    QJSValue toJSParams(const OpenViBE::Parameters::ParameterObjects& params);

    void wrapDriverChannelNames();

    AcquisitionManager& m_acqManager = AcquisitionManager::instance();
    Services::NetService & m_netSrv = OpenViBE::Services::NetService::instance();
    Services::TcpTaggingService & m_tcpTaggingSrv = OpenViBE::Services::TcpTaggingService::instance();
    std::string m_acquisitionName = DEFAULT_ACQUISITION_NAME;
    Acquisition* m_acquisition = nullptr;
    std::vector<AbstractDriver*> m_drivers;
    const Kernel::IKernelContext& m_kernelCtx;
    std::thread* m_thread = nullptr;
    OVQmlLogListener* m_qmlLogListener = nullptr;

    QQmlEngine *m_engine = nullptr;
    AcquisitionController *q = nullptr;

    //config parameters
    OpenViBE::Parameters::ParameterObjects m_configParams;
    OpenViBE::Parameters::ParameterIntObject m_jitterEstimation;
    OpenViBE::Parameters::ParameterIntObject m_oversamplingFactor;
    OpenViBE::Parameters::ParameterBoolObject m_selectOnlyNamedChannels;
    OpenViBE::Parameters::ParameterListStringObject m_NaNReplacement;

    OpenViBE::Parameters::ParameterObjects m_deviceParams;
    OpenViBE::Parameters::ParameterObjects m_driverParams;
    std::unique_ptr<OpenViBE::Parameters::parameterStringVectorObject> m_channelNamesObj = nullptr;

    // UG fork: the Charmeleon the impedance button started, whose montage the driver takes over when it closes
    Charmeleon::Launched m_charmeleon;
    QWinEventNotifier* m_charmeleonWatcher = nullptr;

    OpenViBE::Parameters::ParameterIntObject* m_connectionPort;
    OpenViBE::Parameters::ParameterIntObject* m_tcpTaggingPort;
    OpenViBE::Parameters::ParameterListIntObject* m_sampleCountPerBlock;
    // UG fork: the drift correction settings
    OpenViBE::Parameters::ParameterListStringObject* m_driftCorrectionPolicy;
    OpenViBE::Parameters::ParameterIntObject* m_driftTolerance;
    std::vector<double> m_impedances;
    std::vector<double> m_impedanceBuf;
    QList<ImpedanceItem*> m_impedancesItems;
    QTimer* m_impedanceTimer = nullptr;

    double m_driftLimit = 5.0; ///< Default drift limit in ms.
    double m_instantDrift = 0.0; ///< Instant drift in ms.
    double m_globalDrift = 0.0; ///< Global drift in ms.
    QTimer* m_driftTimer = nullptr;

    Kernel::ILogManager& getLogManager() {
        return m_kernelCtx.getLogManager();
    }
};

AcquisitionControllerPrivate::AcquisitionControllerPrivate(const Kernel::IKernelContext& kernelCtx, AcquisitionController *q)
    : m_kernelCtx(kernelCtx), 
     q(q),
     m_jitterEstimation("Jitter Estimation", "Jitter estimation count for drift in ms", 16, 0, 1000),
     m_oversamplingFactor("Oversampling Factor", "Oversampling factor", 1, 0, 100),
     m_selectOnlyNamedChannels("Select Only Named Channels", "Select only named channels", false),
     m_NaNReplacement("NaN value replacement", "How to replace NaN values", {"Disabled", "Zero", "LastCorrectValue"}, "Disabled")
{
    //Get default acquisition
    m_acquisition = m_acqManager.getAcquisition(m_acquisitionName);
    for (auto& enabledWriter : m_acqManager.getAcquisitionWritersEnableParam(m_acquisitionName)) {
        enabledWriter.second.connect([this](std::any value) {
            SessionManager::instance().applyWriterState(m_acquisitionName);
            emit this->q->configParametersChanged();
            //Saving config change
            SessionManager::instance().saveCurrent(m_acquisitionName);
        });
    }
    
    // Set UI objects with config values
    wrapDriverChannelNames();

    m_sampleCountPerBlock = new OpenViBE::Parameters::ParameterListIntObject(m_acquisition->getChunkSampleCount());
    // UG fork: the drift correction settings
    m_driftCorrectionPolicy = new OpenViBE::Parameters::ParameterListStringObject(m_acquisition->getDriftCorrectionPolicy());
    m_driftTolerance        = new OpenViBE::Parameters::ParameterIntObject(m_acquisition->getDriftTolerance());

    m_connectionPort = new OpenViBE::Parameters::ParameterIntObject(m_netSrv.getPortParameter());
    m_tcpTaggingPort = new OpenViBE::Parameters::ParameterIntObject(m_tcpTaggingSrv.getPortParameter());

    const std::string acquisitionName = m_acquisitionName;
    const auto saveOnChange = [acquisitionName](std::any) { SessionManager::instance().saveCurrent(acquisitionName); };
    m_acquisition->getChunkSampleCount()->connect(saveOnChange);
    m_acquisition->getDriftCorrectionPolicy()->connect(saveOnChange);  // UG fork
    m_acquisition->getDriftTolerance()->connect(saveOnChange);
    m_netSrv.getPortParameter()->connect(saveOnChange);
    m_tcpTaggingSrv.getPortParameter()->connect(saveOnChange);

}

AcquisitionControllerPrivate::~AcquisitionControllerPrivate()
{
    // UG fork
    delete m_charmeleonWatcher;
    Charmeleon::takeResult(m_charmeleon);  // releases the process handle and the result file

    // The driver may outlive the controller its listener posts to
    if (m_acquisition) { m_acquisition->setDriverStateListener({}); }

    if (m_impedanceTimer) {
        m_impedanceTimer->stop();
        delete m_impedanceTimer;
        m_impedanceTimer = nullptr;
    }

    for(auto paramObj: m_driverParams) {
        delete paramObj;
    }

    for(auto *obj : m_impedancesItems) {
        delete obj;
    }
    m_impedancesItems.clear();

    if(m_qmlLogListener) {
        delete m_qmlLogListener;
        m_qmlLogListener = nullptr;
    }
}


void AcquisitionControllerPrivate::wrapDriverChannelNames()
{
    // The wrapper points into the driver, so it only lives as long as the driver it was built for.
    m_channelNamesObj.reset();

    // UG fork: the driver tells when its note or channel locks change, possibly from the acquisition's thread,
    // so the signal is queued onto the controller's
    m_acquisition->setDriverStateListener([controller = q]() {
        QMetaObject::invokeMethod(controller, [controller]() { emit controller->driverStateChanged(); }, Qt::QueuedConnection);
    });

    auto* param = m_acquisition->getDriverChannelNamesParam();
    if (param) {
        m_channelNamesObj.reset(new OpenViBE::Parameters::parameterStringVectorObject(param));
        QObject::connect(m_channelNamesObj.get(), &OpenViBE::Parameters::parameterStringVectorObject::valueChanged,
                         q, &AcquisitionController::channelNamesChanged);
    }
}

void AcquisitionControllerPrivate::launchThread()
{

}

QJSValue AcquisitionControllerPrivate::toJSParams(const OpenViBE::Parameters::ParameterObjects& params)
{
    if (!m_engine) {
        QQmlContext *context = QQmlEngine::contextForObject(q);
        if (!context) {
            m_kernelCtx.getLogManager() << Kernel::LogLevel_Error << "Could not obtain QQmlContext for object.\n";
        } else {
            m_engine = context->engine();
        }
    }

    auto nb_params = params.size();
    QJSValue js_params = m_engine->newArray((uint)nb_params);
    for (int i = 0; i < nb_params; ++i) {
        js_params.setProperty(i, m_engine->newQObject(params[i]));
    }

    return js_params;
}

AcquisitionController::AcquisitionController() {
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
                /* Likely not needed anymore with new acquisition server
                res = kernelCtx->getPluginManager().addPluginsFromFiles(configManager.expand("${AcquisitionServer_Plugins}"));
                if (!res) {
                    kernelCtx->getLogManager() << OpenViBE::Kernel::LogLevel_Error << "Plugins files not found: " << configManager.expand("${AcquisitionServer_Plugins}") <<" Please fix the {AcquisitionServer_Plugins} variable in " << configManager.expand("${Path_Data}/applications/acquisition-server/acquisition-server-defaults.conf") << " to point to ${Path_Lib}/libopenvibe-plugins-*-stream-codec*\n";
                }*/

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

                d = new AcquisitionControllerPrivate(*kernelCtx, this);
                //create qml log controller
                d->m_qmlLogListener = new OVQmlLogListener();
                kernelCtx->getLogManager().addListener(d->m_qmlLogListener);

                //the window may be closed in ways that do not go through the user interface, so
                //hook the one signal every exit route passes through
                connect(qApp, &QCoreApplication::aboutToQuit, this, &AcquisitionController::saveSession);

                emit driverChanged();
            }
        }
    }
}

QStringList AcquisitionController::getDrivers() {
    QStringList drivers;
    for (const std::string& name: d->m_acqManager.getDriverNames()) {
        drivers.append(QString::fromStdString(name));
    }
    return drivers;
}

AcquisitionController::~AcquisitionController() {
    delete d;

}

bool AcquisitionController::isAcquiring() {
    return d->m_acqManager.isAcquisitionRunning(d->m_acquisitionName);
}

bool AcquisitionController::isCheckingImpedance() {
    return d->m_acquisition && d->m_acquisition->state() == Acquisition::State::ImpedanceCheck;
}

// UG fork: impedances in Charmeleon, for the drivers that hand them to it
std::string AcquisitionController::charmeleonExecutable() {
    if (!d->m_acquisition || !d->m_acquisition->usesCharmeleonForImpedance()) { return {}; }
    const CString configured = d->m_kernelCtx.getConfigurationManager().expand("${AcquisitionServer_CharmeleonPath}");
    return Charmeleon::findExecutable(configured.toASCIIString());
}

bool AcquisitionController::supportsImpedanceCheck() {
    return d->m_acquisition && (d->m_acquisition->supportsImpedanceCheck() || !charmeleonExecutable().empty());
}

bool AcquisitionController::toggleImpedanceCheck() {

    bool res = false;

    // UG fork: Charmeleon checks the impedances in its own window; Play closes it again
    const std::string charmeleon = charmeleonExecutable();
    if (!charmeleon.empty() && d->m_acquisition->state() == Acquisition::State::Idle) {
        const std::string montage = d->m_acquisition->getCharmeleonMontage();
        d->getLogManager() << Kernel::LogLevel_Info << "Checking impedances with " << charmeleon
                           << (montage.empty() ? std::string() : " and montage " + montage) << "\n";
        Charmeleon::Launched launched;
        if (!Charmeleon::launch(charmeleon, montage, d->m_acquisition->getCharmeleonArguments(), &launched)) {
            d->getLogManager() << Kernel::LogLevel_Error << "Could not start " << charmeleon << "\n";
            return false;
        }
        // UG fork: once this Charmeleon closes, the driver takes over the montage it saved
        if (launched.process) {
            takeOverCharmeleonMontage();  // a previous one that has not been dealt with yet
            d->m_charmeleon        = launched;
            d->m_charmeleonWatcher = new QWinEventNotifier(static_cast<HANDLE>(launched.process), this);
            connect(d->m_charmeleonWatcher, &QWinEventNotifier::activated, this, &AcquisitionController::takeOverCharmeleonMontage);
        }
        return true;
    }

    if (d->m_acquisition) {
        if (d->m_acquisition->state() == Acquisition::State::ImpedanceCheck) {
            if (d->m_impedanceTimer) {
                d->m_impedanceTimer->stop();
                delete d->m_impedanceTimer;
                d->m_impedanceTimer = nullptr;
            }
            d->m_acquisition->stopImpedanceCheck();
            emit isCheckingImpedanceChanged();
            res = true;
        } else if (d->m_acquisition->state() == Acquisition::State::Idle) {
            if (d->m_acquisition->startImpedanceCheck()) {

                auto channelNames = d->m_acquisition->getDriverChannelNames();
                d->m_impedanceBuf.assign(channelNames.size(), 0.0);

                d->m_impedanceTimer = new QTimer(this);
                d->m_impedanceTimer->setInterval(500);
                connect(d->m_impedanceTimer, &QTimer::timeout, this, [this]() {
                    int n = d->m_acquisition->readImpedances(d->m_impedanceBuf.data(), d->m_impedanceBuf.size());

                    double impedanceLimit = d->m_kernelCtx.getConfigurationManager().expandAsFloat(
                        "${AcquisitionServer_DefaultImpedanceLimit}", 20000);
                    auto channelNames = d->m_acquisition->getDriverChannelNames();

                    bool rebuild = (n != d->m_impedancesItems.size());
                    if (rebuild) {
                        for (auto* obj : d->m_impedancesItems) delete obj;
                        d->m_impedancesItems.clear();
                    }

                    for (int i = 0; i < n; ++i) {
                        QString name = (i < static_cast<int>(channelNames.size())) ? QString::fromStdString(channelNames[i]) : QString("Ch %1").arg(i + 1);
                        double val      = d->m_impedanceBuf[i];
                        double fraction = std::min(val / impedanceLimit, 1.0);
                        if (rebuild) {
                            d->m_impedancesItems.append(new ImpedanceItem(name, val / 1000.0, fraction));
                        } else {
                            d->m_impedancesItems[i]->setValue(val / 1000.0);
                            d->m_impedancesItems[i]->setFraction(fraction);
                        }
                    }

                    if (rebuild) {
                        emit impedancesChanged();
                    }
                });
                d->m_impedanceTimer->start();

                emit isCheckingImpedanceChanged();
                emit supportsImpedanceCheckChanged();
                res = true;
            }
        }
    }
    return res;
}

QJSValue AcquisitionController::configParameters() {
    if (!d->m_engine) {
        QQmlContext *context = QQmlEngine::contextForObject(this);
        if (!context) {
            d->m_kernelCtx.getLogManager() << Kernel::LogLevel_Error << "Could not obtain QQmlContext for object.\n";
        } else {
            d->m_engine = context->engine();
        }
    }

    for (auto* p : d->m_configParams) {
        delete p;
    }
    d->m_configParams.clear();

    auto& enabledWriters = d->m_acqManager.getAcquisitionWritersEnableParam(d->m_acquisitionName);
    std::vector<AbstractWriter*> writers = d->m_acquisition->getWriters();
    QJSValue result = d->m_engine->newArray((quint32)enabledWriters.size());

    size_t i = 0;
    for (auto& enabledWriter : enabledWriters) {
        QJSValue entry = d->m_engine->newObject();
        entry.setProperty("name", QString::fromStdString(enabledWriter.first));

        OpenViBE::Parameters::ParameterObjects writerParams;
        auto* enableObj = OpenViBE::Parameters::ParameterObject::fromParameter(&enabledWriter.second);
        d->m_configParams.push_back(enableObj);
        writerParams.push_back(enableObj);

        auto writerIt = std::find_if(writers.begin(), writers.end(), [enabledWriter](AbstractWriter* w) {
            return w->name() == enabledWriter.first;
        });
        if (writerIt != writers.end()) {
            for (auto* param : (*writerIt)->getParameters()) {
                auto* obj = OpenViBE::Parameters::ParameterObject::fromParameter(param);
                d->m_configParams.push_back(obj);
                writerParams.push_back(obj);
            }
        }
        entry.setProperty("parameters", d->toJSParams(writerParams));
        result.setProperty((quint32)i, entry);
        i++;
    }

    return result;
}

QJSValue AcquisitionController::driverParameters() {
    OpenViBE::Parameters::Parameters params = d->m_acquisition->getDriverParameters();
    
    //TODO can be optimized by reusing the same objects
    //clear old params
    for(auto paramObj: d->m_driverParams) {
        delete paramObj;
    }
    d->m_driverParams.clear();

    for (auto param: params) {
        d->m_driverParams.push_back(OpenViBE::Parameters::ParameterObject::fromParameter(param));
    }
    return d->toJSParams(d->m_driverParams);
}

QJSValue AcquisitionController::deviceParameters()
{
    /*d->getLogManager() << Kernel::LogLevel_Info << "Getting device parameters.\n";
    auto * driver = d->m_acquisitionServer->getDriver();
    if(!driver) {
        d->getLogManager() << Kernel::LogLevel_Warning << "No driver set.\n";
        return QJSValue();
    }
    IHeader* header = driver->getHeader();

    for(auto paramObj: d->m_deviceParams) {
        delete paramObj;
    }*/
    d->m_deviceParams.clear();

/*    d->m_deviceParams.push_back(OpenViBE::Parameters::ParameterObject::fromParameter(&header->getExperimentIDParameter()));
    d->m_deviceParams.push_back(OpenViBE::Parameters::ParameterObject::fromParameter(&header->getSubjectAgeParameter()));
    d->m_deviceParams.push_back(OpenViBE::Parameters::ParameterObject::fromParameter(&header->getSubjectGenderParameter()));
    d->m_deviceParams.push_back(OpenViBE::Parameters::ParameterObject::fromParameter(&header->getChannelCountParameter()));
    d->m_deviceParams.push_back(OpenViBE::Parameters::ParameterObject::fromParameter(&header->getSamplingFrequencyParameter()));*/

    d->getLogManager() << Kernel::LogLevel_Info << "device parameters count: " << d->m_deviceParams.size() << ".\n";
    for(auto param: d->m_deviceParams) {
        d->getLogManager() << Kernel::LogLevel_Info << "device parameter: " << param->name().toStdString() << ".\n";
    }

    d->getLogManager() << Kernel::LogLevel_Info << "Returning device parameters.\n";
    return d->toJSParams(d->m_deviceParams);
}

QStringList AcquisitionController::getChannelNames()
{
    return d->m_channelNamesObj ? d->m_channelNamesObj->value() : QStringList();
}

// UG fork: see AbstractDriver::channelHints()
QStringList AcquisitionController::getChannelHints()
{
    QStringList hints;
    for (const auto& hint : d->m_acquisition->getDriverChannelHints()) { hints.append(QString::fromStdString(hint)); }
    return hints;
}

void AcquisitionController::takeOverCharmeleonMontage()
{
    if (!d->m_charmeleon.process) { return; }
    delete d->m_charmeleonWatcher;
    d->m_charmeleonWatcher = nullptr;

    const Charmeleon::Result result = Charmeleon::takeResult(d->m_charmeleon);
    if (!result.valid) {
        d->getLogManager() << Kernel::LogLevel_Info << "Charmeleon closed without saying which montage it ended with (it is older than 2.2, "
                              "or did not close normally); the driver keeps its montage.\n";
        return;
    }
    if (result.unsaved) { d->getLogManager() << Kernel::LogLevel_Warning << "Charmeleon closed with montage changes that are not saved.\n"; }
    if (!result.saved || result.file.empty()) { return; }

    d->getLogManager() << Kernel::LogLevel_Info << "Taking over the montage Charmeleon saved: " << result.file << "\n";
    d->m_acquisition->takeOverCharmeleonMontage(result.file);
    emit driverParametersChanged();
    saveSession();
}

QString AcquisitionController::driverNote() const
{
    return QString::fromStdString(d->m_acquisition->getDriverNote().text);
}

QString AcquisitionController::driverNoteLevel() const
{
    switch (d->m_acquisition->getDriverNote().level) {
        case AbstractDriver::Note::Level::Warning: return "warning";
        case AbstractDriver::Note::Level::Error: return "error";
        default: return "info";
    }
}

QVariantList AcquisitionController::channelLocks() const
{
    QVariantList locks;
    for (const bool locked : d->m_acquisition->getDriverChannelLocks()) { locks.append(locked); }
    return locks;
}

void AcquisitionController::setChannelNames(const QStringList& channelNames)
{
    std::vector<std::string> names;
    names.reserve(channelNames.size());
    for (const auto& name : channelNames)
        names.push_back(name.toStdString());
    d->m_acquisition->setDriverChannelNames(names);
    saveSession();
}

void AcquisitionController::setChannelName(int index, const QString& name)
{
    auto names = getChannelNames();
    if (index < 0 || index >= names.size()) return;
    names[index] = name;
    setChannelNames(names);
}

QObject* AcquisitionController::connectionPort() const
{
    return d->m_connectionPort;
}

QObject* AcquisitionController::tcpTaggingPort() const
{
    return d->m_tcpTaggingPort;
}

QObject* AcquisitionController::getSampleCountPerBlock() const
{
    return d->m_sampleCountPerBlock;
}

// UG fork: the drift correction settings
QObject* AcquisitionController::driftCorrectionPolicy() const
{
    return d->m_driftCorrectionPolicy;
}

QObject* AcquisitionController::driftTolerance() const
{
    return d->m_driftTolerance;
}

QQmlListProperty<ImpedanceItem> AcquisitionController::impedances()
{
    return QQmlListProperty<ImpedanceItem>(this, &d->m_impedancesItems);
}

QString AcquisitionController::currentDriver() const
{
    return QString::fromStdString(d->m_acquisition->getDriverName());
}

void AcquisitionController::saveSession() {
    SessionManager::instance().saveCurrent(d->m_acquisitionName);
}

void AcquisitionController::setCurrentDriver(const QString& driverName)
{
    if (driverName.isEmpty() || driverName == currentDriver()) {
        return;   // the combo box also reports the selection it makes when the model is first set
    }

    d->getLogManager() << Kernel::LogLevel_Info << "Setting current driver: " << driverName.toStdString() << ".\n";

    // Save current driver settings before switching to a different one
    SessionManager::instance().rememberDriverState(d->m_acquisitionName);

    // Delete wrapper before the old driver (and its parameters) are destroyed.
    d->m_channelNamesObj.reset();

    if (!d->m_acqManager.addAcquisitionDriver(LOCAL_CLIENT_ID, d->m_acquisitionName, driverName.toStdString())) {
        d->getLogManager() << Kernel::LogLevel_Error << "Failed to set driver: " << driverName.toStdString() << ".\n";
    }

    // Apply new driver settings if any were saved
    SessionManager::instance().applyDriverState(d->m_acquisitionName);

    d->wrapDriverChannelNames();

    emit driverChanged();
    emit driverParametersChanged();
    emit channelNamesChanged();
    emit driverStateChanged();
    emit supportsImpedanceCheckChanged();

    // Save acquisition state
    saveSession();
}

double AcquisitionController::instantDrift()
{
    // Return drift in ms
    return d->m_instantDrift;
}

double AcquisitionController::globalDrift()
{
    // Return drift in ms
    return d->m_globalDrift;
}

double AcquisitionController::driftLimit()
{
    // Return drift limit in ms
    return d->m_driftLimit;
}

bool AcquisitionController::connectDevice()
{
    d->getLogManager() << Kernel::LogLevel_Debug <<  "Connecting device.\n";
    return false;
}

bool AcquisitionController::disconnectDevice()
{
    d->getLogManager() << Kernel::LogLevel_Info << "Disconnecting device.\n";
    return false;
}

bool AcquisitionController::startAcquisition()
{
    d->getLogManager() << Kernel::LogLevel_Info << "Starting acquisition.\n";

    // UG fork: Charmeleon holds the amplifier while it runs
    if (const int closed = Charmeleon::closeAll(); closed > 0) {
        d->getLogManager() << Kernel::LogLevel_Info << "Closed " << closed << " running Charmeleon instance(s).\n";
    }
    // UG fork: before the driver starts, so that it records with the montage as Charmeleon saved it on closing
    takeOverCharmeleonMontage();

    // If transitioning from impedance check, stop the polling timer first.
    if (d->m_impedanceTimer) {
        d->m_impedanceTimer->stop();
        delete d->m_impedanceTimer;
        d->m_impedanceTimer = nullptr;
    }

    if (!d->m_driftTimer) {
        d->m_driftTimer = new QTimer(this);
    }
    d->m_driftTimer->stop();
    d->m_driftTimer->setInterval(1000);
    connect(d->m_driftTimer, &QTimer::timeout, this, [this]() {
        d->m_instantDrift = (d->m_acquisition->drift().instantDrift / 1000000.0);
        d->m_globalDrift = (d->m_acquisition->drift().globalDrift / 1000000.0);
        emit driftChanged();
    });
    d->m_driftTimer->start();

    if (d->m_acqManager.startAcquisition(LOCAL_CLIENT_ID, d->m_acquisitionName)) {
        emit isAcquiringChanged();
        emit isCheckingImpedanceChanged();
        return true;
    }
    return false;
}

bool AcquisitionController::stopAcquisition()
{
    d->getLogManager() << Kernel::LogLevel_Info << "Stopping acquisition.\n";

    if (d->m_impedanceTimer) {
        d->m_impedanceTimer->stop();
        delete d->m_impedanceTimer;
        d->m_impedanceTimer = nullptr;
    }

    if (d->m_driftTimer) {
        d->m_driftTimer->stop();
        delete d->m_driftTimer;
        d->m_driftTimer = nullptr;
    }
    d->m_instantDrift = 0.0;
    d->m_globalDrift = 0.0;
    emit driftChanged();

    if (d->m_acqManager.stopAcquisition(LOCAL_CLIENT_ID, d->m_acquisitionName)) {
        //the end of a recording is the natural checkpoint: the settings that produced it are worth
        //keeping even if the application does not get to exit cleanly afterwards
        saveSession();
        emit isAcquiringChanged();
        emit isCheckingImpedanceChanged();
        return true;
    }
    return false;
}

void AcquisitionController::configureDeviceDriver()
{
    // Call driver configure
    //d->m_acquisitionServer->getDriver()->configure();
    d->getLogManager() << Kernel::LogLevel_Info << "Device driver configuration done.\n";
}

}  // namespace AcquisitionServer
}  // namespace OpenViBE
