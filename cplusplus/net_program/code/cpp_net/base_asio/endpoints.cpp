#include "endpoint.h"
#include <boost/asio.hpp>
#include <iostream>

// using namespace boost;

int client_end_point()
{
    std::string raw_ip_addr = "127.4.8.1";
    unsigned short port = 9190;

    boost::system::error_code ec;
    boost::asio::ip::address ip_addr =
        boost::asio::ip::make_address(raw_ip_addr, ec);
    if (ec.value()) // not zero
    {
            std::cout << "Failed to parse the IP address. Error code = "
                    << ec.value() << ". Message is: " << ec.message()
                    << std::endl;
            return ec.value();
    }

    // create_endpoint same as: struct sockaddr_in addr; addr.sin_addr.s_addr =
    // INADDR_ANY;....
    boost::asio::ip::tcp::endpoint ep(ip_addr, port);

    return 0;
}

int server_end_point()
{
    unsigned short port = 9190;
    // 服务器端需监听“本机所有网络接口” <=> INADDR_ANY 0.0.0.0
    boost::asio::ip::address ip_addr = boost::asio::ip::address_v4::any();

    // TCP communication endpoint
    boost::asio::ip::tcp::endpoint ep(ip_addr, port);
    return 0;
}

int create_tcp_socket()
{
    // parameter IO_CONTEXT used to communicate...
    // (总调度、定时器、socket分配资源)
    boost::asio::io_context ioc;
    boost::asio::ip::tcp protocol =
        boost::asio::ip::tcp::v4(); // BOOST_ASIO_OS_DEF_AF_INET
    /*
    // 未打开状态，仅创建sock对象还没有打开底层的fd
    boost::asio::ip::tcp::socket sock(ioc);

    boost::system::error_code ec;
    // 打开sock，相当于底层的socket(AF_INET, SOCK_STREAM, 0);
    // protocal告诉内核采用IPv4 TCP
    sock.open(protocol, ec);
    if (ec.value())
    {
        std::cout << "Failed to parse the IP address. Error code = "
                  << ec.value() << ". Message is: " << ec.message()
                  << std::endl;
        return ec.value();
    }
    */

    // 新方式: 构造函数直接打开 fd
    boost::asio::ip::tcp::socket sock(ioc, protocol);

    return 0;
}

int create_acceptor_socket()
{
    boost::asio::io_context ioc;
    /*
    boost::asio::ip::tcp::acceptor acceptor(ioc);

    boost::asio::ip::tcp protocol = boost::asio::ip::tcp::v4();
    boost::system::error_code ec;
    acceptor.open(protocol, ec);
    if (ec.value())
    {
        std::cout << "Failed to parse the IP address. Error code = "
                  << ec.value() << ". Message is: " << ec.message()
                  << std::endl;
        return ec.value();
    }
    */

    // socket + bind + listen
    // 创建accepter， bind(9190)， listen()
    /* c:
        int listen_fd = socket(AF_INET, SOCK_STREAM, 0);
        struct sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(9190);
        addr.sin_addr.s_addr = INADDR_ANY;
        bind(listen_fd, (struct sockaddr*)&addr, sizeof(addr));
        listen(listen_fd, SOMAXCONN);
    */
    boost::asio::ip::tcp::acceptor acceptor(
        ioc, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), 9190));
    return 0;
}

int bind_acceptor_socket()
{
    unsigned short port = 9190;
    boost::asio::ip::tcp::endpoint ep(boost::asio::ip::address_v4::any(), port);

    boost::asio::io_context ioc;

    // 仅打开构造Open-only，只创建了socket fd没bind和listen
    boost::asio::ip::tcp::acceptor acceptor(ioc, ep.protocol());
    boost::system::error_code ec;

    acceptor.bind(ep, ec);
    if (ec.value())
    {
        std::cout << "Failed to parse the IP address. Error code = "
                  << ec.value() << ". Message is: " << ec.message()
                  << std::endl;
        return ec.value();
    }

    return 0;
}

int connect_to_end()
{
    std::string raw_ip_addr = "192.168.1.124";
    unsigned short port = 9190;

    try
    {
        boost::asio::ip::tcp::endpoint ep(
            boost::asio::ip::make_address(raw_ip_addr), port);
        boost::asio::io_context ioc;
        boost::asio::ip::tcp::socket sock(ioc, ep.protocol());

        // C: connect(fd, (sockaddr*)&addr, sizeof(addr));
        sock.connect(ep);

        return 0;
    }
    catch (boost::system::system_error &se)
    {
        std::cout << "Error coccured! Error code = " << se.code()
                  << ". Message: " << se.what() << std::endl;
        return se.code().value();
    }
}

