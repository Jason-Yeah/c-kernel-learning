#ifndef MSG_NODE_HPP
#define MSG_NODE_HPP

#include <boost/asio.hpp>
#include <iostream>
#include <string>
#include <vector>

class msg_node
{
public:
    msg_node(short max_len);

    virtual ~msg_node();

    void clear();

protected:
    short _cur_len;
    short _total_len;
    std::vector<char> _data;
};

#endif // MSG_NODE_HPP

// RECV_NODE
class recv_node : public msg_node
{
public:
    recv_node(short max_len, short id);

    ~recv_node() override = default;

private:
    short _msg_id;
};

// SEND_NODE
class send_node : public msg_node
{
public:
    send_node(const std::vector<char> &data, short max_len, short id);

    ~send_node() override = default;

private:
    short _msg_id;
};
