#pragma once

#include <grpcpp/grpcpp.h>

#include <common/Singleton.hpp>
#include <common/common.hpp>
#include <memory>
#include <string>

#include "message.grpc.pb.h"

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;

using message::GetVerifyReq;
using message::GetVerifyRsp;
using message::VerifyService;

class VerifyGrpcClient : public Singleton<VerifyGrpcClient> {
    friend class Singleton<VerifyGrpcClient>;

public:
    GetVerifyRsp GetVerifyCode(const std::string& email);

private:
    VerifyGrpcClient();

    std::unique_ptr<VerifyService::Stub> m_stub;
};