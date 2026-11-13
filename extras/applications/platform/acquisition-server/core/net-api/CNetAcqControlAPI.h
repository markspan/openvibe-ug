#pragma once

#include <list>
#include <queue>
#include <string>
#include <utility>
#include <vector>

#include <condition_variable>  // UG fork
#include <mutex>
#include <thread>

#include "AcquisitionManager.h" ///< Include the header file for the AcquisitionManager class.
#include <NetService.h> ///< Include the header file for the NetService class.

#include <openvibe/acquisition-messaging/samples.h>
#include <openvibe/acquisition-messaging/ovCtrlCmdMsg.h>

class Acquisition; ///< Forward declaration of the Acquisition class.

/**
 * @brief Class API for remote controle the acquisition server application.
 *
 * The `CNetAcqControlAPI` class provides methods to process control messages
 * related to application, such as retrieving acquisition lists, settings,
 * and managing drivers and writers.
 */
class CNetAcqControlAPI
{
public:
    static CNetAcqControlAPI& getInstance();
	~CNetAcqControlAPI();

    void processControlMessage(uint64_t id, ControlMsg& msg, ControlMsgResponse& resp);
    void notifyUpdate();

    void pushRequest(uint64_t id, ControlMsg msg);
    void run();

private:
    CNetAcqControlAPI();

    void CmdCreateAcquisition(uint64_t id, ControlMsg& msg, ControlMsgResponse& resp);
    void CmdGetAcqSettings(const ControlMsg& msg, ControlMsgResponse& resp);
    void CmdGetDriverList(uint64_t id, ControlMsgResponse& resp);
    void CmdSetDriver(uint64_t id, const ControlMsg& msg, ControlMsgResponse& resp);
    void CmdGetWriterList(uint64_t id, ControlMsgResponse& resp);
    void CmdSetWriter(uint64_t id, const ControlMsg& msg, ControlMsgResponse& resp);
    void CmdGetAcqList(ControlMsgResponse& resp);
    void CmdStartAcquisition(uint64_t id, const ControlMsg& msg, ControlMsgResponse& resp);
    void CmdStopAcquisition(uint64_t id, const ControlMsg& msg, ControlMsgResponse& resp);
    void CmdRequestAcqStream(uint64_t id, const ControlMsg& msg, ControlMsgResponse& resp);
    void CmdGetGlobatStatus(ControlMsgResponse& resp);

private:
    static std::unique_ptr<CNetAcqControlAPI> s_instance;

    bool m_bRun;
    std::unique_ptr<std::thread> m_thread;

    std::mutex m_queueMutex;
    std::condition_variable m_queueChanged;  ///< UG fork: wakes run() for a request, or to stop
    std::queue < std::pair<uint64_t, ControlMsg> > m_requestQueue;

    Network::CServer & m_netSrv = OpenViBE::Services::NetService::instance();

};
