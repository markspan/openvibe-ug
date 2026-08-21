/**
 * SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#include "OVDesigner.h"

#include <functional>
#include <memory>

#include <QtCore>
#include <QGuiApplication>
#include <QQmlContext>
#include <QQmlEngine>

#ifdef TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines
#include "ovp_global_defines.h"
#endif // TARGET_HAS_ThirdPartyOpenViBEPluginsGlobalDefines

#include <openvibe/ov_all.h>
#include <fs/Files.h>
#include <toolkit/ovtk_all.h>
#include <visualization-toolkit/ovviz_all.h>

#include <system/ovCTime.h>
#include <system/ovCMath.h>

#include <OVLogController.h>
#include <OVQmlLogListener.h>

#include "BoxTreeModel.h"
#include "BoxTreeModelFilter.h"
#include "defines.hpp"
#include "PlayerBridge.h"
#include "QmlEngineSingleton.h"
#include "SessionManager.h"

#define README_File                     OpenViBE::Directories::getDistRootDir() + "/ReadMe.txt"
#define SCENARIOS_PATH                  "${Path_Data}/scenarios"
#define WORKING_SCENARIOS_PATH          "${Designer_DefaultWorkingDirectory}/scenarios"

#define ScenarioImportContext_OpenScenario  OpenViBE::CIdentifier(0xA180DB91, 0x19235AEE)
#define ScenarioExportContext_SaveScenario  OpenViBE::CIdentifier(0xC98C47AD, 0xCBD952B2)
#define ScenarioExportContext_SaveMetabox   OpenViBE::CIdentifier(0x529494F1, 0x6C2527D9)


namespace OpenViBE {
namespace Designer {

struct OVDesignerPrivate {
    explicit OVDesignerPrivate(OVDesigner *parent): m_parent(parent) {}
    ~OVDesignerPrivate();

    bool openScenario(const std::string &filename);
    void launchScenario(Kernel::EPlayerStatus status, std::function<void()> statusChangedCallback); // can do play, play-fast or step

    OVDesigner *m_parent;
    CKernelLoader m_loader;
    Kernel::IKernelContext *m_context = nullptr;
    QList<DesignerScenario*> m_scenarios;
    BoxTreeModel *m_boxTreeModel = nullptr;
    BoxTreeModelFilter *m_boxProxyModel = nullptr;
    size_t m_CurrentScenarioIndex = -1;
    OVQmlLogListener* m_qmlLogListener = nullptr;
    QStringList streamTypes; //here to have a lifetime as long as the application
    QTimer m_simulatedTimeTimer;
    std::unique_ptr<SessionManager> m_session; //created by initialize(), once the command line is known
};

OVDesignerPrivate::~OVDesignerPrivate() { 
    //logMgr << Kernel::LogLevel_Info << "Application terminated, releasing allocated objects\n";

                //VisualizationToolkit::uninitialize(*context);
    //          Toolkit::uninitialize(*context);

    //          desc->releaseKernel(context);

    if(!m_scenarios.empty()) {
        for(auto *scenario : m_scenarios) {
            delete scenario;
        }
    }

    if(m_boxProxyModel) {
        delete m_boxProxyModel;
    }

    if(m_boxTreeModel) {
        delete m_boxTreeModel;
    }

    m_loader.uninitialize();
    m_loader.unload();

    if(m_qmlLogListener) {
        delete m_qmlLogListener;
        m_qmlLogListener = nullptr;
    }

}

bool OVDesignerPrivate::openScenario(const std::string &filename)
{
    CIdentifier scenarioID;
    if(!m_context->getScenarioManager().importScenarioFromFile(scenarioID, ScenarioImportContext_OpenScenario, filename.c_str())) {
        m_context->getLogManager() << Kernel::LogLevel_Error << "Importing scenario from file [" << filename << "] failed... "
            << " Current file either is corrupted or is not compatible with the selected scenario importer (ie not an OpenViBE scenario file)\n";
        return false;
    }

    // Closes first unnamed scenario
    if (m_scenarios.size() == 1) {
        if (m_scenarios[0]->isEmpty()) {
            const CIdentifier tmp = m_scenarios[0]->id();
            delete m_scenarios[0];
            m_context->getScenarioManager().releaseScenario(tmp);
            m_scenarios.clear();
        }
    }

    Kernel::IScenario& scenario = m_context->getScenarioManager().getScenario(scenarioID);
    DesignerScenario *newScenario = new DesignerScenario(m_context, scenario, scenarioID);
    QUrl scenarioUrl = QUrl::fromLocalFile(QString::fromStdString(filename));
    newScenario->setFilePath(scenarioUrl);
    m_scenarios.push_back(newScenario);

    return true;
}

void OVDesignerPrivate::launchScenario(Kernel::EPlayerStatus status, std::function<void()> statusChangedCallback)
{
    if (m_scenarios.empty()) {
        m_context->getLogManager() << Kernel::LogLevel_Error << "No scenario to play\n";
        return;
    }

    DesignerScenario *designerScenario = m_scenarios[m_CurrentScenarioIndex];
    if (designerScenario->isEmpty()) {
        m_context->getLogManager() << Kernel::LogLevel_Error << "Scenario is empty\n";
        return;
    }

    if (status != Kernel::EPlayerStatus::Play && status != Kernel::EPlayerStatus::Forward && status != Kernel::EPlayerStatus::Step) {
        m_context->getLogManager() << Kernel::LogLevel_Error << "Invalid status: " << toString(status) << " \n";
        return;
    }

    m_context->getLogManager() << Kernel::LogLevel_Debug << "Launching Scenario in mode " << toString(status) << " \n";
    auto &scenario = designerScenario->scenario();
    if (scenario.hasOutdatedBox()) {
        if (m_context->getConfigurationManager().expandAsBoolean("${Kernel_AbortPlayerWhenBoxIsOutdated}", false)) {
            std::string outdatedBoxesList = "You can not start the scenario because following boxes need to be updated: \n";
            CIdentifier boxID;
            while ((boxID = scenario.getNextOutdatedBoxIdentifier(boxID)) != CIdentifier::undefined()) {
                const Kernel::IBox* box = scenario.getBoxDetails(boxID);
                outdatedBoxesList += "\t[" + std::string(box->getName()) + "]\n";
            }
            outdatedBoxesList += "To update a box you need to delete it from scenario, and add it again.";
            
            m_context->getLogManager() << Kernel::LogLevel_Warning << outdatedBoxesList << "\n";
            //TODO add toast when it's implemented in the GUI

            //GtkWidget* dialog = gtk_message_dialog_new(nullptr, GtkDialogFlags(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT), GTK_MESSAGE_INFO,
            //                                            GTK_BUTTONS_OK, "%s", outdatedBoxesList.c_str());
            //gtk_dialog_run(GTK_DIALOG(dialog));
            //gtk_widget_destroy(dialog);
            return;
        }
        if (m_context->getConfigurationManager().expandAsBoolean("${Designer_ThrowPopUpWhenBoxIsOutdated}", false)) {
            std::string outdatedBoxesList = "The following boxes need to be updated: \n";
            CIdentifier boxID;
            while ((boxID = scenario.getNextOutdatedBoxIdentifier(boxID)) != CIdentifier::undefined()) {
                const Kernel::IBox* box = scenario.getBoxDetails(boxID);
                outdatedBoxesList += "\t[" + std::string(box->getName()) + "]\n";
            }
            outdatedBoxesList += "Do you still want to play the scenario ?";
            m_context->getLogManager() << Kernel::LogLevel_Warning << outdatedBoxesList << "\n";

            //GtkWidget* dialog = gtk_message_dialog_new(nullptr, GtkDialogFlags(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT), GTK_MESSAGE_QUESTION,
            //                                            GTK_BUTTONS_YES_NO, "%s", outdatedBoxesList.c_str());
            //const gint response = gtk_dialog_run(GTK_DIALOG(dialog));
            //gtk_widget_destroy(dialog);

            //if (response == GTK_RESPONSE_YES) {
            //    m_context->getLogManager() << Kernel::LogLevel_Trace << "CApplication::PlayScenarioCB - GTK_RESPONSE_YES: the scenario will be played. \n";
            //}
            //else {
            //    m_context->getLogManager() << Kernel::LogLevel_Trace << "CApplication::PlayScenarioCB - the scenario will not be played. \n";
            //    return;
            //}
        }
    }

    // create or get the player
    auto& player = designerScenario->createOrGetPlayer();
    if(player.isNull()) {
        m_context->getLogManager() << Kernel::LogLevel_Error << "The initialization of player failed. Check the above log messages to get the issue.\n";
        return;
    }

    player.setStatusChangedCallback(statusChangedCallback);

    designerScenario->windowManager()->show();
    QObject::connect(designerScenario->windowManager(),
            &WindowManager::windowCloseRequested,
            m_parent,
            &OVDesigner::stopPlayer);

    if(!player.start(status, [this]() {
            auto *scenario = this->m_scenarios[this->m_CurrentScenarioIndex];
            if(scenario) {
                scenario->releasePlayer();
            }
         }))
    {
        m_context->getLogManager() << Kernel::LogLevel_Error << "The player thread could not be started. Check the above log messages to get the issue.\n";
        return;
    }
}

OVDesigner *OVDesigner::instance() 
{
    if (!s_instance) {
        s_instance = new OVDesigner();
    }

    return s_instance;
}

void OVDesigner::setParser(QCommandLineParser& parser)
{
    parser.setApplicationDescription("OpenViBE Designer");
    parser.addHelpOption();
    parser.addVersionOption();

    parser.addOption({QStringList() << "o" << "open", QCoreApplication::translate("main", "Open a scenario file."), QCoreApplication::translate("main", "file-open")});
    parser.addOption({QStringList() << "p" << "play", QCoreApplication::translate("main", "Open and play a scenario file."), QCoreApplication::translate("main", "file-play")});
    parser.addOption({QStringList() << "pf" << "play-fast", QCoreApplication::translate("main", "Open and fast play a scenario file."), QCoreApplication::translate("main", "file-play-fast")});

    parser.addOption({QStringList() << "no-gui" << "invisible", QCoreApplication::translate("main", "Run without GUI.")});
    parser.addOption({"no-session-management", QCoreApplication::translate("main", "Do not manage session.")});
    parser.addOption({"random-seed", QCoreApplication::translate("main", "Set random seed."), QCoreApplication::translate("main", "seed")});

    parser.addOption({"config", QCoreApplication::translate("main", "Load a configuration file."), QCoreApplication::translate("main", "file-config")});
    parser.addOption({QStringList() << "d" << "define", QCoreApplication::translate("main", "Define a configuration token in the form name:value ."), QCoreApplication::translate("main", "key:value")});
    parser.process(*qGuiApp);
}

OVDesigner::OVDesigner() : d(new OVDesignerPrivate(this)) {
    //1. Initialize the kernel
    std::cout << "[  INF  ] Created kernel loader, trying to load kernel module" << "\n";
    CString errorMsg;

    CString file = Directories::getLib("kernel");

    if (!d->m_loader.load(file, &errorMsg)) 
    {
        std::cout << "[ FAILED ] Error loading kernel (" << errorMsg << ")" << " from [" << file << "]\n"; 
        return;
    }

    std::cout << "[  INF  ] Kernel module loaded, trying to get kernel descriptor" << "\n";
    Kernel::IKernelDesc* desc = nullptr;
    d->m_loader.initialize();
    d->m_loader.getKernelDesc(desc);
    if (desc == nullptr) { 
        std::cout << "[ FAILED ] No kernel descriptor" << "\n";
        return;
    }

    std::cout << "[  INF  ] Got kernel descriptor, trying to create kernel" << "\n";

    Kernel::IKernelContext* context = desc->createKernel("designer", Directories::getDataDir() + "/kernel/openvibe.conf");
    context->initialize();
    d->m_context = context;
    
    //create qml log controller
    d->m_qmlLogListener = new OVQmlLogListener();
    d->m_context->getLogManager().addListener(d->m_qmlLogListener);

    //Connect error and warning to toast
    connect(OVLogController::instance(), &OVLogController::newError, [this](const QString& message) {
        emit toastMessage(message, "error");
    });

    connect(OVLogController::instance(), &OVLogController::newWarning, [this](const QString& message) {
        emit toastMessage(message, "warning");
    });

    context->getConfigurationManager().addConfigurationFromFile(Directories::getDataDir() + "/applications/designer/designer.conf");
    CString appConfigFile = context->getConfigurationManager().expand("${Designer_CustomConfigurationFile}");
    context->getConfigurationManager().addConfigurationFromFile(appConfigFile);

    VisualizationToolkit::initialize(*context);

    d->m_simulatedTimeTimer.setInterval(50);
    connect(&d->m_simulatedTimeTimer, &QTimer::timeout, this, &OVDesigner::simulatedTimeChanged);
}

OVDesigner::~OVDesigner() {
    delete d;
}

void OVDesigner::startPlayer() {
    auto *scenario = d->m_scenarios[d->m_CurrentScenarioIndex];
    if(!scenario) {
        return;
    }

    auto &player = scenario->currentPlayer();
    if(player.isNull()) {
        d->launchScenario(Kernel::EPlayerStatus::Play, [this]() { emit playerStatusChanged(); });
    } else {
        player.start(Kernel::EPlayerStatus::Play);
    }
    d->m_simulatedTimeTimer.start();
}

void OVDesigner::startPlayerFast() {
    auto *scenario = d->m_scenarios[d->m_CurrentScenarioIndex];
    if(!scenario) {
        return;
    }

    auto &player = scenario->currentPlayer();
    if(player.isNull()) {
        d->launchScenario(Kernel::EPlayerStatus::Forward, [this]() { emit playerStatusChanged(); });
    } else {
        player.start(Kernel::EPlayerStatus::Forward);
    }
    d->m_simulatedTimeTimer.start();
}

bool OVDesigner::threadFinished() const {
    if (d->m_scenarios.empty()) {
        return true;
    }
    return d->m_scenarios[d->m_CurrentScenarioIndex]->currentPlayer().threadFinished;
}

void OVDesigner::stopPlayer() {
    auto *scenario = d->m_scenarios[d->m_CurrentScenarioIndex];
    if(!scenario) {
        return;
    }

    scenario->windowManager()->hide();
    scenario->currentPlayer().stop();
        //TODO restore initial state

    //for (const auto elem : m_boxConfigDialogs) {
    //  elem->RestoreState();
    //  delete elem;
    //}
    //m_boxConfigDialogs.clear();
    d->m_simulatedTimeTimer.stop();
    emit simulatedTimeChanged();
}

void OVDesigner::pauseOrResumePlayer() {
    auto *scenario = d->m_scenarios[d->m_CurrentScenarioIndex];
    if(!scenario) {
        return;
    }

    auto &player = scenario->currentPlayer();
    if(player.getStatus() == Kernel::EPlayerStatus::Pause) {
        player.start(Kernel::EPlayerStatus::Play);
        d->m_simulatedTimeTimer.start();
    } else {
        player.pause();
        d->m_simulatedTimeTimer.stop();
    }
}


void OVDesigner::initialize() {

    // 1. Create an option parser
    // --------------------------------------------------------------------------------------------
    QCommandLineParser parser;
    OVDesigner::setParser(parser);
    bool bNoGUI = parser.isSet("no-gui");

    // add other configuration file if --config option
    if(parser.isSet("config")) {
        CString appConfigFile = parser.value("config").toStdString().c_str();
        d->m_context->getConfigurationManager().addConfigurationFromFile(appConfigFile);
    }

    if(parser.isSet("random-seed")) {
        const size_t seed = size_t(strtol(parser.value("random-seed").toStdString().c_str(), nullptr, 10));
        System::Math::initializeRandomMachine(seed);
    } else {
        // initialize random number generator with nullptr by default
        System::Math::initializeRandomMachine(time(nullptr));
    }
    // --------------------------------------------------------------------------------------------


    // 2. Initialize OpenViBE internals
    // --------------------------------------------------------------------------------------------
    Toolkit::initialize(*d->m_context);

    Kernel::IConfigurationManager& configMgr = d->m_context->getConfigurationManager();
    Kernel::ILogManager& logMgr              = d->m_context->getLogManager();
    Kernel::IScenarioManager* scenarioMgr = &d->m_context->getScenarioManager();
    d->m_context->getPluginManager().addPluginsFromFiles(configMgr.expand("${Kernel_Plugins}"));

    //check if we have to run without GUI
    //bool noGui = parser.isSet("no-gui");
    //bool noSessionManagement = parser.isSet("no-session-management");
    //if (configMgr.expandAsBoolean("${Kernel_WithGUI}", false) && !noGui) {
    //    logMgr << Kernel::LogLevel_ImportantWarning <<
    //            "${Kernel_WithGUI} is set to false and --no-gui flag not set. Forcing the --no-gui flag\n";
    //    noGui = true;
    //    noSessionManagement = true;
    //}
    //
    //// if no GUIs set and no play operation is requested, exit the designer
    //if (noGui && (!parser.isSet("play") && !parser.isSet("play-fast"))) {
    //    logMgr << Kernel::LogLevel_Info
    //           << "Switch --no-gui is enabled but no play operation was requested. Designer will exit automatically.\n";
    //    qGuiApp->exit(0);
    //}

    //register the scenarios
    scenarioMgr->registerScenarioImporter(ScenarioImportContext_OpenScenario, ".xml", OVP_GD_ClassId_Algorithm_XMLScenarioImporter);
    scenarioMgr->registerScenarioImporter(ScenarioImportContext_OpenScenario, ".mxs", OVP_GD_ClassId_Algorithm_XMLScenarioImporter);
    scenarioMgr->registerScenarioImporter(ScenarioImportContext_OpenScenario, ".mxb", OVP_GD_ClassId_Algorithm_XMLScenarioImporter);
    scenarioMgr->registerScenarioExporter(ScenarioExportContext_SaveScenario, ".xml", OVP_GD_ClassId_Algorithm_XMLScenarioExporter);
    scenarioMgr->registerScenarioExporter(ScenarioExportContext_SaveScenario, ".mxs", OVP_GD_ClassId_Algorithm_XMLScenarioExporter);
    scenarioMgr->registerScenarioExporter(ScenarioExportContext_SaveMetabox, ".xml", OVP_GD_ClassId_Algorithm_XMLScenarioExporter);
    scenarioMgr->registerScenarioExporter(ScenarioExportContext_SaveMetabox, ".mxb", OVP_GD_ClassId_Algorithm_XMLScenarioExporter);
    configMgr.createConfigurationToken("Player_ScenarioDirectory", "");
    configMgr.createConfigurationToken("__volatile_ScenarioDir", "");

    d->m_context->getMetaboxManager().addMetaboxesFromFiles(configMgr.expand("${Kernel_Metabox}"));

    // Copy recursively default scenario directory to the default working directory if not exists
    const CString defaultWorkingDirectory   = configMgr.expand(WORKING_SCENARIOS_PATH);
    const CString defaultScenariosDirectory = configMgr.expand(SCENARIOS_PATH);
    if (!FS::Files::directoryExists(defaultWorkingDirectory) && FS::Files::directoryExists(defaultScenariosDirectory)) {
        if (!FS::Files::copyDirectory(defaultScenariosDirectory, defaultWorkingDirectory)) {
            logMgr << Kernel::LogLevel_Error << "Could not create " << defaultWorkingDirectory << " folder\n";
        }
    }
    // --------------------------------------------------------------------------------------------


    // 3. Initialize a scenario
    // --------------------------------------------------------------------------------------------
    // Add or replace a configuration token if required in command line
    if (parser.isSet("define")) {
        QStringList tokens = parser.values("define");
        for (const auto& t : tokens) {
            QStringList token = t.split(":");
            if (token.size() == 2) {
                logMgr << Kernel::LogLevel_Trace << "Adding command line configuration token [" << token[0].toStdString() << " = " << token[1].toStdString() << "]\n";
                configMgr.addOrReplaceConfigurationToken(token[0].toStdString().c_str(), token[1].toStdString().c_str());
            } else {
                logMgr << Kernel::LogLevel_Error << "Invalid token definition: " << t.toStdString() << "\n";
            }
        }
    }

    // Reopen the scenarios that were opened when the designer was last closed. Done before the
    // command line is honoured so that a scenario given on the command line is appended last and
    // ends up being the current one, as in the legacy designer.
    const bool manageSession = !parser.isSet("no-session-management") && !bNoGUI;
    d->m_session             = std::make_unique<SessionManager>(*d->m_context, manageSession);
    connect(qApp, &QCoreApplication::aboutToQuit, this, &OVDesigner::saveSession);

    int restoredScenarioIndex = -1;
    const SessionManager::Session session = d->m_session->restore();
    for (int i = 0; i < session.scenarioFiles.size(); ++i) {
        const QString& scenarioFile = session.scenarioFiles[i];
        if (!QFileInfo::exists(scenarioFile)) {
            logMgr << Kernel::LogLevel_Warning << "Not restoring scenario [" << scenarioFile.toStdString() << "]: the file no longer exists\n";
            continue;
        }
        logMgr << Kernel::LogLevel_Trace << "Restoring scenario [" << scenarioFile.toStdString() << "]\n";
        if(!d->openScenario(scenarioFile.toStdString())) {
            continue; //openScenario already reported the failure
        }
        //compute the index from the tabs actually restored, some of them may have failed to open
        if (i == session.selectedIndex) {
            restoredScenarioIndex = int(d->m_scenarios.size()) - 1;
        }
    }

    if(parser.isSet("open") || parser.isSet("play") || parser.isSet("play-fast")) {
        std::string fileName;
        if(parser.isSet("open"))      fileName = parser.value("open").toStdString();
        if(parser.isSet("play"))      fileName = parser.value("play").toStdString();
        if(parser.isSet("play-fast")) fileName = parser.value("play-fast").toStdString();
        auto BackslashToSlash = [](const char c) { return c == '\\' ? '/' : c; };
        std::transform(fileName.begin(), fileName.end(), fileName.begin(), BackslashToSlash);
        if(d->openScenario(fileName)) {
            this->setCurrentScenarioIndex(d->m_scenarios.length() - 1);

            if(parser.isSet("play")) {
                this->startPlayer();
            } else if(parser.isSet("play-fast")) {
                this->startPlayerFast();
            }
        } else if (bNoGUI) {
            logMgr << Kernel::LogLevel_Error << "Failed to open scenario file [" << fileName << "]\n";
            return;
        }
    } else if (restoredScenarioIndex != -1) {
        //no scenario was requested on the command line, so bring back the one that was displayed
        this->setCurrentScenarioIndex(restoredScenarioIndex);
    }

    if (d->m_scenarios.empty()) {
        CIdentifier identifier;
        if(!d->m_context->getScenarioManager().createScenario(identifier)) {
            logMgr << Kernel::LogLevel_Error << "Failed to create a new scenario! I will crash\n";
        }
        d->m_scenarios.push_back(new DesignerScenario(d->m_context, d->m_context->getScenarioManager().getScenario(identifier), identifier));
        this->setCurrentScenarioIndex(d->m_scenarios.length() - 1);
    }

    if (int(d->m_CurrentScenarioIndex) < 0) {
        //scenarios were restored but none of them was the one displayed on the previous run
        this->setCurrentScenarioIndex(0);
    }
    // --------------------------------------------------------------------------------------------

    // 4. Initialize the BoxTreeModel (available boxes)
    // --------------------------------------------------------------------------------------------

    CIdentifier id;
    d->m_boxTreeModel = new BoxTreeModel();

    //equivalent to old m_BoxAlgorithmTreeModel (openvibe-box_algorithm_tree)
    // 169 entries with box algorithm.
    while ((id = d->m_context->getPluginManager().getNextPluginObjectDescIdentifier(id, OV_ClassId_Plugins_BoxAlgorithmDesc)) != CIdentifier::undefined()) {
        const Plugins::IPluginObjectDesc* pluginDesc = d->m_context->getPluginManager().getPluginObjectDesc(id);
        QString name = QString::fromStdString(pluginDesc->getName().toASCIIString());
        QString author = QString::fromStdString(pluginDesc->getAuthorName().toASCIIString());
        QString description = QString::fromStdString(pluginDesc->getShortDescription().toASCIIString());
        QString category = QString::fromStdString(pluginDesc->getCategory().toASCIIString());
        if(category.isEmpty()) category = "Uncategorized";

        d->m_boxTreeModel->addData(name, author, category, description, pluginDesc->getCreatedClass().str().c_str());
    }

    // 2 metaboxes
    while ((id = d->m_context->getMetaboxManager().getNextMetaboxObjectDescIdentifier(id)) != CIdentifier::undefined()) {
        const Plugins::IPluginObjectDesc* pluginDesc = d->m_context->getMetaboxManager().getMetaboxObjectDesc(id);
        QString name = QString::fromStdString(pluginDesc->getName().toASCIIString());
        QString author = QString::fromStdString(pluginDesc->getAuthorName().toASCIIString());
        QString description = QString::fromStdString(pluginDesc->getShortDescription().toASCIIString());
        QString category = QString::fromStdString(pluginDesc->getCategory().toASCIIString());
        if(category.isEmpty()) category = "Uncategorized";

        d->m_boxTreeModel->addData(name, author, category, description, pluginDesc->getCreatedClass().str().c_str());
    }


    // equivalent to old m_AlgorithmTreeModel (openvibe-algorithm_tree in the .ui file)
    // 65 entries with algorithm . It's not clear where they were previously displayed in the GTK interface.
    /*
    id = CIdentifier::undefined();
    while ((id = d->m_context->getPluginManager().getNextPluginObjectDescIdentifier(id, OV_ClassId_Plugins_AlgorithmDesc)) != CIdentifier::undefined()) {
        const Plugins::IPluginObjectDesc* pluginDesc = d->m_context->getPluginManager().getPluginObjectDesc(id);
        QString name = QString::fromStdString(pluginDesc->getName().toASCIIString());
        QString author = QString::fromStdString(pluginDesc->getAuthorName().toASCIIString());
        QString description = QString::fromStdString(pluginDesc->getShortDescription().toASCIIString());
        QString category = QString::fromStdString(pluginDesc->getCategory().toASCIIString());
        if(category.isEmpty()) category = "Uncategorized";

        d->m_boxTreeModel->addData(name, author, category, description, id.str().c_str());
    }
    //qDebug() << "BoxTreeModel initialized rows: " << d->m_boxTreeModel->entryCount();

    */

    d->m_boxProxyModel = new BoxTreeModelFilter();
    d->m_boxProxyModel->setSourceModel(d->m_boxTreeModel);
    d->m_boxProxyModel->sort(0);
    d->m_boxProxyModel->setRecursiveFilteringEnabled(true);
    d->m_boxProxyModel->setFilterCaseSensitivity(Qt::CaseInsensitive);
    // --------------------------------------------------------------------------------------------

    // 5. Init DataTypeModel
    // --------------------------------------------------------------------------------------------
    const auto& typeManager = d->m_context->getTypeManager();
    const auto& types = typeManager.getSortedTypes();
    for(const auto& type : types) {
        if(typeManager.isStream(type.first)) {
            d->streamTypes << QString::fromStdString(type.second.toASCIIString());
        }
    }
    QmlEngineSingleton::getInstance().engine()->rootContext()->setContextProperty("StreamTypesModel", d->streamTypes);

    // --------------------------------------------------------------------------------------------


    emit boxesTreeModelChanged();
    emit scenariosChanged();
    emit currentScenarioIndexChanged();  
}

