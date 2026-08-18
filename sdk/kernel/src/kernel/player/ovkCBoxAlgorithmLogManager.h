#pragma once

#include "../ovkTKernelObject.h"

namespace OpenViBE {
namespace Kernel {

class CSimulatedBox;

class CBoxAlgorithmLogManager final : public ILogManager
{
public:
    CBoxAlgorithmLogManager(const IPlayerContext& playerContext, ILogManager& logManager, CSimulatedBox& simulatedBox)
        : m_playerCtx(playerContext), m_logManager(logManager), m_simulatedBox(simulatedBox) {}

    bool isActive(const ELogLevel level) override { return m_logManager.isActive(level); }
    bool activate(const ELogLevel level, const bool active) override { return m_logManager.activate(level, active); }

    bool activate(const ELogLevel startLevel, const ELogLevel endLevel, const bool active) override
    {
        return m_logManager.activate(startLevel, endLevel, active);
    }

    bool activate(const bool active) override { return m_logManager.activate(active); }

    bool addListener(ILogListener* listener) override { return m_logManager.addListener(listener); }
    bool removeListener(ILogListener* listener) override { return m_logManager.removeListener(listener); }

    void log(const CTime value) override { m_logManager.log(value); }
#if defined __clang__
    void log(const size_t value) override { m_logManager.log(value); }
#endif
    void log(const uint64_t value) override { m_logManager.log(value); }
    void log(const uint32_t value) override { m_logManager.log(value); }
    void log(const int64_t value) override { m_logManager.log(value); }
    void log(const int value) override { m_logManager.log(value); }
    void log(const double value) override { m_logManager.log(value); }
    void log(const bool value) override { m_logManager.log(value); }
    void log(const CString& value) override { m_logManager.log(value); }
    void log(const std::string& value) override { m_logManager.log(value); }
    void log(const char* value) override { m_logManager.log(value); }
    void log(const CIdentifier& value) override { m_logManager.log(value); }
    void log(const ELogColor value) override { m_logManager.log(value); }
    void log(const ELogLevel logLevel) override;

    CIdentifier getClassIdentifier() const override { return CIdentifier(); }

private:
    const IPlayerContext& m_playerCtx;
    ILogManager& m_logManager;
    CSimulatedBox& m_simulatedBox;
};

}  // namespace Kernel
}  // namespace OpenViBE
