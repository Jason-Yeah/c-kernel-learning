
#ifndef SESSION_HPP
#define SESSION_HPP

#include <boost/asio.hpp>
#include <memory>
#include <mutex>
#include <queue>
#include <string>

class server;

/**
 * When sending data, the transmission might not be completed; this issue is
 * resolved by encapsulating the data in a `msg_node` class.
 */
class msg_node
{
    friend class session;

public:
    msg_node(char *msg, int max_len);

    ~msg_node();

    const char *data() const { return _data.data(); }
    std::size_t size() const { return _data.size(); }

private:
    int _cur_len;
    int _max_len;
    std::string _data;
};

class session : public std::enable_shared_from_this<session>
{
public:
    session(boost::asio::io_context &ioc, server *sev);

    boost::asio::ip::tcp::socket &get_socket();

    void start();

    std::string &get_uuid();

    void send(char *msg, int max_len);

private:
    // one or more times read
    void handle_read(const boost::system::error_code &ec,
                     std::size_t bytes_transfered);

    // only one time write the whole data
    void handle_write(const boost::system::error_code &ec);

private:
    boost::asio::ip::tcp::socket _sock;
    // enum {MAX_LENGTG = 1024}; //  C++03 永远合法，永远是编译期常量
    static constexpr std::size_t MAX_LENGTH = 1024; // C++11
    char _data[MAX_LENGTH];
    server *_server;
    std::string _uuid;

private:
    std::queue<std::shared_ptr<msg_node>> _send_queue;
    std::mutex _send_mtx;
};

#endif // SESSION_HPP