void OVDesigner::createScenario(const QString &scenarioName, const QString& filePath)
{
    static int counter = 1;

    //if there is a filePath use it
    if(!filePath.isEmpty()) {
        if(!d->openScenario(filePath.toStdString())) {
            d->m_context->getLogManager() << Kernel::LogLevel_Error << "Failed to open scenario file [" << filePath.toStdString() << "]\n";
            return;
        }
    } else {
        //create a new scenario
        CIdentifier identifier;
        if(!d->m_context->getScenarioManager().createScenario(identifier)) {
            d->m_context->getLogManager() << Kernel::LogLevel_Error << "Failed to create a new scenario! I may crash\n";
            return;
        }
    
        DesignerScenario *scenario = new DesignerScenario(d->m_context, d->m_context->getScenarioManager().getScenario(identifier), identifier);
        if(scenarioName == "Unnamed") {
            scenario->setName("Unnamed " + QString::number(counter++));
        } else {
            scenario->setName(scenarioName);
        }

        d->m_scenarios.push_back(scenario);
    }
    emit scenariosChanged();

    this->setCurrentScenarioIndex(d->m_scenarios.length() - 1);
    this->saveSession();
}

void OVDesigner::saveSession() {
    if(!d->m_session) {
        return; //initialize() has not run yet
    }

    SessionManager::Session session;
    for(int i = 0; i < d->m_scenarios.size(); ++i) {
        const QUrl filePath = d->m_scenarios[i]->filePath();
        if(!filePath.isLocalFile()) {
            continue; //a scenario that was never saved has nothing to restore from
        }
        if(int(d->m_CurrentScenarioIndex) == i) {
            session.selectedIndex = int(session.scenarioFiles.size());
        }
        session.scenarioFiles << filePath.toLocalFile();
    }

    d->m_session->save(session);
}

