#define _POSIX_C_SOURCE 200809L
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <poll.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CLIENT_MAX 128
static int make_bound_socket(int type, unsigned short port) {
    int fd = socket(AF_INET, type, 0);
    if (fd < 0) { perror("socket"); return -1; }
    int yes = 1;
    if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof yes) < 0) { perror("setsockopt"); close(fd); return -1; }
    struct sockaddr_in a = {0}; a.sin_family = AF_INET; a.sin_port = htons(port);
    a.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    if (bind(fd, (struct sockaddr *)&a, sizeof a) < 0) { perror("bind"); close(fd); return -1; }
    return fd;
}
static int parse_port(const char *s, unsigned short *port) {
    char *end = NULL; errno = 0; unsigned long n = strtoul(s, &end, 10);
    if (errno || !s[0] || *end || n > 65535) return -1;
    *port = (unsigned short)n; return 0;
}
static void echo_tcp_once(int fd) {
    char buf[4096]; ssize_t n; do { n = recv(fd, buf, sizeof buf, 0); } while (n < 0 && errno == EINTR);
    if (n <= 0) { close(fd); return; }
    size_t off = 0;
    while (off < (size_t)n) {
        ssize_t sent = send(fd, buf + off, (size_t)n - off, MSG_NOSIGNAL);
        if (sent < 0) { if (errno == EINTR) continue; perror("send"); close(fd); return; }
        if (sent == 0) { close(fd); return; }
        off += (size_t)sent;
    }
}
static int run_select(int listener, int udp) {
    int clients[CLIENT_MAX]; for (int i=0;i<CLIENT_MAX;i++) clients[i] = -1;
    for (;;) {
        fd_set reads; FD_ZERO(&reads); FD_SET(listener,&reads); FD_SET(udp,&reads);
        int maxfd = listener > udp ? listener : udp;
        for (int i=0;i<CLIENT_MAX;i++) if (clients[i]>=0) { FD_SET(clients[i],&reads); if(clients[i]>maxfd)maxfd=clients[i]; }
        int ready = select(maxfd+1,&reads,NULL,NULL,NULL);
        if (ready < 0) { if(errno==EINTR)continue; perror("select"); return 1; }
        if (FD_ISSET(listener,&reads)) {
            int c = accept(listener,NULL,NULL);
            if(c>=0) { int i; for(i=0;i<CLIENT_MAX;i++)if(clients[i]<0)break;
                if(i==CLIENT_MAX)close(c); else clients[i]=c; }
            else if(errno!=EINTR)perror("accept");
        }
        if (FD_ISSET(udp,&reads)) {
            char buf[4096]; struct sockaddr_storage peer; socklen_t len=sizeof peer;
            ssize_t n=recvfrom(udp,buf,sizeof buf,0,(struct sockaddr*)&peer,&len);
            if(n>=0 && sendto(udp,buf,(size_t)n,0,(struct sockaddr*)&peer,len)<0)perror("sendto");
        }
        for(int i=0;i<CLIENT_MAX;i++) if(clients[i]>=0 && FD_ISSET(clients[i],&reads)) {
            int old=clients[i]; echo_tcp_once(old);
            if(fcntl(old,F_GETFD)==-1 && errno==EBADF) clients[i]=-1;
        }
    }
}
static int run_poll(int listener, int udp) {
    struct pollfd fds[CLIENT_MAX+2];
    fds[0]=(struct pollfd){listener,POLLIN,0}; fds[1]=(struct pollfd){udp,POLLIN,0};
    for(int i=2;i<CLIENT_MAX+2;i++)fds[i]=(struct pollfd){-1,POLLIN,0};
    for (;;) {
        int ready=poll(fds,CLIENT_MAX+2,-1);
        if(ready<0){if(errno==EINTR)continue;perror("poll");return 1;}
        if(fds[0].revents&POLLIN){int c=accept(listener,NULL,NULL);if(c>=0){int i;for(i=2;i<CLIENT_MAX+2;i++)if(fds[i].fd<0)break;if(i==CLIENT_MAX+2)close(c);else fds[i]=(struct pollfd){c,POLLIN,0};}else if(errno!=EINTR)perror("accept");}
        if(fds[1].revents&POLLIN){char b[4096];struct sockaddr_storage peer;socklen_t len=sizeof peer;ssize_t n=recvfrom(udp,b,sizeof b,0,(struct sockaddr*)&peer,&len);if(n>=0&&sendto(udp,b,(size_t)n,0,(struct sockaddr*)&peer,len)<0)perror("sendto");}
        for(int i=2;i<CLIENT_MAX+2;i++) if(fds[i].fd>=0 && fds[i].revents) {
            if(fds[i].revents&(POLLIN|POLLHUP|POLLERR)){int old=fds[i].fd; echo_tcp_once(old); if(fcntl(old,F_GETFD)==-1&&errno==EBADF)fds[i].fd=-1;}
        }
    }
}
int main(int argc,char**argv){
    if(argc!=3){fprintf(stderr,"usage: select_poll select|poll PORT\n");return 2;}
    unsigned short port;if(parse_port(argv[2],&port)<0){fprintf(stderr,"invalid port\n");return 2;}
    int tcp=make_bound_socket(SOCK_STREAM,port), udp=make_bound_socket(SOCK_DGRAM,port);
    if(tcp<0||udp<0)return 1;
    if(listen(tcp,32)<0){perror("listen");return 1;}
    printf("TCP+UDP echo on 127.0.0.1:%u (Ctrl+C to stop)\n",port);
    if(!strcmp(argv[1],"select")){if(tcp>=FD_SETSIZE||udp>=FD_SETSIZE){fprintf(stderr,"fd exceeds FD_SETSIZE; select cannot watch it\n");return 1;}return run_select(tcp,udp);}
    if(!strcmp(argv[1],"poll"))return run_poll(tcp,udp);
    fprintf(stderr,"mode must be select or poll\n");return 2;
}
