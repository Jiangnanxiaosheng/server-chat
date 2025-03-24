#pragma once

#include <grpcpp/grpcpp.h>

#include <common/common.hpp>
#include <coro/coro.hpp>
#include <string>

#include "message.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

using message::StatusService;

using message::RegisterChatServerRequest;
using message::RegisterChatServerResponse;

using message::UnregisterChatServerRequest;
using message::UnregisterChatServerResponse;

using message::GetChatServerRequest;
using message::GetChatServerResponse;

using message::UpdateConnectionCountRequest;
using message::UpdateConnectionCountResponse;

class MyTcpServer : public Singleton<MyTcpServer> {
    friend class Singleton<MyTcpServer>;

public:
    coro::Task<> reportConnectionCount() {
        while (true) {
            {
                std::lock_guard<std::mutex> lock(m_connection_count_mutex);
                UpdateConnectionCountRequest request;
                request.set_ip(m_ip);
                request.set_port(m_port);
                request.set_connection_count(m_connection_count);

                UpdateConnectionCountResponse reply;
                ClientContext context;
                Status status = m_stub->UpdateConnectionCount(&context, request, &reply);

                if (status.ok() && reply.error() == ErrorCodes::Success) {
                    std::cout << "连接数更新: " << m_connection_count << "\n";
                } else {
                    std::cout << "连接数更新失败\n";
                    // 重新创建通道和 stub
                    std::shared_ptr<Channel> channel =
                        grpc::CreateChannel("127.0.0.1:50052", grpc::InsecureChannelCredentials());
                    m_stub = StatusService::NewStub(channel);
                }
            }
            co_await m_scheduler->schedule_after(std::chrono::seconds(5));
        }
    }

    MyTcpServer(const std::string& ip, uint16_t port, const std::string& status_server_addr,
                std::shared_ptr<coro::IoScheduler> scheduler)
        : m_ip(ip), m_port(port), m_scheduler(scheduler) {
        m_server = std::make_shared<coro::net::tcp::Server>(
            m_scheduler, coro::net::tcp::Server::LocalEndPoint{
                             .address = coro::net::IpAddress::from_string(ip), .port = port});

        std::shared_ptr<Channel> channel =
            grpc::CreateChannel(status_server_addr, grpc::InsecureChannelCredentials());
        m_stub = StatusService::NewStub(channel);
        Register();
        m_scheduler->spawn(reportConnectionCount());
    }

    ~MyTcpServer() { Unregister(); }

    int getConnectionCount() const {
        std::lock_guard<std::mutex> lock(m_connection_count_mutex);
        return m_connection_count;
    }

    void incrementConnectionCount() {
        std::lock_guard<std::mutex> lock(m_connection_count_mutex);
        m_connection_count++;
    }

    void decrementConnectionCount() {
        std::lock_guard<std::mutex> lock(m_connection_count_mutex);
        if (m_connection_count > 0)
            m_connection_count--;
    }

    std::string getIp() const { return m_ip; }

    uint16_t getPort() const { return m_port; }

    std::shared_ptr<coro::net::tcp::Server> getServer() const { return m_server; }
    std::shared_ptr<coro::IoScheduler> getScheduler() const { return m_scheduler; }

private:
    std::string m_ip;
    uint16_t m_port;
    std::shared_ptr<coro::net::tcp::Server> m_server;
    std::shared_ptr<coro::IoScheduler> m_scheduler;
    std::shared_ptr<StatusService::Stub> m_stub;
    int m_connection_count = 0;
    mutable std::mutex m_connection_count_mutex;

    void Register() {
        RegisterChatServerRequest request;
        request.set_ip(m_ip);
        request.set_port(m_port);

        RegisterChatServerResponse reply;
        ClientContext context;
        Status status = m_stub->RegisterChatServer(&context, request, &reply);

        if (status.ok() && reply.error() == ErrorCodes::Success) {
            std::cout << m_ip << ":" << m_port << " 成功注册到 StatusServer\n";
        } else {
            std::cout << m_ip << ":" << m_port << " 注册到 StatusServer 失败\n";
        }
    }

    void Unregister() {
        UnregisterChatServerRequest request;
        request.set_ip(m_ip);
        request.set_port(m_port);

        UnregisterChatServerResponse response;
        ClientContext context;
        Status status = m_stub->UnregisterChatServer(&context, request, &response);

        if (status.ok() && response.error() == ErrorCodes::Success) {
            std::cout << m_ip << ":" << m_port << " 成功从 StatusServer 注销\n";
        } else {
            std::cout << m_ip << ":" << m_port << " 从 StatusServer 注销失败\n";
        }
    }
};