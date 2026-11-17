// SPDX-FileCopyrightText: Inria <florent.leray@inria.fr>
// SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

#include <QGuiApplication>
#include <QIcon>
#include <QSettings>
#include <QQuickStyle>
#include <QLoggingCategory>
#include <QQmlApplicationEngine>
#include <QUrl>
#include <QStyleHints>
#include <QQmlContext>

#include <iostream>
#include <openvibe/ConsoleLog.hpp>  // UG fork
#include "Acquisition.h"
#include "AcquisitionManager.h"


#include <CInitApp.h>

#include <openvibe/Services/IService.h>
#include <NetService.h>

#include "Api.h"
#include "SessionManager.h"

#if defined TARGET_OS_Windows
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>  // UG fork: MessageBoxA
#endif

namespace {
/**
 * UG fork: a second acquisition server, started by mistake, used to run next to the first on the same port, and Designer
 * connected to either one: names and settings changed in one server did not reach the scenario. The network server now
 * refuses a port another server listens on (CServer::initConnection()); this says so, and stops, before anything else.
 */
bool portIsFree()
{
    Network::CServer& server = OpenViBE::Services::NetService::instance();
    if (server.isListening()) { return true; }
    const std::string text = "Port " + std::to_string(server.port())
                             + " is already in use, most likely by another OpenViBE acquisition server that is still running.\n\n"
                               "Close that one first, and start this one again. (To run two servers side by side, give the "
                               "running one another port under Preferences.)";
    std::cerr << "[Init] " << text << std::endl;
#if defined TARGET_OS_Windows
    MessageBoxA(nullptr, text.c_str(), "OpenViBE Acquisition Server", MB_OK | MB_ICONERROR);
#endif
    return false;
}
}  // namespace

int main(int argc, char *argv[])
{
    // UG fork: the server has no console window (WIN32_EXECUTABLE); what it prints goes to
    // <user data>/log/openvibe-acquisition-server-console.log, unless the parent redirected the output
    OpenViBE::redirectConsoleToLogFile("openvibe-acquisition-server-console");

    CInitApp appInit(argc, const_cast<char const**>(argv));

    appInit.loadSettings();
    appInit.loadServices();
    if (!portIsFree()) { return 1; }
    appInit.loadDriversAndWriters();

    // Shared default acquisition. Will be setup with restored session from SessionManager.
    // UI user and Network client can both control it, or just the Network client, in headless mode.
    if (!Api::createAcquisition(LOCAL_CLIENT_ID, DEFAULT_ACQUISITION_NAME, EOwnership::Shared)) {
        std::cerr << "[Init] Could not create the default acquisition" << std::endl;
    }
    OpenViBE::AcquisitionServer::SessionManager::instance().restoreAll(DEFAULT_ACQUISITION_NAME);

    if (appInit.isHeadlessMode()) {
        std::cout << "[Init] Running in headless mode, starting acquisition server" << std::endl;
        
        
        while(true);
    } else {
        QGuiApplication app(argc, argv);

        app.setApplicationName(QString::fromStdString(appInit.getAppName()));
        app.setOrganizationName("inria");
        app.setOrganizationDomain("inria.fr");
        // Set before the engine loads any QML, so every top-level window inherits it.
        app.setWindowIcon(QIcon(":/qt/qml/AcquisitionServer/ov-icon.ico"));
        QSettings::setDefaultFormat(QSettings::IniFormat);

        QQuickStyle::setStyle("Basic");
        QGuiApplication::styleHints()->setUseHoverEffects(true);
        QLoggingCategory::setFilterRules(QStringLiteral("qt.qml.binding.removal.info=true"));

        QQmlApplicationEngine engine;
        engine.addImportPath("qrc:/qt/qml/");

        #if defined TARGET_OS_Windows
        engine.rootContext()->setContextProperty("TARGET_OS_Windows", QVariant(true));
        #else
        engine.rootContext()->setContextProperty("TARGET_OS_Windows", QVariant(false));
        #endif

        engine.load(QUrl("qrc:/qt/qml/AcquisitionServer/ui/qml/Main.qml"));

        return app.exec();
    }

    return 0;
}
