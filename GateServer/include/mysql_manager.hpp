#include <common/Singleton.hpp>
#include <memory>
#include <mutex>

#include "mysql_dao.hpp"

class MySQLManager : public Singleton<MySQLManager> {
    friend class Singleton<MySQLManager>;

public:
    int registerUser(const std::string& account, const std::string& email,
                     const std::string& password) {
        return m_dao.registerUser(account, email, password);
    }

    bool isUserExists(const std::string& account) { return m_dao.isUserExists(account); }

    bool updatePassword(const std::string& account, const std::string& newPassword) {
        return m_dao.updatePassword(account, newPassword);
    }

    bool verifyPassword(const std::string& account, const std::string& password) {
        return m_dao.verifyPassword(account, password);
    }

private:
    MySQLManager() = default;
    ~MySQLManager() = default;

    MySQLDao m_dao;
};