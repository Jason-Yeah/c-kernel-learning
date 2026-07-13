#pragma once

#include <cstddef>
#include <cstdint>

constexpr size_t HEAD_LEN = 4;
constexpr uint32_t MAX_BODY = 4 * 1024 * 1024;
constexpr uint32_t MAX_MSG_SIZE = MAX_BODY + HEAD_LEN;

constexpr uint16_t SERVER_PORT = 9190;
const size_t MSG_QUEUE_MAX = 1e5;

constexpr unsigned int LOGIC_THREAD_CNT = 1;
constexpr unsigned int NET_THREAD_CNT = 2;
