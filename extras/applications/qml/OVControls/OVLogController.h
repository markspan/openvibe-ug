/**
 * SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#pragma once

#include <QtCore>
#include <QtQml>

class OVLogControllerItem;

class OVLogController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON

    Q_PROPERTY(QQmlListProperty<OVLogControllerItem> items READ items NOTIFY itemsChanged)
    Q_PROPERTY(int infoCount READ infoCount NOTIFY infoCountChanged)
    Q_PROPERTY(int warningCount READ warningCount NOTIFY warningCountChanged)
    Q_PROPERTY(int errorCount READ errorCount NOTIFY errorCountChanged)
    Q_PROPERTY(int debugCount READ debugCount NOTIFY debugCountChanged)
    Q_PROPERTY(bool displayDebug READ displayDebug WRITE setDisplayDebug NOTIFY displayDebugChanged)
    Q_PROPERTY(bool displayInfo READ displayInfo WRITE setDisplayInfo NOTIFY displayInfoChanged)
    Q_PROPERTY(bool displayWarning READ displayWarning WRITE setDisplayWarning NOTIFY displayWarningChanged)
    Q_PROPERTY(bool displayError READ displayError WRITE setDisplayError NOTIFY displayErrorChanged)

public:
    static OVLogController *instance();
    static OVLogController *create(QQmlEngine *qmlEngine, QJSEngine *jsEngine)
    {
        OVLogController *result = OVLogController::instance();
        //QJSEngine::setObjectOwnership(result, QJSEngine::CppOwnership);
        return result;
    }


signals:
    void itemsChanged();
    void infoCountChanged();
    void warningCountChanged();
    void errorCountChanged();
    void debugCountChanged();
    void displayDebugChanged();
    void displayInfoChanged();
    void displayWarningChanged();
    void displayErrorChanged();

    //for toasts
    void newError(const QString& message);
    void newWarning(const QString& message);

public:

    QQmlListProperty<OVLogControllerItem> items();
    int infoCount() const;
    int warningCount() const;
    int errorCount() const;
    int debugCount() const;
    bool displayDebug() const;
    void setDisplayDebug(bool displayDebug);
    bool displayInfo() const;
    void setDisplayInfo(bool displayInfo);
    bool displayWarning() const;
    void setDisplayWarning(bool displayWarning);
    bool displayError() const;
    void setDisplayError(bool displayError);

public:
    Q_INVOKABLE void clear();
    void addLogItem(const QString &message, const QString &level);

private:
    OVLogController();
    ~OVLogController();

    void addLogItemAndFlush();
    static OVLogController *s_instance;
    QList<OVLogControllerItem *> m_itemsDebug;
    QList<OVLogControllerItem *> m_itemsInfo;
    QList<OVLogControllerItem *> m_itemsWarning;
    QList<OVLogControllerItem *> m_itemsError;

    QList<OVLogControllerItem *> m_items; // all items displayed
    int m_infoCount = 0;
    int m_warningCount = 0;
    int m_errorCount = 0;
    int m_debugCount = 0;
    bool m_displayDebug = false;
    bool m_displayInfo = true;
    bool m_displayWarning = true;
    bool m_displayError = true;
    QString m_message = "";
    QString m_level = "";
    QDateTime m_time;
};
