#include "redis_connection_pool.hpp"

RedisConnectionPool::RedisConnectionPool() {
    m_url = "tcp://127.0.0.1:6379";
    m_max_size = 5;
    m_timeout = std::chrono::milliseconds(1);
    try {
        for (size_t i = 0; i < m_max_size; ++i) {
            auto conn = create_connection();
            m_pool.push(std::move(conn));
            m_active_conn++;
        }
    } catch (const Error& e) {
        clear_pool();
        throw;
    }
}

RedisConnectionPool::~RedisConnectionPool() { clear_pool(); }

RedisConnectionPool::RedisGuard RedisConnectionPool::get() {
    std::unique_lock<std::mutex> lk{m_mutex};
    if (!m_cv.wait_for(lk, m_timeout, [this]() { return !m_pool.empty(); })) {
        throw std::runtime_error("获取 Redis 连接超时");
    }

    // 取出连接
    auto conn = std::move(m_pool.front());
    m_pool.pop();
    lk.unlock();

    // 检查连接是否有效
    if (!validate_connection(conn)) {
        return RedisGuard(*this, create_connection());
    }
    return RedisGuard(*this, std::move(conn));
}

Redis RedisConnectionPool::create_connection() {
    try {
        Redis redis(m_url);
        redis.auth("123456");
        return redis;
    } catch (const Error& e) {
        throw std::runtime_error("无法创建 Redis 连接: " + std::string(e.what()));
    }
}

// 验证连接是否有效
bool RedisConnectionPool::validate_connection(Redis& conn) {
    try {
        conn.ping();
        return true;
    } catch (const Error&) {
        return false;
    }
}

void RedisConnectionPool::release(Redis&& conn) {
    std::lock_guard<std::mutex> lk{m_mutex};
    if (validate_connection(conn)) {
        m_pool.push(std::move(conn));
    } else {
        m_active_conn--;
        try {
            auto new_conn = create_connection();
            m_pool.push(std::move(new_conn));
            m_active_conn++;
        } catch (const Error&) {
            // 创建失败，不做额外处理
        }
    }
    m_cv.notify_one();
}

void RedisConnectionPool::clear_pool() {
    std::lock_guard<std::mutex> lk{m_mutex};
    while (!m_pool.empty()) {
        m_pool.pop();
    }
    m_active_conn = 0;
}
