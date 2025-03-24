#include <grpcpp/grpcpp.h>

#include <iostream>
#include <memory>
#include <string>

#include "hello.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

using hello::Greeter;
using hello::HelloRequest;
using hello::HelloResponse;

class GreeterClient {
public:
    // 构造函数，通过传入的 Channel 创建 Stub
    GreeterClient(std::shared_ptr<Channel> channel) : stub_(Greeter::NewStub(channel)) {}

    // 发起 SayHello RPC 调用
    std::string SayHello(const std::string& user) {
        // 组装请求消息
        HelloRequest request;
        request.set_name(user);

        // 用于接收服务器返回的响应
        HelloResponse reply;

        // 用于配置 RPC 调用（如超时、认证等，目前使用默认设置）
        ClientContext context;

        // 进行远程调用
        Status status = stub_->SayHello(&context, request, &reply);

        // 返回响应或错误信息
        if (status.ok()) {
            return reply.message();
        } else {
            std::cout << "RPC failed: " << status.error_message() << std::endl;
            return "RPC failed";
        }
    }

private:
    // gRPC 自动生成的客户端 Stub
    std::unique_ptr<Greeter::Stub> stub_;
};

int main(int argc, char** argv) {
    // 指定服务端地址
    std::string target_str = "localhost:50055";
    GreeterClient greeter(grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));

    std::string user("world");
    std::string reply = greeter.SayHello(user);
    std::string reply2 = greeter.SayHello("Jack (twice)");
    std::cout << "Greeter received: " << reply << std::endl;
    std::cout << "Greeter received: " << reply2 << std::endl;

    return 0;
}