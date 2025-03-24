#pragma once

#include <coro/coro.hpp>
#include <string>

using namespace coro;
namespace http = coro::net::tcp::http;

class MyHttpServer {
public:
    MyHttpServer(std::string ip = "0.0.0.0", uint16_t port = 8080);

    void init();

    void start();

private:
    http::HttpServer m_http_server;
};
