#pragma once

#include "ovas_base.h"
#include "ovasIDriver.h"
#include "ovasIHeader.h"
#include "ovasCDriftCorrection.h"

#include <socket/IConnectionServer.h>

#include <deque>
#include <functional>
#include <list>
#include <mutex>
#include <thread>
#include <vector>

namespace OpenViBE {
namespace AcquisitionServer {
class CConnectionServerHandlerThread;
class CConnectionClientHandlerThread;
class IAcquisitionServerPlugin;

typedef struct
{
    uint64_t connectionTime;                                        // Time the client connected
    uint64_t stimulationTimeOffset;                                 // Time offset wrt acquisition start
    uint64_t nSampleToSkip;                                         // How many samples to skip wrt current buffer start. n.b. not a constant.
    CConnectionClientHandlerThread* connectionClientHandlerThread;  // Ptr to the class object that is executed by the client connection handler thread
    std::thread* connectionClientHandlerStdThread;                  // The actual thread handle
    bool isChannelUnitsSent;
    bool isChannelLocalisationSent;
} connection_info_t;

enum class ENaNReplacementPolicy { LastCorrectValue=0, Zero, Disabled };

std::string toString(const ENaNReplacementPolicy policy);

// Concurrency handling
class DoubleLock
{
    // Implements
    //   lock(mutex1);
    //   lock(mutex2);
    //   unlock(mutex1);
    // mutex2 lock is released when the object goes out of scope
    //
    // @details The two mutex 'pattern' is used to avoid thread starving which can happen e.g.
    // on Linux if just a single mutex is used; apparently the main loop just takes the
    // mutex repeatedly without the gui thread sitting on the mutex being unlocked.
    // n.b. potentially calls for redesign
public:
    DoubleLock(std::mutex* m1, std::mutex* m2) : lock2(*m2, std::defer_lock)
    {
        std::lock_guard<std::mutex> lock1(*m1);
        lock2.lock();
    }

private:
    std::unique_lock<std::mutex> lock2;
};

class CDriverContext;

class CAcquisitionServer final : public IDriverCallback
{
public:
    explicit CAcquisitionServer(const Kernel::IKernelContext& ctx);
    ~CAcquisitionServer() override;

    IDriverContext& getDriverContext() const;
    size_t getClientCount() const { return m_connections.size(); }
    double getImpedance(const size_t index) { return (index < m_impedances.size()) ? m_impedances[index] : OVAS_Impedance_Unknown; }

    bool loop();

    bool connect(IDriver& driver, const uint32_t nSamplingPerSentBlock, const uint32_t port);
    bool start();
    bool stop();
    bool disconnect();

    // Driver samples information callback
    void setSamples(const float* samples) override;
    void setSamples(const float* samples, const size_t count) override;
    void setStimulationSet(const CStimulationSet& stimSet) override;

    // Driver context callback
    bool isConnected() const { return m_isInitialized; }
    bool isStarted() const { return m_isStarted; }
    bool updateImpedance(const size_t index, const double impedance);

    //Driver set/access
    IDriver* getDriver() const { return m_Driver; }
    void setDriver(IDriver* driver) { m_Driver = driver; }

    // General parameters configurable from the GUI
    ENaNReplacementPolicy getNaNReplacementPolicy() const { return m_eNaNReplacementPolicy; }
    uint64_t getOversamplingFactor() const { return m_overSamplingFactor; }
    bool isImpedanceCheckRequested() const { return m_Driver ? m_Driver->getHeader()->isImpedanceCheckRequested() : false; }
    bool isChannelSelectionRequested() const { return m_isChannelSelectionRequested; }
    void setNaNReplacementPolicy(const ENaNReplacementPolicy policy) { m_eNaNReplacementPolicy = policy; }
    bool setOversamplingFactor(const uint64_t oversamplingFactor);
    void setImpedanceCheckRequest(const bool active) { m_isImpedanceCheckRequested = active; }
    void setChannelSelectionRequest(const bool active) { m_isChannelSelectionRequested = active; }
    void setDriftCallback(const std::function<void()>& callback) { m_DriftCorrection.setCallback(callback); }

