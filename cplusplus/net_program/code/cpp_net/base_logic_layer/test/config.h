// config.h — 全局配置宏
// 所有可调参数集中在此，方便修改
#pragma once

#include <cstdint>
#include <cstddef>

// ===== TLV 协议头部字段 =====
// 协议格式：[ HEAD_LEN 字节 header ][ body: body_len 字节 ]
// header 内容：[ body_len: 4B ]  (uint32_t, 网络字节序)
constexpr size_t   HEAD_LEN    = 4;        // 头部长度（字节）
constexpr uint32_t MAX_BODY    = 4 * 1024 * 1024;  // 消息体最大长度（4MB）
constexpr uint32_t MAX_MSG_SIZE = MAX_BODY + HEAD_LEN; // 完整消息最大长度

// ===== 服务器配置 =====
constexpr uint16_t SERVER_PORT = 9190;      // 监听端口
constexpr size_t   MSG_QUEUE_MAX = 10000;   // 消息队列最大长度

// ===== 线程配置 =====
constexpr unsigned int LOGIC_THREAD_COUNT = 1;   // 逻辑处理线程数
constexpr unsigned int NET_THREAD_COUNT   = 2;   // 网络 io_context 线程数
