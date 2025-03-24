#pragma once

#include <common/INIReader.hpp>
#include <common/Singleton.hpp>
#include <condition_variable>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>

#include "mysql_connection.hpp"

class MySQLConnectionPool : public Singleton<MySQLConnectionPool> {
    friend class Singleton<MySQLConnectionPool>;

public:
    // 从连接池获取一个 MySQLConnection 实例（以 shared_ptr 管理，使用完后记得归还）
    std::shared_ptr<MySQLConnection> get() {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_cv.wait(lock, [this] { return !m_pool.empty(); });
        auto conn = m_pool.front();
        m_pool.pop();
        return conn;
    }

    // 将连接归还到连接池
    void release(std::shared_ptr<MySQLConnection> conn) {
        std::unique_lock<std::mutex> lock(m_mutex);
        m_pool.push(conn);
        lock.unlock();
        m_cv.notify_one();
    }

    ~MySQLConnectionPool() {
        while (!m_pool.empty()) {
            m_pool.pop();
        }
    }

private:
    MySQLConnectionPool() {
        // 连接池大小，可根据需要调整
        auto& reader = Singleton<INIReader>::getInstance();

        // 数据库连接参数
        auto host = reader["MySQL"]["Host"];
        auto user = reader["MySQL"]["User"];
        auto passwd = reader["MySQL"]["PassWd"];
        auto db = reader["MySQL"]["DBName"];
        auto port = std::stoi(reader["MySQL"]["Port"]);
        const int poolSize = 5;

        // std::string host = "127.0.0.1";
        // std::string user = "yh";
        // std::string passwd = "111111";
        // std::string db = "llfc";
        // unsigned int port = 3306;
        for (int i = 0; i < poolSize; ++i) {
            auto conn = std::make_shared<MySQLConnection>(host, user, passwd, db, port);
            if (conn->connect()) {
                m_pool.push(conn);
            } else {
                throw std::runtime_error{"Failed to create MySQL connection in pool"};
            }
        }
        std::cout << "create MySQL connection in pool, size: " << m_pool.size() << "\n";
    }

    std::queue<std::shared_ptr<MySQLConnection>> m_pool;
    std::mutex m_mutex;
    std::condition_variable m_cv;
};