/**
 * SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/ 

#include <QApplication>
#include <QStyleHints>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QtQml/qqmlextensionplugin.h>
#include <QLoggingCategory>
#include <QtCore>

#include "QmlEngineSingleton.h"
#include "OVDesigner.h"
#include "WindowManagerNode.h"
#include "system/OSAL.h"

#ifdef WIN32
#include <system/WindowsUtilities.h>
#endif

#include <chrono>
#include <cstdio>
#include <iostream>
#include <thread>

#if defined TARGET_OS_Linux
#include <csignal>

void sigHandler(int s)
{
    std::signal(s, SIG_DFL);
    qApp->quit();
}
#endif

int main(int argc, char *argv[])
{
#ifdef WIN32
        // The GUI subsystem gives the process no console. Attach the parent's so logs stay visible
        // when launched from a terminal. Skipped when stdout is already valid, i.e. the parent
        // redirected it to a pipe or file -- reopening CONOUT$ would discard that output.
        const HANDLE existingOut = GetStdHandle(STD_OUTPUT_HANDLE);
        if ((existingOut == nullptr || existingOut == INVALID_HANDLE_VALUE) && AttachConsole(ATTACH_PARENT_PROCESS)) {
            FILE* stream = nullptr;
            freopen_s(&stream, "CONOUT$", "w", stdout);
            freopen_s(&stream, "CONOUT$", "w", stderr);
            std::ios::sync_with_stdio();
        }

        SetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_DEFAULT_DIRS | LOAD_LIBRARY_SEARCH_USER_DIRS);
        // Python DLL location for delay-load hook
        OpenViBE::CString pythonRuntimeDir = OpenViBE::Directories::getBinDir() + "/python_runtime";
        AddDllDirectory(System::WindowsUtilities::convertUtf8ToWide(OSAL::normalizedPath(pythonRuntimeDir.toASCIIString())).c_str());
#endif

    QLocale::setDefault(QLocale::C);
    QQuickWindow::setGraphicsApi(QSGRendererInterface::OpenGLRhi);
    QApplication app(argc, argv); //needed for Qt Charts
    app.setApplicationName("OpenViBE Designer");
    app.setOrganizationName("inria");
    app.setOrganizationDomain("inria.fr");
    app.setWindowIcon(QIcon(":/qt/qml/designer/ov-icon.ico"));
    QSettings::setDefaultFormat(QSettings::IniFormat);

    QQuickStyle::setStyle("Basic");
    QGuiApplication::styleHints()->setUseHoverEffects(true);
    QLoggingCategory::setFilterRules(QStringLiteral("qt.qml.binding.removal.info=true"));

    QQmlApplicationEngine engine;
    QmlEngineSingleton::getInstance().setEngine(&engine);

    //for debug purposes
    //QDirIterator it(":", QDirIterator::Subdirectories);
    //while (it.hasNext()) {
    //    qDebug() << it.next();
    //}

#if defined TARGET_OS_Linux
    //catch SIGINT and SIGTERM to quit the application
    signal(SIGINT, sigHandler);
    signal(SIGTERM, sigHandler);
#endif

    //check if the arguments are --no-gui or --no-session-management
    QCommandLineParser parser;
    OpenViBE::Designer::OVDesigner::setParser(parser);
    auto *designer = OpenViBE::Designer::OVDesigner::instance();
    designer->initialize();
    if(parser.isSet("no-gui") && parser.isSet("no-session-management")) {
        while(!designer->threadFinished()) {
            //wait for the player to stop
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        return 0;
    } else {
        // GUI mode: the QML engine will access the designer instance
        designer = nullptr;

    #if defined TARGET_OS_Windows
        engine.rootContext()->setContextProperty("TARGET_OS_Windows", QVariant(true));
    #else
        engine.rootContext()->setContextProperty("TARGET_OS_Windows", QVariant(false));
    #endif
    
        //engine.addImportPath(QML_PATH);
        engine.loadFromModule("designer", "Main");
        return app.exec();
    }
}
