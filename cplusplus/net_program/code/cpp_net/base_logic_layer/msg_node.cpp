#include "msg_node.hpp"
#include "arpa/inet.h"
#include "config.h"

msg_node::msg_node(short max_len) : _cur_len(0), _total_len(max_len)
{
    // 仅分配内存实际size() == 0，避免后续push_back/insert扩容s
    _data.resize(max_len);
}

msg_node::~msg_node() { std::cout << "Destruct msg node..." << std::endl; }

void msg_node::clear()
{
    _data.clear();
    _cur_len = 0;
}

recv_node::recv_node(short max_len, short id) : msg_node(max_len), _msg_id(id)
{
}

send_node::send_node(const std::vector<char> &data, short max_len, short msg_id)
    : msg_node(max_len + HEAD_TOTAL_LEN), _msg_id(msg_id)
{
    short id = htons(msg_id);
    memcpy(_data.data(), &id, HEAD_ID_LEN);

    short len = htons(max_len);
    memcpy(_data.data() + HEAD_ID_LEN, &len, HEAD_DATA_LEN);

    memcpy(_data.data() + HEAD_TOTAL_LEN, data.data(), max_len);
}
