/**
 * SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/ 
#include "OVLogController.h"
#include "OVLogControllerItem.h"

OVLogController *OVLogController::instance() 
{
    if (!s_instance) {
        s_instance = new OVLogController();
    }

    return s_instance;
}

QQmlListProperty<OVLogControllerItem> OVLogController::items()
{
    m_items.clear();
    if(m_displayDebug) {
        m_items.append(m_itemsDebug);
    }
    if(m_displayInfo) {
        m_items.append(m_itemsInfo);
    }
    if(m_displayWarning) {
        m_items.append(m_itemsWarning);
    }
    if(m_displayError) {
        m_items.append(m_itemsError);
    }

    std::sort(m_items.begin(), m_items.end(), [](const OVLogControllerItem *a, const OVLogControllerItem *b) {
        return a->time() < b->time();
    });

    return QQmlListProperty<OVLogControllerItem>(this, &m_items);
}

int OVLogController::infoCount() const
{
    return m_infoCount;
}

int OVLogController::warningCount() const
{
    return m_warningCount;
}

int OVLogController::errorCount() const
{
    return m_errorCount;
}

int OVLogController::debugCount() const
{
    return m_debugCount;
}

bool OVLogController::displayDebug() const
{
    return m_displayDebug;
}

void OVLogController::setDisplayDebug(bool displayDebug)
{
    if (m_displayDebug == displayDebug) {
        return;
    }

    m_displayDebug = displayDebug;
    emit displayDebugChanged();
    emit itemsChanged();
}

bool OVLogController::displayInfo() const
{
    return m_displayInfo;
}

void OVLogController::setDisplayInfo(bool displayInfo)
{
    if (m_displayInfo == displayInfo) {
        return;
    }

    m_displayInfo = displayInfo;
    emit displayInfoChanged();
    emit itemsChanged();
}

bool OVLogController::displayWarning() const
{
    return m_displayWarning;
}

void OVLogController::setDisplayWarning(bool displayWarning)
{
    if (m_displayWarning == displayWarning) {
        return;
    }

    m_displayWarning = displayWarning;
    emit displayWarningChanged();
    emit itemsChanged();
}

bool OVLogController::displayError() const
{
    return m_displayError;
}

void OVLogController::setDisplayError(bool displayError)
{
    if (m_displayError == displayError) {
        return;
    }

    m_displayError = displayError;
    emit displayErrorChanged();
    emit itemsChanged();    
}

Q_INVOKABLE void OVLogController::clear()
{
    m_items.clear(); // no delete as the items are shared with the other lists

    for(auto item : m_itemsDebug) {
        delete item;
    }
    m_itemsDebug.clear();

    for(auto item : m_itemsInfo) {
        delete item;
    }
    m_itemsInfo.clear();

    for(auto item : m_itemsWarning) {
        delete item;
    }
    m_itemsWarning.clear();

    for(auto item : m_itemsError) {
        delete item;
    }
    m_itemsError.clear();

    m_infoCount = 0;
    m_warningCount = 0;
    m_errorCount = 0;
    m_debugCount = 0;

    m_displayDebug = false;
    m_displayInfo = true;
    m_displayWarning = true;
    m_displayError = true;

    emit itemsChanged();
    emit infoCountChanged();
    emit warningCountChanged();
    emit errorCountChanged();
    emit debugCountChanged();
}


void OVLogController::addLogItem(const QString &message, const QString &level) {

    // display level changed, somewhere the \n was forgot: have to flush the last string
    QDateTime dt = QDateTime::currentDateTime();

    // New string
    if (! m_time.isValid()) {
        m_time = dt;
    }
    // New string
    if (m_level == "") {
        m_level = level;
    }
    auto delta = dt.toMSecsSinceEpoch()-m_time.toMSecsSinceEpoch();
    if ((m_level != level) || (delta > 10)){ // we accept a delay of 10 micro seconds
        m_message += "\n";
        addLogItemAndFlush();
    }
    
    m_message += message;
    m_level = level;
    m_time = dt;
    if (! message.endsWith("\n")) {
        return;
    }
    addLogItemAndFlush();
}

void OVLogController::addLogItemAndFlush() {
    
    QStringList levels = {"info", "warning", "error", "fatal", "debug"};
    QString inLevel = m_level.toLower();
    if(!levels.contains(inLevel)) {
        qWarning() << Q_FUNC_INFO << " - Unknown log level: " << m_level << "setting to debug";
        inLevel = "debug";
    }
    OVLogControllerItem *item = new OVLogControllerItem(m_message.trimmed(), inLevel);
    item->moveToThread(QCoreApplication::instance()->thread());

    // message send, reset
    if (inLevel == "info") {
        m_infoCount++;
        m_itemsInfo.append(item);
        emit infoCountChanged();
    } else if (inLevel == "warning") {
        m_warningCount++;
        m_itemsWarning.append(item);
        emit newWarning(m_message);
        emit warningCountChanged();
    } else if (inLevel == "error" || inLevel == "fatal") {
        m_errorCount++;
        m_itemsError.append(item);
        emit newError(m_message);
        emit errorCountChanged();
    } else {
        m_debugCount++;
        m_itemsDebug.append(item);
        emit debugCountChanged();
    }
    m_message = "";
    m_level = "";
    m_time = QDateTime();
    emit itemsChanged();
}

OVLogController::OVLogController() {
    //qmlRegisterType<OVLogController>("OVAS", 1, 0, "OVLogController");
    m_time = QDateTime();
}

OVLogController::~OVLogController() {
    this->clear();
};

OVLogController *OVLogController::s_instance = nullptr;