int dns_connect_to_end()
{
    std::string host = "llfc.club";
    unsigned short port = 9190;
    boost::asio::io_context ioc;
    // 域名解析 tcp::resolver
    // C: getaddrinfo("llfc.club", "9190", ..., ...);
    boost::asio::ip::tcp::resolver resolver(ioc);

    try
    {
        // gethostbyname(char* str);
        // resolve()返回是一个范围（一个域名可能对应多个IP）
        auto endpoints = resolver.resolve(host, std::to_string(port));
        auto ep = *endpoints.begin();
        // ep = endpoints.front();

        boost::asio::ip::tcp::socket sock(ioc);
        // asio::connect函数接收一个端点范围，ASIO会逐一尝试直到成功
        boost::asio::connect(sock, endpoints);

        std::cout << "Connected to: " << ep.endpoint() << std::endl;
        return 0;
    }
    catch (boost::system::system_error &se)
    {
        std::cout << "Error coccured! Error code = " << se.code()
                  << ". Message: " << se.what() << std::endl;
        return se.code().value();
    }
}

int accept_new_connection()
{
    const int BACKLOG_SIZE = 30; // 等待队列最大长度
    unsigned short port = 9190;
    boost::asio::ip::tcp::endpoint ep(boost::asio::ip::address_v4::any(), port);
    boost::asio::io_context ioc;

    try
    {
        // ep.protocol == {AF_INET, SOCK_STREAM, IPPROTO_TCP}
        // Only-open Constructor. Only open socket.
        boost::asio::ip::tcp::acceptor acceptor(ioc, ep.protocol());
        acceptor.bind(ep);             // bind
        acceptor.listen(BACKLOG_SIZE); // <==> listen(int fd, int n)
        boost::asio::ip::tcp::socket sock(ioc);

        // 用于和客户端通信 int conn = accept(fd, NULL, NULL);
        // 阻塞 指导有客户端连接上来
        acceptor.accept(sock);

        // 到这里sock已经连接到客户端
        return 0;
    }
    catch (boost::system::system_error &se)
    {
        std::cout << "Error coccured! Error code = " << se.code()
                  << ". Message: " << se.what() << std::endl;
        return se.code().value();
    }
}

void use_const_buffer()
{
    std::string buf = "Hello world";
    boost::asio::const_buffer asio_cbuf(buf.data(), buf.length());
    std::vector<boost::asio::const_buffer> buffers_sequence;
    buffers_sequence.push_back(asio_cbuf);
}

void use_buffer_str()
{
    boost::asio::const_buffer output_buf = boost::asio::buffer("hello...");
}

void use_buffer_array()
{
    const size_t BUF_SIZE_BYTES = 20;
    std::unique_ptr<char[]> buf(new char[BUF_SIZE_BYTES]);
    auto input_bug =
        boost::asio::buffer(static_cast<void *>(buf.get()), BUF_SIZE_BYTES);
}

void write_to_socket(boost::asio::ip::tcp::socket &sock)
{
    std::string buf = "11112222";
    std::size_t total_bytes_written = 0;
    boost::system::error_code ec;
    // Cycle sending
    // write_some(): return the byte size for every writing.
    while (total_bytes_written < buf.size())
    {
        auto n = sock.write_some(
            boost::asio::buffer(buf.data() + total_bytes_written,
                                buf.size() - total_bytes_written),
            ec);
        if (ec)
            break;
        total_bytes_written += n;
    }
}

int send_data_by_write_some()
{
    std::string raw_ip_addr = "127.0.0.1";
    unsigned short port = 9190;
    try
    {
        boost::asio::ip::tcp::endpoint ep(
            boost::asio::ip::make_address(raw_ip_addr), port);
        boost::asio::io_context ioc;
        boost::asio::ip::tcp::socket sock(ioc, ep.protocol()); // tcp::AF_INET
        sock.connect(ep);
        // write_to_socket(sock);
        std::string buf = "Hello .... ";
        /*
            The function
            call will block until one or more bytes of the data has been sent
            successfully, or an until error occurs.
        */
        int send_size = sock.send(boost::asio::buffer(buf.data(), buf.size()));
        if (send_size <= 0)
            return 0;
    }
    catch (boost::system::system_error &se)
    {
        std::cout << "Error coccured! Error code = " << se.code()
                  << ". Message: " << se.what() << std::endl;
        return se.code().value();
    }
    return 0;
}

