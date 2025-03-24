#include <coro/coro.hpp>

using namespace coro;
using namespace coro::net;
using namespace coro::net::tcp;

// 应用层发送接口：传入请求类型和负载
// 这里请求类型为 uint16_t（2字节），负载为 std::string
coro::Task<> app_send_message(Client& client, uint16_t reqType, const std::string& payload) {
    constexpr size_t appHeaderSize = 2;
    // 构造应用层消息内容：先写入2字节请求类型（大端编码），后接负载
    std::vector<char> appMsg;
    appMsg.resize(appHeaderSize + payload.size());
    appMsg[0] = (reqType >> 8) & 0xFF;
    appMsg[1] = reqType & 0xFF;
    std::copy(payload.begin(), payload.end(), appMsg.begin() + appHeaderSize);
    // 调用底层 async_send_message，底层会自动在 appMsg 前加上【2字节长度】
    co_await client.async_send_message(std::string(appMsg.begin(), appMsg.end()));
    co_return;
}

// 应用层接收接口：返回解析后的请求类型和负载
// 底层 async_read_message 返回的内容已经去除了【2字节长度】，即为 [请求类型 + 负载]
coro::Task<std::optional<std::pair<uint16_t, std::string>>> app_read_message(Client& client) {
    auto optMsg = co_await client.async_read_message();
    if (!optMsg.has_value()) {
        co_return std::nullopt;
    }
    std::string fullMsg = optMsg.value();
    constexpr size_t appHeaderSize = 2;
    if (fullMsg.size() < appHeaderSize) {
        co_return std::nullopt;  // 数据格式错误
    }
    // 解析前2字节为请求类型（大端）
    uint16_t reqType = (static_cast<uint16_t>(static_cast<unsigned char>(fullMsg[0])) << 8) |
                       static_cast<uint16_t>(static_cast<unsigned char>(fullMsg[1]));
    std::string payload = fullMsg.substr(appHeaderSize);
    co_return std::make_pair(reqType, payload);
}

int main() {
    auto tcp_echo_server = [](std::shared_ptr<IoScheduler> scheduler) -> Task<> {
        // 连接任务：接收完整消息并回显
        auto connection_task = [](Client client) -> coro::Task<> {
            while (true) {
                auto optResult = co_await app_read_message(client);
                if (!optResult.has_value()) {
                    std::cout << "服务端：连接关闭或消息格式错误" << std::endl;
                    break;
                }

                auto [reqType, payload] = optResult.value();
                std::cout << "服务端接收到请求类型: " << reqType << ", payload: " << payload
                          << "\n";

                // 根据 reqType 分发处理，如：
                switch (reqType) {
                case 1001:
                    // 处理登录请求
                    {
                        std::cout << "处理登录请求" << std::endl;
                        co_await app_send_message(client, 10086, "你好");
                    }
                    break;
                // 其他请求类型……
                default:
                    std::cout << "未知请求类型" << std::endl;
                }
            }
            co_return;
        };

        co_await scheduler->schedule();

        Server server{scheduler, {.address = IpAddress::from_string("0.0.0.0"), .port = 8088}};
        while (true) {
            auto pstatus = co_await server.poll();
            switch (pstatus) {
            case PollStatus::Event: {
                auto client = server.accept();
                if (client.socket().is_valid()) {
                    std::cout << "连接成功\n";
                    scheduler->spawn(connection_task(std::move(client)));
                }
            } break;
            case PollStatus::Error:
            case PollStatus::Closed:
            case PollStatus::Timeout:
            default:
                co_return;
            }
        }
        co_return;
    };

    std::vector<Task<>> workers{};
    for (int i = 0; i < 16; ++i) {
        auto scheduler = IoScheduler::make_shared(
            IoScheduler::Options{.execution_strategy = io_exec_thread_pool});
        workers.push_back(tcp_echo_server(scheduler));
    }

    sync_wait(when_all(std::move(workers)));

    sleep(100000000);
}