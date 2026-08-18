#pragma once

/**
* @file ovCtrlCmdMsg.h
* @author Florent LERAY
* @date 2025
* @version 1.0
* @copyright Copyright (c) 2025 INRIA. All rights reserved.
* @license This project is released under LGPLv3 license.
* @brief Header file defining the ovCtrlCmdMsg structure for control commands sent to Acquisition Server.
* @note This structure is used for communication between OPENVIBE and the Acquisition Server.
*/

#include <list>
#include <string>
#include <vector>
#include <variant>

#include <nlohmann/json.hpp>


#include "../parameters/parameterTraits.h"
#include "samples.h"


/////////////////////////////////// Control API ////////////////////////////////
enum ControlCommand
{
    CREATE_ACQUISITION,
    GET_ACQ_LIST,
    GET_ACQ_SETTINGS,
    GET_DRIVER_LIST,
    SET_DRIVER,
    GET_WRITER_LIST,
    SET_WRITER,
    START_ACQ,
    STOP_ACQ,
    REQUEST_ACQ_STREAM,
    GET_GLOBAL_STATUS
};

NLOHMANN_JSON_SERIALIZE_ENUM(ControlCommand,
{
    {CREATE_ACQUISITION,      "CREATE_ACQUISITION"},
    {GET_ACQ_LIST,            "GET_ACQ_LIST"},
    {GET_ACQ_SETTINGS,        "GET_ACQ_SETTINGS"},
    {GET_DRIVER_LIST,         "GET_DRIVER_LIST"},
    {SET_DRIVER,              "SET_DRIVER"},
    {GET_WRITER_LIST,         "GET_WRITER_LIST"},
    {SET_WRITER,              "SET_WRITER"},
    {START_ACQ,               "START_ACQ"},
    {STOP_ACQ,                "STOP_ACQ"},
    {REQUEST_ACQ_STREAM,      "REQUEST_ACQ_STREAM"},
    {GET_GLOBAL_STATUS,       "GET_GLOBAL_STATUS"}
})

/////////////////////////////////// Control API Request ////////////////////////////////
struct CreateAcquisitionRequest
{
    std::string acquisitionName;

    friend void to_json(nlohmann::json& j, const CreateAcquisitionRequest& req) {
        j = nlohmann::json{
            {"acquisitionName", req.acquisitionName}
        };
    }

    friend void from_json(const nlohmann::json& j, CreateAcquisitionRequest& req)
    {
        j.at("acquisitionName").get_to(req.acquisitionName);
    }
};

struct SetDriverRequest
{
    std::string acquisitionName;
    std::string driverName;

    friend void to_json(nlohmann::json& j, const SetDriverRequest& req) {
        j = nlohmann::json{
            {"acquisitionName", req.acquisitionName},
            {"driverName", req.driverName}
        };
    }

    friend void from_json(const nlohmann::json& j, SetDriverRequest& req)
    {
        j.at("acquisitionName").get_to(req.acquisitionName);
        j.at("driverName").get_to(req.driverName);
    }
};

struct SetWriterRequest
{
    std::string acquisitionName;
    std::string writerName;

    friend void to_json(nlohmann::json& j, const SetWriterRequest& req) {
        j = nlohmann::json{
            {"acquisitionName", req.acquisitionName},
            {"writerName", req.writerName}
        };
    }

    friend void from_json(const nlohmann::json& j, SetWriterRequest& req)
    {
        j.at("acquisitionName").get_to(req.acquisitionName);
        j.at("writerName").get_to(req.writerName);   
    }
};

struct StartAcquisitionRequest
{
    std::string acquisitionName;

    friend void to_json(nlohmann::json& j, const StartAcquisitionRequest& req) {
        j = nlohmann::json{
            {"acquisitionName", req.acquisitionName}
        };
    }

    friend void from_json(const nlohmann::json& j, StartAcquisitionRequest& req)
    {
        j.at("acquisitionName").get_to(req.acquisitionName);
    }
};

struct StopAcquisitionRequest
{
    std::string acquisitionName;

    friend void to_json(nlohmann::json& j, const StopAcquisitionRequest& req) {
        j = nlohmann::json{
            {"acquisitionName", req.acquisitionName}
        };
    }

    friend void from_json(const nlohmann::json& j, StopAcquisitionRequest& req)
    {
        j.at("acquisitionName").get_to(req.acquisitionName);
    }
};

struct RequestAcqStreamRequest
{
    std::string acquisitionName;
    bool enable;

    friend void to_json(nlohmann::json& j, const RequestAcqStreamRequest& req) {
        j = nlohmann::json{
            {"acquisitionName", req.acquisitionName},
            {"enable", req.enable}
        };
    }

    friend void from_json(const nlohmann::json& j, RequestAcqStreamRequest& req)
    {
        j.at("acquisitionName").get_to(req.acquisitionName);
        j.at("enable").get_to(req.enable);
    }
};

struct ControlMsg
{
    ControlCommand command;
    std::variant<
        std::monostate,
        CreateAcquisitionRequest,
        SetDriverRequest,
        SetWriterRequest,
        StartAcquisitionRequest,
        StopAcquisitionRequest,
        RequestAcqStreamRequest
        //updateAcquisitionParameters
    > payload;

