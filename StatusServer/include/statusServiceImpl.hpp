#pragma once

#include <chrono>
#include <mutex>
#include <thread>

#include "message.grpc.pb.h"
#include "redis_connection_pool.hpp"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
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

class StatusServiceImpl final : public StatusService::Service {
public:
    using clock = std::chrono::steady_clock;

    struct ChatServerInfo {
        ChatServerInfo(std::string ip, int port, int connection_count, clock::time_point timePoint)
            : ip(ip), port(port), connection_count(connection_count), last_heartbeat(timePoint) {}

        std::string ip;
        uint16_t port;
        int connection_count;
        clock::time_point last_heartbeat;
    };

    StatusServiceImpl() {
        std::thread([this]() { CheckHeartbeats(); }).detach();
    }

    Status RegisterChatServer(ServerContext* context, const RegisterChatServerRequest* request,
                              RegisterChatServerResponse* reply) override {
        std::string key = request->ip() + ":" + std::to_string(request->port());
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_chat_servers.find(key) != m_chat_servers.end()) {
            reply->set_error(ErrorCodes::ChatServerAlreadyRegistered);  // 已经注册
            return Status::OK;
        } else {
            m_chat_servers.emplace(key, ChatServerInfo(request->ip(), request->port(), 0,
                                                       std::chrono::steady_clock::now()));
            std::cout << request->ip() << ":" << request->port() << " 成功注册到 StatusServer\n";
        };
        auto redis = RedisConnectionPool::getInstance().get();
        redis->set("chatServer_" + key, "0");
        reply->set_error(ErrorCodes::Success);
        return Status::OK;
    }

    Status UnregisterChatServer(ServerContext* context, const UnregisterChatServerRequest* request,
                                UnregisterChatServerResponse* reply) override {
        std::string key = request->ip() + ":" + std::to_string(request->port());
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_chat_servers.erase(key)) {
            reply->set_error(ErrorCodes::Success);
        } else {
            reply->set_error(ErrorCodes::ChatServerNotFound);
        }
        return Status::OK;
    }

    // GateServer 获取 ChatServer 的 IP 和端口请求
    Status GetChatServer(ServerContext* context, const GetChatServerRequest* request,
                         GetChatServerResponse* reply) override {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (m_chat_servers.empty()) {
            reply->set_error(ErrorCodes::NoAvailableChatServers);
            return Status::OK;
        }
        for (const auto& pair : m_chat_servers) {
            std::cout << "Server Key: " << pair.first << std::endl;
            std::cout << "IP: " << pair.second.ip << std::endl;
            std::cout << "Port: " << pair.second.port << std::endl;
        }

        // 找到连接数最少的 ChatServer
        auto min_it = std::min_element(
            m_chat_servers.begin(), m_chat_servers.end(), [](const auto& a, const auto& b) {
                return a.second.connection_count < b.second.connection_count;  // 比较连接数
            });

        reply->set_ip(min_it->second.ip);
        reply->set_port(min_it->second.port);
        // 生成 token 并保存用户与 token 的键值对
        std::string token = generateToken();
        reply->set_token(token);
        auto redis = RedisConnectionPool::getInstance().get();
        redis->set("token_" + request->username(), token);

        reply->set_error(ErrorCodes::Success);

        std::cout << "Assigned ChatServer " << min_it->second.ip << ":" << min_it->second.port
                  << " to user " << request->username() << " with token " << token << std::endl;

        return Status::OK;
    }

    Status UpdateConnectionCount(ServerContext* context,
                                 const UpdateConnectionCountRequest* request,
                                 UpdateConnectionCountResponse* reply) override {
        // std::cout << "调用了 UpdateConnectionCount\n";
        std::string key = request->ip() + ":" + std::to_string(request->port());
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_chat_servers.find(key);
        auto redis = RedisConnectionPool::getInstance().get();
        if (it != m_chat_servers.end() || redis->get("chatServer_" + key).has_value()) {
            if (it != m_chat_servers.end()) [[likely]] {
                it->second.connection_count = request->connection_count();
            } else {
                it->second.connection_count = std::stoi(*redis->get("chatServer_" + key));
            }
            it->second.last_heartbeat = std::chrono::steady_clock::now();
            reply->set_error(ErrorCodes::Success);
        } else {
            reply->set_error(ErrorCodes::ChatServerNotFound);
        }
        return Status::OK;
    }

private:
    std::map<std::string, ChatServerInfo> m_chat_servers;  // key: "ip:port"
    const int m_heartbeat_timeout = 10;                    // 心跳超时时间 (秒)
    std::mutex m_mutex;                                    // 保护 map 的并发访问

    // 生成随机的 32 位 token
    std::string generateToken() {
        std::string chars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
        std::string token;
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<> dis(0, chars.size() - 1);
        for (int i = 0; i < 32; ++i) {
            token += chars[dis(gen)];
        }
        return token;
    }

    // 定时检查心跳
    void CheckHeartbeats() {
        while (true) {
            std::this_thread::sleep_for(std::chrono::seconds(m_heartbeat_timeout / 2));
            auto now = std::chrono::steady_clock::now();
            std::lock_guard<std::mutex> lock(m_mutex);
            for (auto it = m_chat_servers.begin(); it != m_chat_servers.end();) {
                int elapsed = std::chrono::duration_cast<std::chrono::seconds>(
                                  now - it->second.last_heartbeat)
                                  .count();
                if (elapsed > m_heartbeat_timeout) {
                    std::cout << "移除断开连接的TcpChatServer: " << it->first << std::endl;
                    it = m_chat_servers.erase(it);
                } else {
                    ++it;
                }
            }
        }
    }
};