    std::vector<IAcquisitionServerPlugin*> getPlugins() const { return m_Plugins; }

    bool acceptNewConnection(Socket::IConnection* connection);

    //---------- Variables ----------
    // See class DoubleLock
    std::mutex m_ProtectionMutex;
    std::mutex m_ExecutionMutex;

    std::deque<std::vector<float>> m_PendingBuffers;
    std::vector<float> m_SwapBuffers;

    size_t m_nSample   = 0;
    size_t m_startTime = 0;

    CDriftCorrection m_DriftCorrection;
    CStimulationSet m_PendingStimSet;

    std::vector<IAcquisitionServerPlugin*> m_Plugins;

protected:
    static bool requestClientThreadQuit(CConnectionClientHandlerThread* th);

    //---------- Variables ----------
    std::mutex m_oPendingConnectionProtectionMutex;
    std::mutex m_oPendingConnectionExecutionMutex;

    std::thread* m_connectionServerHandlerStdThread = nullptr;

    const Kernel::IKernelContext& m_kernelCtx;
    CDriverContext* m_driverCtx = nullptr;
    IDriver* m_Driver           = nullptr;

    Kernel::IAlgorithmProxy* m_encoder = nullptr;
    Kernel::TParameterHandler<uint64_t> ip_subjectID;
    Kernel::TParameterHandler<uint64_t> ip_subjectAge;
    Kernel::TParameterHandler<uint64_t> ip_subjectGender;
    Kernel::TParameterHandler<CMatrix*> ip_matrix;
    Kernel::TParameterHandler<CMatrix*> ip_channelUnits;
    Kernel::TParameterHandler<uint64_t> ip_sampling;
    Kernel::TParameterHandler<CStimulationSet*> ip_stimSet;
    Kernel::TParameterHandler<uint64_t> ip_bufferDuration;
    Kernel::TParameterHandler<CMemoryBuffer*> op_buffer;

    Kernel::TParameterHandler<bool> ip_encodeChannelLocalisationData;
    Kernel::TParameterHandler<bool> ip_encodeChannelUnitData;

    std::list<std::pair<Socket::IConnection*, connection_info_t>> m_connections;
    std::list<std::pair<Socket::IConnection*, connection_info_t>> m_pendingConnections;
    std::vector<float> m_overSamplingSwapBuffers;
    std::vector<double> m_impedances;
    std::vector<size_t> m_selectedChannels;
    std::vector<CString> m_selectedChannelNames;
    Socket::IConnectionServer* m_connectionServer = nullptr;

    ENaNReplacementPolicy m_eNaNReplacementPolicy = ENaNReplacementPolicy::LastCorrectValue;
    bool m_replacementInProgress                  = false;

    bool m_isInitialized               = false;
    bool m_isStarted                   = false;
    bool m_isImpedanceCheckRequested   = false;
    bool m_isChannelSelectionRequested = false;
    bool m_gotData                     = false;
    size_t m_overSamplingFactor        = 0;
    size_t m_nChannel                  = 0;
    size_t m_sampling                  = 0;
    uint32_t m_nSamplePerSentBlock     = 0;
    size_t m_nLastSample               = 0;
    size_t m_lastDeliveryTime          = 0;

    uint64_t m_lastEstimatedDrift      = 0;
    uint64_t m_nbNewSamplesSinceLastDriftEstimation = 0;

    //CDriftCorrection m_DriftCorrection;

    size_t m_nJitterEstimationForDrift   = 0;
    size_t m_driverTimeoutDuration       = 0;           // ms after which the driver is considered having time-outed
    int64_t m_startedDriverSleepDuration = 0;           // ms, <0 == spin, 0 == yield thread, >0 sleep. Used when driver does not return samples.
    size_t m_stoppedDriverSleepDuration  = 0;           // ms to sleep when driver is not running

    uint8_t* m_sampleBuffer = nullptr;
    //CStimulationSet m_PendingStimSet;
};
}  // namespace AcquisitionServer
}  // namespace OpenViBE
