#include <grpcpp/grpcpp.h>

#include <iostream>
#include <memory>
#include <string>

#include "hello.grpc.pb.h"

using grpc::Server;
using grpc::ServerBuilder;
using grpc::ServerContext;
using grpc::Status;

using hello::Greeter;
using hello::HelloRequest;
using hello::HelloResponse;

// 实现 Greeter 服务
class GreeterServiceImpl final : public Greeter::Service {
public:
    // 实现 SayHello RPC 方法
    Status SayHello(ServerContext* context, const HelloRequest* request,
                    HelloResponse* reply) override {
        std::string prefix("Hello ");
        reply->set_message(prefix + request->name());
        return Status::OK;
    }
};

void RunServer() {
    std::string server_address("0.0.0.0:50055");
    GreeterServiceImpl service;

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
