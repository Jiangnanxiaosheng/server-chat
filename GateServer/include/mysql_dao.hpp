#pragma once

#include "mysql_connection_pool.hpp"

class MySQLDao {
public:
    MySQLDao() = default;
    ~MySQLDao() = default;

    int registerUser(const std::string& account, const std::string& email,
                     const std::string& password) {
        auto& pool = Singleton<MySQLConnectionPool>::getInstance();
        auto conn = pool.get();
        if (!conn) {
            return -1;
        }

        // 对输入参数做转义，防止SQL注入
        std::string escAccount = conn->escapeString(account);
        std::string escEmail = conn->escapeString(email);
        std::string escPwd = conn->escapeString(password);

        // 构建 SQL 调用存储过程, OUT 参数为 @result
        std::string query =
            "CALL register('" + escAccount + "', '" + escEmail + "', '" + escPwd + "', @result);";
        if (mysql_query(conn->getMYSQL(), query.c_str()) != 0) {
            std::cout << "registerUser: 存储过程执行失败: " << mysql_error(conn->getMYSQL())
                      << std::endl;
            pool.release(conn);
            return -1;
        }

        // 存储过程执行后，再查询 @result
        if (mysql_query(conn->getMYSQL(), "SELECT @result")) {
            pool.release(conn);
            return -1;
        }

        // 获取 @result 的值
        MYSQL_RES* res = mysql_store_result(conn->getMYSQL());
        if (!res) {
            pool.release(conn);
            return -1;
        }

        MYSQL_ROW row = mysql_fetch_row(res);
        int result = -1;

        if (row && row[0]) {
            result = std::stoi(row[0]);
        }
        mysql_free_result(res);

        pool.release(conn);
        return result;
    }

    bool isUserExists(const std::string& account) {
        auto conn = Singleton<MySQLConnectionPool>::getInstance().get();
        std::string sql = "SELECT COUNT(*) as count FROM users WHERE account = '" +
                          conn->escapeString(account) + "'";
        MYSQL_RES* res = conn->query(sql);
        if (!res) {
            Singleton<MySQLConnectionPool>::getInstance().release(conn);
            return false;
        }
        bool exists = false;
        MYSQL_ROW row = mysql_fetch_row(res);
        if (row && std::stoi(row[0]) > 0) {
            exists = true;
        }
        mysql_free_result(res);
        Singleton<MySQLConnectionPool>::getInstance().release(conn);
        return exists;
    }

    bool updatePassword(const std::string& account, const std::string& newPassword) {
        auto& pool = Singleton<MySQLConnectionPool>::getInstance();
        auto conn = pool.get();
        if (!conn) {
            return false;
        }
        std::string escAccount = conn->escapeString(account);
        std::string escPwd = conn->escapeString(newPassword);
        // 构造 UPDATE 语句
        std::string sql =
            "UPDATE users SET password = '" + escPwd + "' WHERE account = '" + escAccount + "'";
        if (mysql_query(conn->getMYSQL(), sql.c_str())) {
            pool.release(conn);
            return false;
        }
        // 可以根据 mysql_affected_rows 来判断是否成功修改（至少1行受影响）
        long affected = mysql_affected_rows(conn->getMYSQL());
        pool.release(conn);
        return affected > 0;
    }

    bool verifyPassword(const std::string& account, const std::string& password) {
        // 先检查账号是否存在
        if (!isUserExists(account)) {
            return false;
        }

        auto& pool = Singleton<MySQLConnectionPool>::getInstance();
        auto conn = pool.get();
        if (!conn) {
            return false;
        }

        std::string query =
            "SELECT password FROM users WHERE account = '" + conn->escapeString(account) + "';";
        if (mysql_query(conn->getMYSQL(), query.c_str()) != 0) {
            std::cout << "verifyPassword: 查询失败: " << mysql_error(conn->getMYSQL()) << std::endl;
            pool.release(conn);
            return false;
        }

        MYSQL_RES* res = mysql_store_result(conn->getMYSQL());
        bool match = false;
        if (res) {
            MYSQL_ROW row = mysql_fetch_row(res);
            if (row && row[0]) {
                std::string storedPassword(row[0]);  // 数据库存储的密码
                if (storedPassword == password) {    // 直接对比明文密码（推荐使用加密存储）
                    match = true;
                }
            }
            mysql_free_result(res);
        }

        pool.release(conn);
        return match;
    }
};