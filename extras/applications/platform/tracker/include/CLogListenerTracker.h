#pragma once

#include "base.hpp"
#include "CLogListenerDesigner.hpp"

#include <vector>
#include <map>

namespace OpenViBE {
namespace Tracker {

/**
 * \class CLogListenerTracker
 * \brief Log listener wrapper for CLogListenerDesigner that can be called from multiple threads
 * \author J. T. Lindgren
 *
 */
class CLogListenerTracker final : public Kernel::ILogListener
{
public:
    CLogListenerTracker(const Kernel::IKernelContext& ctx, GtkBuilder* builder) : m_impl(ctx, builder) { }

    // @note Only call from gtk thread
    void clearMessages()
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        // std::cout << "Intercepted\n"; 
        m_impl.ClearMessages();
    }

    bool isActive(const Kernel::ELogLevel level) override
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        return m_impl.isActive(level);
    }

    bool activate(const Kernel::ELogLevel logLevel, const bool active) override
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        return m_impl.activate(logLevel, active);
    }

    bool activate(const Kernel::ELogLevel startLogLevel, const Kernel::ELogLevel endLogLevel, const bool active) override
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        return m_impl.activate(startLogLevel, endLogLevel, active);
    }

    bool activate(const bool active) override
    {
        std::unique_lock<std::mutex> lock(m_mutex);
        return m_impl.activate(active);
    }

    // This function encapsulates CLogListenerDesigner so that its gtk calls will be run from g_idle_add().
    template <class T>
    void logWrapper(T param)
    {
        auto ptr = new std::pair<CLogListenerTracker*, T>(this, param);

        auto fun = [](gpointer data)
        {
            auto ptr = static_cast<std::pair<CLogListenerTracker*, T>*>(data);

            // LogListenerDesigner may not be thread safe, so we lock here for safety as activate() and isActive() do not go through the g_idle_add().
            std::unique_lock<std::mutex> lock(ptr->first->getMutex());

            ptr->first->getImpl().log(ptr->second);
            delete ptr;

            return gboolean(FALSE);
        };

        g_idle_add(fun, ptr);
    }

    // Specialization for const char*. It frees memory. We don't call it 
    // template<> logWrapper() to avoid gcc complaining.
    void logWrapperCharPtr(const char* param)
    {
        const auto ptr = new std::pair<CLogListenerTracker*, const char*>(this, param);

        const auto fun = [](gpointer data)
        {
            auto ptr = static_cast<std::pair<CLogListenerTracker*, const char*>*>(data);

            // LogListenerDesigner may not be thread safe, so we lock here for safety as activate() and isActive() do not go through the g_idle_add().
            std::unique_lock<std::mutex> lock(ptr->first->getMutex());

            ptr->first->getImpl().log(ptr->second);
            delete[] ptr->second;
            delete ptr;

            return gboolean(FALSE);
        };

        g_idle_add(fun, ptr);
    }

    void log(const CTime value) override { logWrapper(value); }
    void log(const uint64_t value) override { logWrapper(value); }
    void log(const uint32_t value) override { logWrapper(value); }
    void log(const int64_t value) override { logWrapper(value); }
    void log(const int value) override { logWrapper(value); }
    void log(const double value) override { logWrapper(value); }
    void log(const bool value) override { logWrapper(value); }
    void log(const Kernel::ELogLevel eLogLevel) override { logWrapper(eLogLevel); }
    void log(const Kernel::ELogColor eLogColor) override { logWrapper(eLogColor); }

    // With these calls we need to make copies, as by the time the actual calls are run in gtk_idle, the originals may be gone
    void log(const CIdentifier& value) override
    {
        const CIdentifier nonRef = value;
        logWrapper(nonRef);
    }

    void log(const CString& value) override
    {
        const CString nonRef = value;
        logWrapper(nonRef);
    }

    void log(const std::string& value) override
    {
        const std::string nonRef = value;
        logWrapper(nonRef);
    }

    void log(const char* value) override
    {
        // This one is tricksy, we cannot simply use CString as we'd lose color, but with char* we need to free the memory. Use a specialization.
#if defined(TARGET_OS_Windows)
        const char* valueCopy = _strdup(value);
#else
            const char* valueCopy = strdup(value);
#endif
        logWrapperCharPtr(valueCopy);
    }

    // Callbacks need access to these
    Designer::CLogListenerDesigner& getImpl() { return m_impl; }
    std::mutex& getMutex() { return m_mutex; }

    // getClassIdentifier()
    _IsDerivedFromClass_Final_(Kernel::ILogListener, CIdentifier::undefined())

protected:
    Designer::CLogListenerDesigner m_impl;
    std::mutex m_mutex;
};
}  // namespace Tracker
}  // namespace OpenViBE