int write_data_by_send()
{
    std::string raw_ip_addr = "127.0.0.1";
    unsigned short port = 9190;
    try
    {
        boost::asio::ip::tcp::endpoint ep(
            boost::asio::ip::make_address(raw_ip_addr), port);
        boost::asio::io_context ioc;
        boost::asio::ip::tcp::socket sock(ioc, ep.protocol()); // tcp::AF_INET
        sock.connect(ep);
        // write_to_socket(sock);
        std::string buf = "Hello .... ";
        // 一口气写完
        int send_size = boost::asio::write(
            sock, boost::asio::buffer(buf.data(), buf.size()));
        if (send_size <= 0)
            return 0;
    }
    catch (boost::system::system_error &se)
    {
        std::cout << "Error coccured! Error code = " << se.code()
                  << ". Message: " << se.what() << std::endl;
        return se.code().value();
    }
    return 0;
}

std::string read_from_sock(boost::asio::ip::tcp::socket &sock)
{
    const unsigned char MESSAGE_SIZE = 7;
    char buf[MESSAGE_SIZE];
    std::size_t total_bytes_read = 0;
    while (total_bytes_read < MESSAGE_SIZE)
    {
        total_bytes_read += sock.read_some(boost::asio::buffer(
            buf + total_bytes_read, MESSAGE_SIZE - total_bytes_read));
    }

    return std::string(buf, total_bytes_read);
}

int read_data_by_read_some()
{
    std::string raw_ip_addr = "127.0.0.1";
    unsigned short port = 9190;
    try
    {
        boost::asio::ip::tcp::endpoint ep(
            boost::asio::ip::make_address(raw_ip_addr), port);
        boost::asio::io_context ioc;
        boost::asio::ip::tcp::socket sock(ioc, ep.protocol()); // tcp::AF_INET
        sock.connect(ep);
        read_from_sock(sock); // 会阻塞
        return 0;
    }
    catch (boost::system::system_error &se)
    {
        std::cout << "Error coccured! Error code = " << se.code()
                  << ". Message: " << se.what() << std::endl;
        return se.code().value();
    }
    return 0;
}

int read_data_by_read_receive()
{
    std::string raw_ip_addr = "127.0.0.1";
    unsigned short port = 9190;
    try
    {
        boost::asio::ip::tcp::endpoint ep(
            boost::asio::ip::make_address(raw_ip_addr), port);
        boost::asio::io_context ioc;
        boost::asio::ip::tcp::socket sock(ioc, ep.protocol()); // tcp::AF_INET
        sock.connect(ep);
        // read_from_sock(sock); // 会阻塞
        const unsigned char BUF_SIZE = 7;
        char buf_recv[BUF_SIZE];
        int recv_len = sock.receive(boost::asio::buffer(buf_recv, BUF_SIZE));
        if (recv_len <= 0)
            std::cout << "receive failed.\n";
        return 0;
    }
    catch (boost::system::system_error &se)
    {
        std::cout << "Error coccured! Error code = " << se.code()
                  << ". Message: " << se.what() << std::endl;
        return se.code().value();
    }
    return 0;
}

int read_data_by_read_func()
{
    std::string raw_ip_addr = "127.0.0.1";
    unsigned short port = 9190;
    try
    {
        boost::asio::ip::tcp::endpoint ep(
            boost::asio::ip::make_address(raw_ip_addr), port);
        boost::asio::io_context ioc;
        boost::asio::ip::tcp::socket sock(ioc, ep.protocol()); // tcp::AF_INET
        sock.connect(ep);
        // read_from_sock(sock); // 会阻塞
        const unsigned char BUF_SIZE = 7;
        char buf_recv[BUF_SIZE];
        // int recv_len = sock.receive(boost::asio::buffer(buf_recv, BUF_SIZE));
        int recv_len =
            boost::asio::read(sock, boost::asio::buffer(buf_recv, BUF_SIZE));
        if (recv_len <= 0)
            std::cout << "receive failed.\n";
        return 0;
    }
    catch (boost::system::system_error &se)
    {
        std::cout << "Error coccured! Error code = " << se.code()
                  << ". Message: " << se.what() << std::endl;
        return se.code().value();
    }
    return 0;
}
