#include <common/INIReader.hpp>
#include <common/common.hpp>
#include <common/json.hpp>
#include <coro/coro.hpp>
#include <filesystem>
#include <iostream>
#include <stdexcept>

#include "http_server.hpp"

using namespace coro;

using nlohmann::json;

int main() {
    try {
        std::string config_path = std::filesystem::absolute("../config/config.ini").string();
        INIReader& reader = Singleton<INIReader>::getInstance();
        if (!reader.load(config_path)) {
            std::cerr << "配置文件加载失败\n";
        } else {
            std::cerr << "配置文件加载成功\n";
            reader.dump();
        }

    } catch (std::exception& e) {
        std::cout << e.what() << "\n";
    }

    // auto& pool = MySQLConnectionPool::getInstance();
    // MySQLDao dao;
    // if (!dao.isUserExists("test_user7")) {
    //     std::cout << "注册前，用户不存在\n";
    // } else {
    //     std::cout << "注册前，用户存在\n";
    // }
    // // 注册用户
    // int userId = dao.registerUser("test_user7", "test7@example.com", "securepassword");
    // if (userId > 0) {
    //     std::cout << "用户注册成功，ID: " << userId << std::endl;
    // } else if (userId == 0) {
    //     std::cout << "账号或邮箱已存在" << std::endl;
    // } else {
    //     std::cout << "注册失败" << std::endl;
    // }
    // std::cout << "userId: " << userId << "\n";

    // if (!dao.isUserExists("test_user7")) {
    //     std::cout << "注册后，用户不存在\n";
    // } else {
    //     std::cout << "注册后，用户存在\n";
    // }

    // // 修改密码
    // if (dao.updatePassword("test_user7", "newpassword")) {
    //     std::cout << "密码修改成功" << std::endl;
    // } else {
    //     std::cout << "密码修改失败" << std::endl;
    // }

    MyHttpServer server;
    server.start();  // 阻塞操作
    return 0;
}