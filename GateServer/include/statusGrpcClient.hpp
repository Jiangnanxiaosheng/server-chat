#pragma once

#include <grpcpp/grpcpp.h>

#include <common/INIReader.hpp>
#include <common/common.hpp>
#include <memory>
#include <string>

#include "message.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

using message::GetChatServerRequest;
using message::GetChatServerResponse;
using message::LoginRequest;
using message::LoginResponse;
using message::StatusService;

class StatusGrpcClient : public Singleton<StatusGrpcClient> {
    friend class Singleton<StatusGrpcClient>;

public:
    // GateServer 获取 ChatServer 的 IP 和端口响应
    GetChatServerResponse GetChatServer(std::string& username) {
        GetChatServerRequest request;
        request.set_username(username);

        GetChatServerResponse reply;
        ClientContext context;
        Status status = m_stub->GetChatServer(&context, request, &reply);

        if (!status.ok()) {
            reply.set_error(ErrorCodes::ErrorRpcFailed);
            std::cout << "Grpc failed: " << status.error_message() << "\n";
        }

        return reply;
    }

    // 用户登录请求
    LoginResponse Login(std::string& username, std::string& token) {
        LoginRequest request;
        request.set_username(username);
        request.set_token(token);

        LoginResponse reply;
        ClientContext context;
        Status status = m_stub->Login(&context, request, &reply);

        if (!status.ok()) {
            reply.set_error(ErrorCodes::ErrorRpcFailed);
            std::cout << "Grpc failed: " << status.error_message() << "\n";
        }

        return reply;
    }

private:
    StatusGrpcClient() {
        auto& config = Singleton<INIReader>::getInstance();
        std::string host = config["StatusServer"]["Host"];
        std::string port = config["StatusServer"]["Port"];

        std::cout << "StatusServer host : " << host << " port: " << port << "\n";

        std::shared_ptr<Channel> channel =
            grpc::CreateChannel(host + ":" + port, grpc::InsecureChannelCredentials());
        m_stub = StatusService::NewStub(channel);
    }

    std::unique_ptr<StatusService::Stub> m_stub;
};