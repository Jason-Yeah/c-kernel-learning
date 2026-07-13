// message.hpp — 消息数据结构定义（TLV 协议）
#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

// ===== TLV 协议格式 =====
// [ body_len: 4B (htonl) ][ body: body_len B ]
//    ↑ 头部：头部长度 = HEAD_LEN           ↑ 消息体

// TLV 消息体（不包含 header）
struct MessageBody
{
    uint8_t type;           // 消息类型
    std::vector<char> data; // 消息体数据

    MessageBody() : type(0) {}
    MessageBody(uint8_t t, const std::vector<char> &d) : type(t), data(d) {}
};

// 逻辑层内部传输的完整消息（包含来源 Session ID）
struct LogicMessage
{
    uint64_t session_id;    // 来源 Session（用于回复）
    uint16_t msg_type;      // 消息类型
    std::vector<char> body; // 消息体（二进制）

    LogicMessage() : session_id(0), msg_type(0) {}
    LogicMessage(uint64_t sid, uint16_t type, const std::vector<char> &b)
        : session_id(sid), msg_type(type), body(b)
    {
    }
};

// 消息类型枚举
enum MsgType : uint16_t
{
    MSG_UNKNOWN = 0,
    MSG_LOGIN = 1,
    MSG_CHAT = 2,
    MSG_HEARTBEAT = 3,
    MSG_LOGOUT = 4,
};
