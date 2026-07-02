#pragma only

#include <boost/asio.hpp>
#include <iostream>
#include <memory>
#include <queue>

const int RECVSIZE = 1024;

/**
 * Used to manage `data` to be sent and received, this structure contains the
 * starting address of the data buffer, the total data length, and the length
 * already processed (i.e., the length read or written).
 */
class msg_node
{
  public:
    msg_node(const char *msg, int total);

    msg_node(int total);

    ~msg_node();

    int _total_len;

    int _cur_len; // new_pos = _msg + _cur_len;

    char *_msg;
};

class session : public std::enable_shared_from_this<session>
{
  public:
    session(std::shared_ptr<boost::asio::ip::tcp::socket> sock);
    void connect(const boost::asio::ip::tcp::endpoint &ep);

  public:
    // ERROR: w/r!!!
    void write_callback_err(const boost::system::error_code &ec,
                            std::size_t bytes_transferred,
                            std::shared_ptr<msg_node> send_node);
    void write2socket_err(const std::string &buf);

  public:
    void write_callback(const boost::system::error_code &ec,
                        std::size_t bytes_transferred);
    void write2socket(const std::string &buf);

    void write_all_callback(const boost::system::error_code &ec,
                            std::size_t bytes_transferred);
    void write_all_to_socket(const std::string &buf);

    void read_from_socket();
    void read_call_back(const boost::system::error_code &ec,
                        std::size_t bytes_transferred);

    void read_all_from_socket();
    void read_all_callback(const boost::system::error_code &ec,
                           std::size_t bytes_transferred);

  private:
    std::shared_ptr<boost::asio::ip::tcp::socket> _sock;
    std::shared_ptr<msg_node> _send_node;
    std::queue<std::shared_ptr<msg_node>> _send_queue;

    std::shared_ptr<msg_node> _recv_node;

    bool _send_pending;
    bool _recv_pending;
};