void OVDesigner::saveScenario(int index, QUrl filePath) {
    if(index < 0 || index >= d->m_scenarios.size()) {
        d->m_context->getLogManager() << Kernel::LogLevel_Error << "Invalid scenario index " << index << "\n";
        return;
    }

    QString fileString;
    if(filePath.isLocalFile()) {
        fileString = filePath.toLocalFile();
    } else {
        fileString = filePath.toString();
    }
    QFileInfo fileInfo(fileString);

    CIdentifier exportContext;
    if(fileInfo.suffix() ==".mxb") {
        exportContext = ScenarioExportContext_SaveMetabox;
    } else {
        exportContext = ScenarioExportContext_SaveScenario;
        //add the extension if not present
        if(fileInfo.suffix().isEmpty()) {
            fileString += ".xml";
        }
    }
    //the extension may have just been added, so the scenario must be told about the file that is
    //really written, otherwise its tab label and the saved session point to a nonexistent file
    d->m_scenarios[index]->setFilePath(filePath.isLocalFile() ? QUrl::fromLocalFile(fileString) : QUrl(fileString));
    d->m_context->getScenarioManager().exportScenarioToFile(exportContext,
                                                            fileString.toStdString().c_str(),
                                                            d->m_scenarios[index]->id());

    this->saveSession();
}

