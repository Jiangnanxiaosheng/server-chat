#include <grpcpp/grpcpp.h>

#include <common/common.hpp>
#include <iostream>
#include <map>
#include <memory>
#include <random>
#include <string>

#include "statusServiceImpl.hpp"

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