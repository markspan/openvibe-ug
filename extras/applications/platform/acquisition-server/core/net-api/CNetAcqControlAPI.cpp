#include "CNetAcqControlAPI.h"
#include <Acquisition.h>
#include <Api.h>

#include <openvibe/acquisition-messaging/ovCtrlCmdMsg.h>
#include <iostream>


std::unique_ptr<CNetAcqControlAPI> CNetAcqControlAPI::s_instance = nullptr;

bool readControlMsg(void* data, size_t dataSize, uint64_t remoteId)
{
    try 
    {
        // Assume data is a null-terminated JSON string
        // (Adjust based on your actual network protocol)
        const char* jsonString = reinterpret_cast<const char*>(data);

        // Parse JSON string
        nlohmann::json jsonData = nlohmann::json::parse(jsonString, jsonString + dataSize);

        // Deserialize into ControlMsg
        // @Todo: what happens when this fails ?
        ControlMsg ctrlMsg = jsonData.get<ControlMsg>();

        // Push to processing queue
        CNetAcqControlAPI::getInstance().pushRequest(remoteId, std::move(ctrlMsg));

        return true;
    }
    catch (const nlohmann::json::parse_error& e) {
        std::cerr << "JSON parse error: " << e.what() << std::endl;
        return false;
    }
    catch (const nlohmann::json::exception& e) {
        std::cerr << "JSON error: " << e.what() << std::endl;
        return false;
    }
    catch (const std::exception& e) {
        std::cerr << "Error deserializing control message: " << e.what() << std::endl;
        return false;
    }
}

std::pair<size_t, void*> writeControlMsgResponse(void* data, uint64_t clientId)
{
    try 
    {
        std::string jsonStr = nlohmann::json(*reinterpret_cast<ControlMsgResponse*>(data)).dump();

        // Allocate buffer for JSON string (will be freed by Network++ after sending)
        char* buffer = new char[jsonStr.size()];
        std::memcpy(buffer, jsonStr.data(), jsonStr.size());

        std::cout << "Sending response: " << jsonStr << std::endl;

        return {jsonStr.size(), buffer};
    }
    catch (const std::exception& e) {
        std::cerr << "Error serializing response: " << e.what() << std::endl;
        return {0, nullptr};
    }
}


CNetAcqControlAPI::CNetAcqControlAPI()
{
    m_netSrv.registerReadCallback<ControlMsg>(readControlMsg);
    m_netSrv.registerWriteCallback<ControlMsgResponse>(writeControlMsgResponse);
    m_bRun = true;
    m_thread = std::unique_ptr<std::thread>(new std::thread([this] { run(); }));
}

CNetAcqControlAPI& CNetAcqControlAPI::getInstance()
{
    if (!s_instance)
    {
        s_instance = std::unique_ptr<CNetAcqControlAPI>(new CNetAcqControlAPI());
    }
    return *s_instance;
}

CNetAcqControlAPI::~CNetAcqControlAPI()
{
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_bRun = false;
    }
    m_queueChanged.notify_all();
    m_thread->join();
}

void CNetAcqControlAPI::pushRequest(uint64_t id, ControlMsg req)
{
    // UG fork: under the lock run() takes (upstream pushes without it, from the network thread), and waking it
    {
        std::lock_guard<std::mutex> lock(m_queueMutex);
        m_requestQueue.push({ id, std::move(req) });
    }
    m_queueChanged.notify_one();
}

void CNetAcqControlAPI::run()
{
    // UG fork: waits for a request; upstream polls the queue without pause, which keeps a CPU core busy for as long
    // as the server runs, also when no client is connected
    while (true)
    {
        uint64_t remoteId;
        ControlMsg ctrlMsg;
        ControlMsgResponse ctrlMsgResponse;

        {
            std::unique_lock<std::mutex> lock(m_queueMutex);
            m_queueChanged.wait(lock, [this] { return !m_requestQueue.empty() || !m_bRun; });
            if (!m_bRun) { return; }

            auto request = std::move(m_requestQueue.front());
            remoteId = request.first;
            ctrlMsg = std::move(request.second);
            m_requestQueue.pop();
        }

        processControlMessage(remoteId, ctrlMsg, ctrlMsgResponse);
        m_netSrv.send<ControlMsgResponse>(&ctrlMsgResponse, (int)remoteId);
    }
}