void OVDesigner::removeScenario(int index) {
    if(index < 0 || index >= d->m_scenarios.size()) {
        return;
    }

    //if we remove the current scenario, we need to set the current scenario to the next or previous one
    int new_scenarioIndex = -1;
    if(int(d->m_CurrentScenarioIndex) == index) {
        if(index > 0) {
            new_scenarioIndex = index - 1;
        } else {
            //check if we have a next scenario
            if(d->m_scenarios.size() > 1) {
                // we will remove the scenario with index, so the next one will have a new index = index
                new_scenarioIndex = index;
            } else {
                //we have only one scenario and we remove it. We will create a new one
                this->createScenario();
            }
        }
    }

    //if we remove a scenario before the current one, we need to decrement the index
    if(int(d->m_CurrentScenarioIndex) > index) {
        new_scenarioIndex = d->m_CurrentScenarioIndex - 1;
    }

    auto *scenario = d->m_scenarios[index];
    if(scenario) {
        const CIdentifier tmp = scenario->id();
        delete scenario;
        d->m_context->getScenarioManager().releaseScenario(tmp);
        d->m_scenarios.removeAt(index);
    }

    if(new_scenarioIndex != -1) {
        this->setCurrentScenarioIndex(new_scenarioIndex);
    }

    emit scenariosChanged();
    this->saveSession();
}

