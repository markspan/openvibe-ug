// SPDX-FileCopyrightText: Inria <florent.leray@inria.fr>
// SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
// SPDX-License-Identifier: AGPL-3.0-only

#include <QGuiApplication>
#include <QSettings>
#include <QQuickStyle>
#include <QLoggingCategory>
#include <QQmlApplicationEngine>
#include <QUrl>
#include <QStyleHints>
#include <QQmlContext>

#include <iostream>
#include "Acquisition.h"
#include "AcquisitionManager.h"


#include <CInitApp.h>

#include <openvibe/Services/IService.h>
#include <NetService.h>

int main(int argc, char *argv[])
{
    CInitApp appInit;

    appInit.loadSettings();
    appInit.loadServices();
    appInit.loadDriversAndWriters();

    AcquisitionManager& acqManager = AcquisitionManager::instance();

    if (appInit.isHeadlessMode()) {
        std::cout << "[Init] Running in headless mode, starting acquisition server" << std::endl;
        
        
        while(true);
    } else {
        QGuiApplication app(argc, argv);

        app.setApplicationName(QString::fromStdString(appInit.getAppName()));
        app.setOrganizationName("inria");
        app.setOrganizationDomain("inria.fr");
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