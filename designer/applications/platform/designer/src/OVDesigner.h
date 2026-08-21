/**
 * SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#include <QtCore>
#include <QtQml>

#include <QJSValue>

#include "DesignerScenario.h"
#include "BoxTreeModel.h"

namespace OpenViBE {
namespace Designer {

/**
 * @brief The OVDesigner class is the main interface between the GUI and OpenViBE internals.
 * it holds a list of scenarios and a player to run them.
 * It is a singleton that can be accessed from QML.
 * it also holds the tree model of all the available boxes.
*/
class OVDesigner : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QString playerStatus READ playerStatus NOTIFY playerStatusChanged)
    Q_PROPERTY(QString simulatedTime READ simulatedTime NOTIFY simulatedTimeChanged)
    Q_PROPERTY(QString simulatedTimeMs READ simulatedTimeMs NOTIFY simulatedTimeChanged)
    //TODO add signal toast to show messages to the user and ask for confirmation
    Q_PROPERTY(int currentScenarioIndex READ currentScenarioIndex WRITE setCurrentScenarioIndex NOTIFY currentScenarioIndexChanged)
    Q_PROPERTY(QQmlListProperty<DesignerScenario> scenarios READ scenarios NOTIFY scenariosChanged)
    Q_PROPERTY(QObject *boxesTreeModel READ boxesTreeModel NOTIFY boxesTreeModelChanged)

public:
    static OVDesigner *instance();
    static OVDesigner *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine)
    {
        OVDesigner *result = OVDesigner::instance();
        QJSEngine::setObjectOwnership(result, QJSEngine::CppOwnership);
        return result;
    }

    //set a command line parser to the options supported by the designer
    static void setParser(QCommandLineParser &);

    //Box Tree
    Q_INVOKABLE void filterBoxTree(const QString &filter, const QString& columnName);

    //player
    Q_INVOKABLE void startPlayer();
    Q_INVOKABLE void startPlayerFast();
    Q_INVOKABLE void stopPlayer();
    Q_INVOKABLE void pauseOrResumePlayer();

    //will read arguments from the command line
    Q_INVOKABLE void initialize();
    //create a scenario with a optional name and an optional file path
    Q_INVOKABLE void createScenario(const QString &scenarioName = "Unnamed", const QString& file_path = "");
    Q_INVOKABLE void saveScenario(int index, QUrl filePath);
    Q_INVOKABLE void removeScenario(int index);

    //save the scenarios currently opened so that they are reopened on the next run
    void saveSession();

    QString playerStatus() const;
    QString simulatedTime() const;
    QString simulatedTimeMs() const;
    bool threadFinished() const;
    int currentScenarioIndex() const;
    QQmlListProperty<DesignerScenario> scenarios();
    QObject *boxesTreeModel();

    void setCurrentScenarioIndex(int index);

signals:
    void playerStatusChanged();
    void simulatedTimeChanged();
    void currentScenarioIndexChanged();
    void scenariosChanged();
    void boxesTreeModelChanged();

    //Toasts Signals
    void toastMessage(const QString &message, const QString &type = "info");

protected:
    class OVDesignerPrivate *d;

private:
    OVDesigner();
    ~OVDesigner();
    static OVDesigner *s_instance;
};

}
}