int OVDesigner::currentScenarioIndex() const {
    return d->m_CurrentScenarioIndex;
}

void OVDesigner::setCurrentScenarioIndex(int index) {
    if(index < 0 || index >= d->m_scenarios.size() || index == int(d->m_CurrentScenarioIndex)) {
        return;
    }
    d->m_CurrentScenarioIndex = index;

    QUrl filepath = d->m_scenarios[d->m_CurrentScenarioIndex]->filePath();
    QString path = filepath.isLocalFile() ? filepath.toLocalFile() : filepath.toString();
    if(!path.isEmpty()) {
        QFileInfo fileInfo(path);
        path = fileInfo.absolutePath();
    }

    auto& configMgr = d->m_context->getConfigurationManager();
    configMgr.setConfigurationTokenValue(configMgr.lookUpConfigurationTokenIdentifier("Player_ScenarioDirectory"), path.toStdString().c_str());
    configMgr.setConfigurationTokenValue(configMgr.lookUpConfigurationTokenIdentifier("__volatile_ScenarioDir"),   path.toStdString().c_str());

    emit currentScenarioIndexChanged();
}

QString OVDesigner::playerStatus() const {
    if(!d->m_scenarios.empty()) {
        auto *currentScenario = d->m_scenarios[d->m_CurrentScenarioIndex];
        if(currentScenario) {
            auto& player = currentScenario->currentPlayer();
            if(!player.isNull()) {
                Kernel::EPlayerStatus status = player.getStatus();
                return QString::fromStdString(toString(status));
            }
            return QString::fromStdString(toString(Kernel::EPlayerStatus::Stop));
        }
    }

    return "Invalid Status";
}

