#include "verifyGrpcClient.hpp"

#include <common/INIReader.hpp>

VerifyGrpcClient::VerifyGrpcClient() {
    auto& config = Singleton<INIReader>::getInstance();
    std::string host = config["VerifyServer"]["Host"];
    std::string port = config["VerifyServer"]["Port"];

    std::cout << "VerifyServer host : " << host << " port: " << port << "\n";

    std::shared_ptr<Channel> channel =
        grpc::CreateChannel(host + ":" + port, grpc::InsecureChannelCredentials());
    m_stub = VerifyService::NewStub(channel);
}

GetVerifyRsp VerifyGrpcClient::GetVerifyCode(const std::string& email) {
    GetVerifyReq request;
    request.set_email(email);

    GetVerifyRsp reply;
    ClientContext context;
    Status status = m_stub->GetVerifyCode(&context, request, &reply);

    if (status.ok()) {
    } else {
        reply.set_error(ErrorCodes::ErrorRpcFailed);
        std::cout << "Grpc failed: " << status.error_message() << "\n";
    }

    return reply;
}
