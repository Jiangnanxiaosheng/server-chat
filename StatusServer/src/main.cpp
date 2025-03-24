#include <grpcpp/grpcpp.h>

#include <common/common.hpp>
#include <iostream>
#include <map>
#include <memory>
#include <random>
#include <string>

#include "message.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using message::GetChatServerRequest;
using message::GetChatServerResponse;
using message::LoginRequest;
using message::LoginResponse;
using message::StatusService;

// 实现 Greeter 服务
class StatusServiceImpl final : public StatusService::Service {
public:
    // GateServer 获取 ChatServer 的 IP 和端口请求
    Status GetChatServer(ServerContext* context, const GetChatServerRequest* request,
                         GetChatServerResponse* reply) override {
        if (chat_servers.empty()) {
            return Status(grpc::StatusCode::UNAVAILABLE, "No ChatServer available");
        }

        // 找到连接数最少的 ChatServer
        auto min_it = std::min_element(
            chat_servers.begin(), chat_servers.end(), [](const auto& a, const auto& b) {
                return std::get<2>(a.second) < std::get<2>(b.second);  // 比较连接数
            });
        auto [ip, port, connection_count] = min_it->second;

        reply->set_ip(ip);
        reply->set_port(port);
        reply->set_error(ErrorCodes::Success);

        // 生成 token 并保存用户与 token 的键值对
        std::string token = generateToken();
        user_tokens[request->username()] = token;
        reply->set_token(token);
        std::get<2>(min_it->second)++;  // 分配后连接数加1

        std::cout << "Assigned ChatServer " << ip << ":" << port << " to user "
                  << request->username() << " with token " << token << std::endl;
        return Status::OK;
    }

private:
    // 存储 ChatServer 的信息，key 为 ip:port，value 为 (ip, port, connection_count)
    std::map<std::string, std::tuple<std::string, int, int>> chat_servers{
        {"127.0.0.1:8090", {"127.0.0.1", 8090, 3}},
        {"127.0.0.1:8091", {"127.0.0.2", 8091, 1}},
        {"127.0.0.1:8092", {"127.0.0.3", 8092, 2}},
        {"127.0.0.1:8088", {"127.0.0.1", 8088, 0}}};

    // 存储用户与 token 的键值对，key 为 username，value 为 token
    std::map<std::string, std::string> user_tokens;

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
};

void RunServer() {
    std::string server_address("0.0.0.0:50052");
    StatusServiceImpl service;

    // 构建服务器
    ServerBuilder builder;
    // 在指定地址上开启监听，使用不安全（明文）认证
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    // 注册服务实例
    builder.RegisterService(&service);
    // 启动服务器
    std::unique_ptr<Server> server(builder.BuildAndStart());
    std::cout << "Server listening on " << server_address << std::endl;

    // 等待服务器关闭（通常由外部信号终止）
    server->Wait();
}

int main(int argc, char** argv) {
    RunServer();
    return 0;
}