
#include <coro/coro.hpp>

using namespace coro;
using namespace coro::net;
using namespace coro::net::tcp;

int main() {
    auto tcp_echo_client = [](std::shared_ptr<IoScheduler> scheduler) -> Task<> {
        std::string ss =
            "dajiahao dajiahao dajiahao dajiahao dajiahao dajiahao dajiahao dajiahao dajiahao "
            "dajiahao 111 "
            "dajiahao dajiahao dajiahao dajiahao dajiahao dajiahao dajiahao dajiahao dajiahao "
            "dajiahao 222 "
            "dajiahao dajiahao dajiahao dajiahao dajiahao dajiahao dajiahao dajiahao dajiahao "
            "dajiahao 333 "
            "dajiahao dajiahao dajiahao dajiahao dajiahao dajiahao dajiahao dajiahao dajiahao "
            "dajiahao 444 "
            "dajiahao dajiahao dajiahao dajiahao dajiahao dajiahao dajiahao dajiahao dajiahao "
            "dajiahao 555 "
            "dajiahao dajiahao dajiahao dajiahao dajiahao dajiahao dajiahao dajiahao dajiahao "
            "dajiahao 666 "
            "dajiahao dajiahao dajiahao dajiahao dajiahao dajiahao dajiahao dajiahao dajiahao "
            "dajiahao 777 "
            "dajiahao dajiahao dajiahao dajiahao dajiahao dajiahao dajiahao dajiahao dajiahao "
            "dajiahao 888 "
            "dajiahao dajiahao dajiahao dajiahao dajiahao dajiahao dajiahao dajiahao dajiahao "
            "dajiahao 999 "
            "dajiahao dajiahao dajiahao dajiahao dajiahao dajiahao dajiahao dajiahao dajiahao "
            "dajiahao 111 "
            "dajiahao dajiahao dajiahao dajiahao dajiahao dajiahao dajiahao dajiahao dajiahao "
            "dajiahao 222 "
            "dajiahao dajiahao dajiahao dajiahao dajiahao dajiahao dajiahao dajiahao dajiahao "
            "dajiahao 333 "
            "dajiahao dajiahao dajiahao dajiahao dajiahao dajiahao dajiahao dajiahao dajiahao "
            "dajiahao 444 "
            "dajiahao dajiahao dajiahao dajiahao dajiahao dajiahao dajiahao dajiahao dajiahao "
            "dajiahao 555 "
            "dajiahao dajiahao dajiahao dajiahao dajiahao dajiahao dajiahao dajiahao dajiahao "
            "dajiahao 666 "
            "dajiahao dajiahao dajiahao dajiahao dajiahao dajiahao dajiahao dajiahao dajiahao "
            "dajiahao 777 "
            "dajiahao dajiahao dajiahao dajiahao dajiahao dajiahao dajiahao dajiahao dajiahao "
            "dajiahao 888 "
            "dajiahao dajiahao dajiahao dajiahao dajiahao dajiahao dajiahao dajiahao dajiahao "
            "dajiahao 999 "
            "dajiahao dajiahao dajiahao dajiahao dajiahao dajiahao dajiahao dajiahao dajiahao "
            "dajiahao 000 ";
        std::cout << "size: " << ss.size() << "\n";
        co_await scheduler->schedule();

        net::tcp::Client client{scheduler};

        auto cstatus = co_await client.connect();
        switch (cstatus) {
        case ConnectStatus::Connected: {
            std::cout << "客户端连接服务器成功\n";
            std::this_thread::sleep_for(std::chrono::seconds(2));
            co_await client.async_send_message(ss);
            std::cout << "客户端消息已发送\n";

        } break;
        case ConnectStatus::Error:
        case ConnectStatus::Timeout:
        default:
            std::cout << "连接失败\n";
            co_return;
        }

        co_await client.poll(PollOp::Read);
        auto optResponse = co_await client.async_read_message();
        if (!optResponse.has_value()) {
            std::cout << "服务器断开连接\n";
            co_return;
        }
        std::string response = optResponse.value();
        std::cout << "客户端收到回复: " << response << "\n";

        co_return;
    };

    auto scheduler =
        IoScheduler::make_shared(IoScheduler::Options{.execution_strategy = io_exec_thread_pool});
    sync_wait(tcp_echo_client(scheduler));
}
