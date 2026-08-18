#include <QGuiApplication>
#include <QStyleHints>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QtQml/qqmlextensionplugin.h>
#include <QLoggingCategory>
#include <QtCore>
#include <QQmlContext>


int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    app.setApplicationName("OpenViBE Acquisition Server");
    app.setOrganizationName("inria");
    app.setOrganizationDomain("inria.fr");

    QQuickStyle::setStyle("Basic");
    QGuiApplication::styleHints()->setUseHoverEffects(true);
    QLoggingCategory::setFilterRules(QStringLiteral("qt.qml.binding.removal.info=true"));

    /*QDirIterator it(":", QDirIterator::Subdirectories);
    while (it.hasNext()) {
        qDebug() << it.next();
    }*/

    QQmlApplicationEngine engine;
    engine.addImportPath("qrc:/qt/qml/");

#if defined TARGET_OS_Windows
    engine.rootContext()->setContextProperty("TARGET_OS_Windows", QVariant(true));
#else
    engine.rootContext()->setContextProperty("TARGET_OS_Windows", QVariant(false));
#endif

    engine.load(QUrl("qrc:/qt/qml/openvibe-acquisition-server-qml/main.qml"));

    return app.exec();
}
