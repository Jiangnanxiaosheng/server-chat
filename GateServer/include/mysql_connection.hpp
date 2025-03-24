#pragma once

#include <mysql/mysql.h>

#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string>

class MySQLConnection {
public:
    MySQLConnection(const std::string& host, const std::string& user, const std::string& passwd,
                    const std::string& db, unsigned int port = 3306)
        : m_host(host), m_user(user), m_passwd(passwd), m_db(db), m_port(port), m_conn(nullptr) {
        m_conn = mysql_init(nullptr);
        if (!m_conn) {
            std::cerr << "mysql_init failed\n";
        }
    }

    ~MySQLConnection() {
        if (m_conn) {
            mysql_close(m_conn);
        }
    }

    // 建立连接
    bool connect() {
        if (!mysql_real_connect(m_conn, m_host.c_str(), m_user.c_str(), m_passwd.c_str(),
                                m_db.c_str(), m_port, nullptr, 0)) {
            std::cerr << "mysql_real_connect failed: " << mysql_error(m_conn) << std::endl;
            return false;
        }
        // 设置字符集
        mysql_set_character_set(m_conn, "utf8");
        return true;
    }

    // 执行查询，返回 MYSQL_RES*（调用者使用后需调用 mysql_free_result 释放）
    MYSQL_RES* query(const std::string& sql) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (mysql_query(m_conn, sql.c_str())) {
            std::cerr << "Query failed: " << mysql_error(m_conn) << std::endl;
            return nullptr;
        }
        return mysql_store_result(m_conn);
    }

    // 执行更新/插入/删除，返回受影响的行数
    long execute(const std::string& sql) {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (mysql_query(m_conn, sql.c_str())) {
            std::cerr << "Execute failed: " << mysql_error(m_conn) << std::endl;
            return -1;
        }
        return mysql_affected_rows(m_conn);
    }

    // 转义字符串，防止SQL注入
    std::string escapeString(const std::string& input) {
        std::lock_guard<std::mutex> lock(m_mutex);
        char buffer[input.size() * 2 + 1];  // 预留空间
        mysql_real_escape_string(m_conn, buffer, input.c_str(), input.size());
        return std::string(buffer);
    }

    MYSQL* getMYSQL() { return m_conn; }

private:
    std::string m_host;
    std::string m_user;
    std::string m_passwd;
    std::string m_db;
    uint16_t m_port;
    MYSQL* m_conn;
    std::mutex m_mutex;
};