- 用户表
```sql
CREATE TABLE users (
    id INT AUTO_INCREMENT PRIMARY KEY,        -- 用户ID，主键，自增
    account VARCHAR(50) UNIQUE NOT NULL,      -- 账号，唯一
    password VARCHAR(255) NOT NULL,           -- 密码，加密存储
    email VARCHAR(100) UNIQUE NOT NULL,       -- 邮箱，唯一
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP  -- 注册时间
);
```

- 消息表
```sql
CREATE TABLE messages (
    id INT AUTO_INCREMENT PRIMARY KEY,        -- 消息ID，主键，自增
    sender_id INT NOT NULL,                   -- 发送者ID
    receiver_id INT NOT NULL,                 -- 接收者ID
    type ENUM('text', 'image', 'file') NOT NULL,  -- 消息类型：文本、图片、文件
    content TEXT NOT NULL,                    -- 消息内容：文本或文件路径
    sent_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,  -- 发送时间
    is_read BOOLEAN DEFAULT FALSE,            -- 是否已读
    FOREIGN KEY (sender_id) REFERENCES users(id),    -- 外键关联发送者
    FOREIGN KEY (receiver_id) REFERENCES users(id)   -- 外键关联接收者
);
```