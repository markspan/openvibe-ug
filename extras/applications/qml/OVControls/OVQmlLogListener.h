/**
 * SPDX-FileCopyrightText: Inria <tristan.cabel@inria.fr>
 * SPDX-License-Identifier: AGPL-3.0-only
*/

#pragma once

#include <openvibe/CIdentifier.hpp>
#include <openvibe/CTime.hpp>
#include <openvibe/kernel/log/ovILogListener.h>

class OVQmlLogListener : public OpenViBE::Kernel::ILogListener
{
public:
    OVQmlLogListener();
    virtual ~OVQmlLogListener();
    OpenViBE::CIdentifier getClassIdentifier() const override { return OpenViBE::CIdentifier::undefined(); }

    bool isActive(const OpenViBE::Kernel::ELogLevel level) override;
    bool activate(const OpenViBE::Kernel::ELogLevel level, const bool active) override;
    bool activate(const OpenViBE::Kernel::ELogLevel startLogLevel, const OpenViBE::Kernel::ELogLevel endLogLevel, const bool active) override;
    bool activate(const bool active) override;

    void log(const OpenViBE::CTime value) override;
#if defined __clang__
    void log(const size_t value) override;
#endif
    void log(const uint64_t value) override;
    void log(const uint32_t value) override;
    void log(const int64_t value) override;
    void log(const int value) override;
    void log(const double value) override;
    void log(const bool value) override;
    void log(const OpenViBE::CIdentifier& value) override;
    void log(const OpenViBE::CString& value) override;
    void log(const std::string& value) override;
    void log(const char* value) override;
    void log(const OpenViBE::Kernel::ELogLevel level) override;
    void log(const OpenViBE::Kernel::ELogColor color) override;

private:
    std::string m_currentLogLevel;
};