QString OVDesigner::simulatedTime() const {
    if(!d->m_scenarios.empty()) {
        auto *currentScenario = d->m_scenarios[d->m_CurrentScenarioIndex];
        if(currentScenario) {
            auto& player = currentScenario->currentPlayer();
            if(!player.isNull()) {
                const double time = player.getCurrentSimulatedTime();

                const int hours = int(time) / 3600;
                const int minutes = int(time) % 3600 / 60;
                const int seconds = int(time) % 60;
                return QString("%1:%2:%3")
                        .arg(hours, 2, 10, QChar('0'))
                        .arg(minutes, 2, 10, QChar('0'))
                        .arg(seconds, 2, 10, QChar('0'));
            }
        }
    }
    return QString("00:00:00");
}

QString OVDesigner::simulatedTimeMs() const {
    if(!d->m_scenarios.empty()) {
        auto *currentScenario = d->m_scenarios[d->m_CurrentScenarioIndex];
        if(currentScenario) {
            auto& player = currentScenario->currentPlayer();
            if(!player.isNull()) {
                const double time = player.getCurrentSimulatedTime();
                const int milliseconds = int(time * 1000) % 1000;
                return QString("%1").arg(milliseconds, 3, 10, QChar('0'));
            }
        }
    }
    return QString("000");
}

QQmlListProperty<DesignerScenario> OVDesigner::scenarios() {
    return QQmlListProperty<DesignerScenario>(this, &d->m_scenarios);
}

QObject *OVDesigner::boxesTreeModel() {    
    return d->m_boxProxyModel;
}

void OVDesigner::filterBoxTree(const QString &filter, const QString& columnName)
{
    //columns are name/category, author, description, boxAlgorithmClassID
    //we allow to filter on name/category and author
    int intColumn = 0;
    if(columnName.toLower() == "author") {
        intColumn = 1;
    }

    d->m_boxProxyModel->setFilterKeyColumn(intColumn);
    d->m_boxProxyModel->setFilterFixedString(filter);

}

OVDesigner *OVDesigner::s_instance = nullptr;

}
}