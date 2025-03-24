#pragma once

#include <sw/redis++/redis++.h>

#include <chrono>
#include <common/Singleton.hpp>
#include <condition_variable>
#include <mutex>
#include <queue>

using namespace sw::redis;

class RedisConnectionPool : public Singleton<RedisConnectionPool> {
    friend class Singleton<RedisConnectionPool>;

public:
    // 封装一个智能指针，确保连接在作用域结束时归还
    class RedisGuard {
    public:
        RedisGuard(RedisConnectionPool& pool, Redis&& conn)
            : m_pool(pool), m_conn(std::move(conn)) {}

        // 移动构造
        RedisGuard(RedisGuard&& other) noexcept
            : m_pool(other.m_pool), m_conn(std::move(other.m_conn)) {}

        // 禁止拷贝
        RedisGuard(const RedisGuard&) = delete;
        RedisGuard& operator=(const RedisGuard&) = delete;

        // 作用域结束自动归还
        ~RedisGuard() { m_pool.release(std::move(m_conn)); }

        // 提供 Redis 操作接口
        Redis& operator*() { return m_conn; }
        Redis* operator->() { return &m_conn; }

    private:
        RedisConnectionPool& m_pool;
        Redis m_conn;
    };

    ~RedisConnectionPool();

    RedisGuard get();

private:
    RedisConnectionPool();

    Redis create_connection();
    // 验证连接是否有效
    bool validate_connection(Redis& conn);

    void release(Redis&& conn);

    void clear_pool();

    std::queue<Redis> m_pool;
    std::mutex m_mutex;
    std::condition_variable m_cv;
    std::atomic<size_t> m_active_conn;
    size_t m_max_size;
    std::string m_url;
    std::chrono::milliseconds m_timeout;
};