/**
 * SPDX-FileCopyrightText: Inria <thomas.prampart@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/ 

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQuickStyle>

#include "updateHandler.h"

int main(int argc, char *argv[]) {
    QGuiApplication app(argc, argv);

    QQuickStyle::setStyle("Imagine");

    QQmlApplicationEngine engine;

    UpdateHandler updateHandler;
    engine.rootContext()->setContextProperty("updateHandler", &updateHandler);

    engine.loadFromModule("openvibe-scenario-update-2to3", "Main");
    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}