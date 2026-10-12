#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <sys/epoll.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <fcntl.h>
#include <cerrno>
#include <cstdint>
#include <cstring>
#include <iostream>
#include <stdexcept>
#include <string>
#include <system_error>
#include <unordered_map>
#include <vector>
#include <utility>
[[noreturn]] static void fail(const char* s){throw std::system_error(errno,std::generic_category(),s);}
class Fd{int x_=-1;public:explicit Fd(int x=-1):x_(x){}~Fd(){if(x_>=0)::close(x_);}Fd(const Fd&)=delete;Fd&operator=(const Fd&)=delete;Fd(Fd&&o)noexcept:x_(std::exchange(o.x_,-1)){}Fd&operator=(Fd&&o)noexcept{if(this!=&o){if(x_>=0)::close(x_);x_=std::exchange(o.x_,-1);}return *this;}int get()const{return x_;}};
struct Client{std::string out;std::size_t sent=0;bool eof=false;};
static void ctl(int ep,int op,int fd,std::uint32_t events){epoll_event e{};e.events=events;e.data.fd=fd;if(::epoll_ctl(ep,op,fd,&e)<0)fail("epoll_ctl");}
static void update(int ep,int fd,Client&c,bool et){std::uint32_t e=0;if(!c.eof)e|=EPOLLIN|EPOLLRDHUP;if(et)e|=EPOLLET;if(c.sent<c.out.size())e|=EPOLLOUT;ctl(ep,EPOLL_CTL_MOD,fd,e);}
int main(int argc,char**argv)try{
 if(argc<2||argc>3)throw std::runtime_error("usage: epoll_tcp_udp PORT [lt|et]");
 char*end=nullptr;unsigned long p=strtoul(argv[1],&end,10);if(!argv[1][0]||*end||p>65535)throw std::runtime_error("invalid port");
 bool et=argc==3&&std::string(argv[2])=="et";if(argc==3&&!et&&std::string(argv[2])!="lt")throw std::runtime_error("mode must be lt or et");
 Fd tcp(::socket(AF_INET,SOCK_STREAM|SOCK_NONBLOCK|SOCK_CLOEXEC,0));if(tcp.get()<0)fail("tcp socket");
 Fd udp(::socket(AF_INET,SOCK_DGRAM|SOCK_NONBLOCK|SOCK_CLOEXEC,0));if(udp.get()<0)fail("udp socket");
 int yes=1;if(::setsockopt(tcp.get(),SOL_SOCKET,SO_REUSEADDR,&yes,sizeof yes)<0)fail("setsockopt");
 sockaddr_in a{};a.sin_family=AF_INET;a.sin_port=htons((unsigned short)p);a.sin_addr.s_addr=htonl(INADDR_LOOPBACK);
 if(::bind(tcp.get(),reinterpret_cast<sockaddr*>(&a),sizeof a)<0)fail("tcp bind");
 if(::bind(udp.get(),reinterpret_cast<sockaddr*>(&a),sizeof a)<0)fail("udp bind");
 if(::listen(tcp.get(),128)<0)fail("listen");
 socklen_t alen=sizeof a;if(::getsockname(tcp.get(),reinterpret_cast<sockaddr*>(&a),&alen)<0)fail("getsockname");
 // 如果请求端口为 0，TCP bind 分配的端口需用于 UDP；为避免端口竞争，重新绑定 UDP。
 if(p==0){Fd replacement(::socket(AF_INET,SOCK_DGRAM|SOCK_NONBLOCK|SOCK_CLOEXEC,0));if(replacement.get()<0)fail("udp socket");if(::bind(replacement.get(),reinterpret_cast<sockaddr*>(&a),sizeof a)<0)fail("udp bind allocated port");udp=std::move(replacement);}
 Fd ep(::epoll_create1(EPOLL_CLOEXEC));if(ep.get()<0)fail("epoll_create1");
 std::uint32_t base=EPOLLIN;if(et)base|=EPOLLET;ctl(ep.get(),EPOLL_CTL_ADD,tcp.get(),base);ctl(ep.get(),EPOLL_CTL_ADD,udp.get(),base);
 std::unordered_map<int,Client> clients;std::vector<epoll_event> events(128);
 std::cout<<"TCP+UDP echo 127.0.0.1:"<<ntohs(a.sin_port)<<" mode="<<(et?"ET":"LT")<<"\n";
 for(;;){int n=::epoll_wait(ep.get(),events.data(),(int)events.size(),-1);if(n<0){if(errno==EINTR)continue;fail("epoll_wait");}
  for(int i=0;i<n;i++){int fd=events[i].data.fd;auto flags=events[i].events;
   if(fd==tcp.get()){
    for(;;){int c=::accept4(tcp.get(),nullptr,nullptr,SOCK_NONBLOCK|SOCK_CLOEXEC);if(c<0){if(errno==EINTR)continue;if(errno==EAGAIN||errno==EWOULDBLOCK)break;std::cerr<<"accept4: "<<strerror(errno)<<'\n';break;}clients.emplace(c,Client{});std::uint32_t ce=EPOLLIN|EPOLLRDHUP;if(et)ce|=EPOLLET;ctl(ep.get(),EPOLL_CTL_ADD,c,ce);}
   }else if(fd==udp.get()){
    for(;;){char b[65536];sockaddr_storage peer{};socklen_t len=sizeof peer;ssize_t r=::recvfrom(fd,b,sizeof b,0,reinterpret_cast<sockaddr*>(&peer),&len);if(r<0){if(errno==EINTR)continue;if(errno==EAGAIN||errno==EWOULDBLOCK)break;std::cerr<<"recvfrom: "<<strerror(errno)<<'\n';break;}if(::sendto(fd,b,(size_t)r,MSG_NOSIGNAL,reinterpret_cast<sockaddr*>(&peer),len)<0&&errno!=EAGAIN)std::cerr<<"sendto: "<<strerror(errno)<<'\n';}
   }else{
    auto it=clients.find(fd);if(it==clients.end())continue;auto&c=it->second;bool alive=true;
    if(flags&(EPOLLIN|EPOLLRDHUP|EPOLLHUP)){char b[4096];for(;;){ssize_t r=::recv(fd,b,sizeof b,0);if(r>0){c.out.append(b,(size_t)r);continue;}if(r==0){c.eof=true;break;}if(errno==EINTR)continue;if(errno==EAGAIN||errno==EWOULDBLOCK)break;alive=false;break;}}
    if(alive&&(flags&EPOLLOUT||c.sent<c.out.size())){while(c.sent<c.out.size()){ssize_t w=::send(fd,c.out.data()+c.sent,c.out.size()-c.sent,MSG_NOSIGNAL);if(w>0){c.sent+=(size_t)w;continue;}if(w<0&&errno==EINTR)continue;if(w<0&&(errno==EAGAIN||errno==EWOULDBLOCK))break;alive=false;break;}if(c.sent==c.out.size()){c.out.clear();c.sent=0;}}
    if(flags&EPOLLERR)alive=false;
    if(c.eof&&c.sent==c.out.size())alive=false;
    if(!alive){::epoll_ctl(ep.get(),EPOLL_CTL_DEL,fd,nullptr);::close(fd);clients.erase(it);}else update(ep.get(),fd,c,et);
   }
  }
 }
}catch(const std::exception&e){std::cerr<<e.what()<<'\n';return 1;}
