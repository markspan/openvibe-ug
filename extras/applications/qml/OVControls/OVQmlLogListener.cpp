/**
 * SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#include "OVQmlLogListener.h"
#include "OVLogController.h"

OVQmlLogListener::OVQmlLogListener()
{
    OVLogController::instance();
}

OVQmlLogListener::~OVQmlLogListener() 
{

}

bool OVQmlLogListener::isActive(const OpenViBE::Kernel::ELogLevel level) {
    // m_currentLogLevel set here because of logic in see CLogManager::logForEach

    switch(level) {
    case OpenViBE::Kernel::LogLevel_First:
        m_currentLogLevel = "debug";
        break;
    case OpenViBE::Kernel::LogLevel_None:
        m_currentLogLevel = "info";
        break;
    case OpenViBE::Kernel::LogLevel_Debug:
        m_currentLogLevel = "debug";
        break;
    case OpenViBE::Kernel::LogLevel_Benchmark:
        m_currentLogLevel = "info";
        break;
    case OpenViBE::Kernel::LogLevel_Trace:
        m_currentLogLevel = "debug";
        break;
    case OpenViBE::Kernel::LogLevel_Info:
        m_currentLogLevel = "info";
        break;
    case OpenViBE::Kernel::LogLevel_Warning:
        m_currentLogLevel = "warning";
        break;
    case OpenViBE::Kernel::LogLevel_ImportantWarning:
        m_currentLogLevel = "warning";
        break;
    case OpenViBE::Kernel::LogLevel_Error:
        m_currentLogLevel = "error";
        break;
    case OpenViBE::Kernel::LogLevel_Fatal:
        m_currentLogLevel = "error";
        break;
    case OpenViBE::Kernel::LogLevel_Last:
        m_currentLogLevel = "error";
        break;
    default:
        m_currentLogLevel = "";
        break;
    }

    return true;
}

bool OVQmlLogListener::activate(const OpenViBE::Kernel::ELogLevel level, const bool active) {
    // dont bother with this, always active
    return true;
}

bool OVQmlLogListener::activate(const OpenViBE::Kernel::ELogLevel startLogLevel, const OpenViBE::Kernel::ELogLevel endLogLevel, const bool active) {
    // dont bother with this, always active
    return true;
}

bool OVQmlLogListener::activate(const bool active) {
    return true;
}

void OVQmlLogListener::log(const OpenViBE::CTime value) {
    OVLogController::instance()->addLogItem(QString::fromStdString(value.str()), m_currentLogLevel.c_str());
}

#if defined __clang__
    void OVQmlLogListener::log(const size_t value) {
        OVLogController::instance()->addLogItem(QString::number(value), m_currentLogLevel.c_str()); 
    }
#endif
    void OVQmlLogListener::log(const uint64_t value) {
        OVLogController::instance()->addLogItem(QString::number(value), m_currentLogLevel.c_str()); 
    }

    void OVQmlLogListener::log(const uint32_t value) {
        OVLogController::instance()->addLogItem(QString::number(value), m_currentLogLevel.c_str()); 
    }

    void OVQmlLogListener::log(const int64_t value) {
        OVLogController::instance()->addLogItem(QString::number(value), m_currentLogLevel.c_str());  
    }
    void OVQmlLogListener::log(const int value) {
        OVLogController::instance()->addLogItem(QString::number(value), m_currentLogLevel.c_str()); 
    }
    void OVQmlLogListener::log(const double value) {
        OVLogController::instance()->addLogItem(QString::number(value), m_currentLogLevel.c_str()); 
    }
    void OVQmlLogListener::log(const bool value) {
        OVLogController::instance()->addLogItem(QString(value ? "true" : "false"), m_currentLogLevel.c_str()); 
    }
    void OVQmlLogListener::log(const OpenViBE::CIdentifier& value) {
        this->log(value.toString());
    }
    void OVQmlLogListener::log(const OpenViBE::CString& value) {
        OVLogController::instance()->addLogItem(QString(value.toASCIIString()), m_currentLogLevel.c_str());
    }
    void OVQmlLogListener::log(const std::string& value) {
        OVLogController::instance()->addLogItem(QString(value.c_str()), m_currentLogLevel.c_str());
    }
    void OVQmlLogListener::log(const char* value) {
        OVLogController::instance()->addLogItem(QString(value), m_currentLogLevel.c_str());
    }
    void OVQmlLogListener::log(const OpenViBE::Kernel::ELogLevel level) {
        this->isActive(level);
    }

    void OVQmlLogListener::log(const OpenViBE::Kernel::ELogColor color) {
        // dont bother with this
    }
