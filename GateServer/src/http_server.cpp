#include "http_server.hpp"

#include <common/json.hpp>

#include "mysql_manager.hpp"
#include "redis_connection_pool.hpp"
#include "statusGrpcClient.hpp"
#include "verifyGrpcClient.hpp"
using nlohmann::json;

MyHttpServer::MyHttpServer(std::string ip, uint16_t port)
    : m_http_server(http::HttpServer(IoScheduler::make_shared(),
                                     {.address = net::IpAddress::from_string(ip), .port = port})) {
    init();
}

void MyHttpServer::start() { coro::sync_wait(m_http_server.start()); }

void MyHttpServer::init() {
    // 注册路由
    m_http_server.Get("/", [](http::Request& req, http::Response& resp) -> Task<> {
        resp.body = "Hello from coro HTTP server!";
        co_return;
    });

    m_http_server.Get("/api", [](http::Request& req, http::Response& resp) -> Task<> {
        resp.headers["Content-Type"] = "application/json";
        resp.body = R"({"status": "ok"})";
        co_return;
    });

    m_http_server.Post("/get_verifycode", [](http::Request& req, http::Response& resp) -> Task<> {
        try {
            json j = json::parse(req.body);
            std::string email = j["email"];
            std::cout << "解析出email: " << email << "\n";

            json response_json;

            resp.headers["Content-Type"] = "application/json";

            GetVerifyRsp rsp = Singleton<VerifyGrpcClient>::getInstance().GetVerifyCode(email);
            response_json["error"] = rsp.error();

            resp.body = response_json.dump();

        } catch (const json::exception& e) {
            resp.status_code = 400;
            resp.body = R"({"code":400, "message": "无效的json格式"})";
        }

        co_return;
    });

    m_http_server.Post("/register", [](http::Request& req, http::Response& resp) -> Task<> {
        json j;
        try {
            j = json::parse(req.body);

        } catch (const json::exception& e) {
            resp.status_code = 400;
            resp.body = R"({"code":400, "message": "无效的json格式"})";
        }

        std::string account = j["account"];
        std::string password = j["password"];
        std::string email = j["email"];
        std::string verifycode = j["verifycode"];

        std::cout << "解析出account: " << account << "\n";
        std::cout << "解析出email: " << email << "\n";

        json response_json;
        resp.headers["Content-Type"] = "application/json";

        // 查询 redis 是否缓存了验证码
        auto redis = Singleton<RedisConnectionPool>::getInstance().get();
        auto val = redis->get("code_" + email);
        if (val) {
            std::cout << "验证码为: " << *val << std::endl;
            if (*val != verifycode) {
                std::cout << "验证码不匹配\n";
                response_json["error"] = ErrorCodes::ErrorVerifyCode;
                resp.body = response_json.dump();
                co_return;
            }
        } else {
            std::cout << " get varify code expired" << std::endl;
            response_json["error"] = ErrorCodes::ErrorVerifyExpired;
            resp.body = response_json.dump();
            co_return;
        }

        int uid = Singleton<MySQLManager>::getInstance().registerUser(account, email, password);
        if (uid == -1) {
            std::cout << "MySQL 产生异常\n";
            response_json["error"] = ErrorCodes::UnknownError;
        } else if (uid == -2) {
            std::cout << "user is exist\n";
            response_json["error"] = ErrorCodes::UserExist;
        } else if (uid == -3) {
            std::cout << "eamil  is exist\n";
            response_json["error"] = ErrorCodes::EmailExist;
        } else if (uid > 0) [[likely]] {
            response_json["error"] = ErrorCodes::Success;
        }

        resp.body = response_json.dump();

        co_return;
    });

    m_http_server.Post("/login", [](http::Request& req, http::Response& resp) -> Task<> {
        json j;
        try {
            j = json::parse(req.body);
        } catch (const json::exception& e) {
            resp.status_code = 400;
            resp.body = R"({"code":400, "message": "无效的json格式"})";
        }

        std::string account = j["account"];
        std::string password = j["password"];

        std::cout << "解析出account: " << account << "\n";
        std::cout << "解析出password: " << password << "\n";

        json response_json;
        resp.headers["Content-Type"] = "application/json";

        bool ok = Singleton<MySQLManager>::getInstance().verifyPassword(account, password);
        if (ok) {
            std::cout << "账户密码正确\n";
            response_json["error"] = ErrorCodes::Success;
        } else {
            response_json["error"] = ErrorCodes::ErrorAccountOrPwd;
            resp.body = response_json.dump();
            co_return;
        }

        auto reply = Singleton<StatusGrpcClient>::getInstance().GetChatServer(account);
        if (reply.error()) {
            std::cout << "get chat server failed, error is: " << reply.error() << "\n";
            response_json["error"] = ErrorCodes::ErrorRpcFailed;
            resp.body = response_json.dump();
            co_return;
        }

        response_json["error"] = ErrorCodes::Success;
        response_json["ip"] = reply.ip();
        response_json["port"] = reply.port();
        response_json["token"] = reply.token();
        resp.body = response_json.dump();
        co_return;
    });
}