void CNetAcqControlAPI::processControlMessage(uint64_t id, ControlMsg& msg, ControlMsgResponse& resp)
{
   switch (msg.command)
   {
       case CREATE_ACQUISITION:
           CmdCreateAcquisition(id, msg, resp); break;
       case GET_ACQ_LIST:
           CmdGetAcqList(resp); break;
       case GET_ACQ_SETTINGS:
           CmdGetAcqSettings(msg, resp); break;
       case GET_DRIVER_LIST:
           CmdGetDriverList(id, resp); break;
       case SET_DRIVER:
           CmdSetDriver(id, msg, resp); break;
       case GET_WRITER_LIST:
           CmdGetWriterList(id, resp); break;
       case SET_WRITER:
           CmdSetWriter(id, msg, resp); break;
       case START_ACQ:
           CmdStartAcquisition(id, msg, resp); break;
       case STOP_ACQ:
           CmdStopAcquisition(id, msg, resp); break;
        case REQUEST_ACQ_STREAM:
            CmdRequestAcqStream(id, msg, resp); break;
       case GET_GLOBAL_STATUS:
           CmdGetGlobatStatus(resp); break;
       default:
           break;// Handle unknown command
   }
}

void CNetAcqControlAPI::CmdCreateAcquisition(uint64_t id, ControlMsg& msg, ControlMsgResponse& resp)
{
    resp.command = CREATE_ACQUISITION;
    std::string acqName = std::get<CreateAcquisitionRequest>(msg.payload).acquisitionName;
    SuccessResponse response;
    response.success = Api::createAcquisition(id, acqName);
    resp.payload = response;
}

void CNetAcqControlAPI::CmdGetAcqList(ControlMsgResponse& resp)
{
    resp.command = GET_ACQ_LIST;

    std::vector<std::string> acquisitionNames = Api::getAcquisitionNames();
    AcquisitionListResponse response;
    response.acquisitionNames.assign(acquisitionNames.begin(), acquisitionNames.end());
    resp.payload = response;
}

void CNetAcqControlAPI::CmdGetAcqSettings(const ControlMsg& msg, ControlMsgResponse& resp)
{
    resp.command = GET_ACQ_SETTINGS;
    /*TODO when parameters are in place */
}

void CNetAcqControlAPI::CmdGetDriverList(uint64_t id, ControlMsgResponse& resp)
{
    resp.command = GET_DRIVER_LIST;

    std::vector<std::string> driverNames = Api::getDriverNames();
    DriverListResponse response;
    response.driverNames.assign(driverNames.begin(), driverNames.end());
    resp.payload = response;
}

void CNetAcqControlAPI::CmdSetDriver(uint64_t id, const ControlMsg& msg, ControlMsgResponse& resp)
{
    resp.command = SET_DRIVER;
    SetDriverRequest request = std::get<SetDriverRequest>(msg.payload);
    
    SuccessResponse response;
    response.success = Api::SetAcquisitionDriver(id, request.acquisitionName, request.driverName);
    resp.payload = response;
}

void CNetAcqControlAPI::CmdGetWriterList(uint64_t id, ControlMsgResponse& resp)
{
    resp.command = GET_WRITER_LIST;

    std::vector<std::string> writerNames = Api::getWriterNames();
    WriterListResponse response;
    response.writerNames.assign(writerNames.begin(), writerNames.end());
    resp.payload = response;
}

void CNetAcqControlAPI::CmdSetWriter(uint64_t id, const ControlMsg& msg, ControlMsgResponse& resp)
{
    resp.command = SET_WRITER;
    SetWriterRequest request = std::get<SetWriterRequest>(msg.payload);
    
    SuccessResponse response;
    response.success = Api::SetAcquisitionWriter(id, request.acquisitionName, request.writerName);
    resp.payload = response;
}

void CNetAcqControlAPI::CmdStartAcquisition(uint64_t id, const ControlMsg& msg, ControlMsgResponse& resp)
{
    resp.command = START_ACQ;
    std::string acqName = std::get<StartAcquisitionRequest>(msg.payload).acquisitionName;

    SuccessResponse response;
    response.success = Api::start(id, acqName);

    resp.payload = response;
}

void CNetAcqControlAPI::CmdStopAcquisition(uint64_t id, const ControlMsg& msg, ControlMsgResponse& resp)
{
    resp.command = STOP_ACQ;
    std::string acqName = std::get<StopAcquisitionRequest>(msg.payload).acquisitionName;

    SuccessResponse response;
    response.success = Api::stop(id, acqName);

    resp.payload = response;
}

void CNetAcqControlAPI::CmdRequestAcqStream(uint64_t id, const ControlMsg& msg, ControlMsgResponse& resp)
{
    resp.command = REQUEST_ACQ_STREAM;
    std::string acqName = std::get<RequestAcqStreamRequest>(msg.payload).acquisitionName;
    bool enable = std::get<RequestAcqStreamRequest>(msg.payload).enable;

    SuccessResponse response;
    if (enable) {
        response.success = Api::addClient(id, acqName);
    } else {
        response.success = Api::removeClient(id, acqName);
    }

    resp.payload = response;
}

void CNetAcqControlAPI::CmdGetGlobatStatus(ControlMsgResponse& resp)
{
    //auto currentAcq = m_acqManager.getCurrentAcquisition();
}