    friend void to_json(nlohmann::json& j, const ControlMsg& msg)
    {
        j["command"] = msg.command;

        std::visit([&j](const auto& p) {
            using T = std::decay_t<decltype(p)>;

            if constexpr (std::is_same_v<T, std::monostate>) {
                j["payload"] = nullptr;
            } else {
                j["payload"] = p;
            }
        }, msg.payload);
    }

    friend void from_json(const nlohmann::json& j, ControlMsg& msg)
    {
        j.at("command").get_to(msg.command);

        // Determine payload type based on command
        if (!j.contains("payload") || j["payload"].is_null()) {
            msg.payload = std::monostate{};
        } else {
            switch (msg.command) {
                case CREATE_ACQUISITION:
                    msg.payload = j["payload"].get<CreateAcquisitionRequest>();
                    break;
                case SET_DRIVER:
                    msg.payload = j["payload"].get<SetDriverRequest>();
                    break;
                case SET_WRITER:
                    msg.payload = j["payload"].get<SetWriterRequest>();
                    break;
                case START_ACQ:
                    msg.payload = j["payload"].get<StartAcquisitionRequest>();
                    break;
                case STOP_ACQ:
                    msg.payload = j["payload"].get<StopAcquisitionRequest>();
                    break;
                case REQUEST_ACQ_STREAM:
                    msg.payload = j["payload"].get<RequestAcqStreamRequest>();
                    break;
                default:
                    msg.payload = std::monostate{};
                    break;
            }
        }
    }
};

///////////////////////////////////// Control API Response ////////////////////////////////
struct AcquisitionListResponse
{
    std::list<std::string> acquisitionNames;

    friend void to_json(nlohmann::json& j, const AcquisitionListResponse& resp)
    {
        j = nlohmann::json{
            {"acquisitionNames", resp.acquisitionNames}
        };
    }

    friend void from_json(const nlohmann::json& j, AcquisitionListResponse& resp)
    {
        j.at("acquisitionNames").get_to(resp.acquisitionNames);
    }
};

struct DriverListResponse
{
    std::list<std::string> driverNames;

    friend void to_json(nlohmann::json& j, const DriverListResponse& resp)
    {
        j = nlohmann::json{
            {"driverNames", resp.driverNames}
        };
    }

    friend void from_json(const nlohmann::json& j, DriverListResponse& resp)
    {
        j.at("driverNames").get_to(resp.driverNames);
    }
};

struct WriterListResponse
{
    std::list<std::string> writerNames;

    friend void to_json(nlohmann::json& j, const WriterListResponse& resp)
    {
        j = nlohmann::json{
            {"writerNames", resp.writerNames}
        };
    }

    friend void from_json(const nlohmann::json& j, WriterListResponse& resp)
    {
        j.at("writerNames").get_to(resp.writerNames);
    }
};

using acqIsRunningResponse = struct AcqIsRunningResponse
{
    bool isRunning;

    friend void to_json(nlohmann::json& j, const AcqIsRunningResponse& resp)
    {
        j = nlohmann::json{
            {"isRunning", resp.isRunning}
        };
    }

    friend void from_json(const nlohmann::json& j, AcqIsRunningResponse& resp)
    {
        j.at("isRunning").get_to(resp.isRunning);
    }
};
using globalStatusResponse = struct GlobalStatusResponse
{
    std::string statusInfo;

    friend void to_json(nlohmann::json& j, const GlobalStatusResponse& resp)
    {
        j = nlohmann::json{
            {"statusInfo", resp.statusInfo}
        };
    }

    friend void from_json(const nlohmann::json& j, GlobalStatusResponse& resp)
    {
        j.at("statusInfo").get_to(resp.statusInfo);
    }
};

using sucessResponse = struct SuccessResponse
{
    bool success;

    friend void to_json(nlohmann::json& j, const SuccessResponse& resp)
    {
        j = nlohmann::json{
            {"success", resp.success}
        };
    }

    friend void from_json(const nlohmann::json& j, SuccessResponse& resp)
    {
        j.at("success").get_to(resp.success);
    }
};

struct ControlMsgResponse
{
    ControlCommand command;
    std::variant<
        std::monostate,
        AcquisitionListResponse,
        DriverListResponse,
        WriterListResponse,
        AcqIsRunningResponse,
        GlobalStatusResponse,
        SuccessResponse
    > payload;

    friend void to_json(nlohmann::json& j, const ControlMsgResponse& resp)
    {
        j["command"] = resp.command;

        std::visit([&j](const auto& p) {
            using T = std::decay_t<decltype(p)>;

            if constexpr (std::is_same_v<T, std::monostate>) {
                j["payload"] = nullptr;
            } else {
                j["payload"] = p;
            }
        }, resp.payload);
    }

    friend void from_json(const nlohmann::json& j, ControlMsgResponse& resp)
    {
        j.at("command").get_to(resp.command);

        // Determine payload type based on command
        if (!j.contains("payload") || j["payload"].is_null()) {
            resp.payload = std::monostate{};
        } else {
            switch (resp.command) {
                case GET_ACQ_LIST:
                    resp.payload = j["payload"].get<AcquisitionListResponse>();
                    break;
                case GET_DRIVER_LIST:
                    resp.payload = j["payload"].get<DriverListResponse>();
                    break;
                case GET_WRITER_LIST:
                    resp.payload = j["payload"].get<WriterListResponse>();
                    break;
                default:
                    resp.payload = j["payload"].get<SuccessResponse>();
                    break;
            }
        }
    }
};
