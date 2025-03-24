#pragma once

enum ErrorCodes {
    Success = 0,
    ErrorNetWork = -1,
    ErrorAccountOrPwd = 1001,
    ErrorVerifyCode = 1002,
    ErrorVerifyExpired = 1003,  // 验证码过期
    ErrorJsonParse = 1004,
    UserExist = 1005,  // 用户已经存在
    EmailExist = 1006,
    ErrorRpcFailed,
    EmailNotMatch,   // 邮箱不匹配
    PasswdUpFailed,  // 更新密码失败
    PasswdInvalid,   // 密码更新失败
    TokenInvalid,    // Token失效
    UidInvalid,      // uid无效

    UnknownError,
};