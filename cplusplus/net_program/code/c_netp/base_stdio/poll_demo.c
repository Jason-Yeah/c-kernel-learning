#include <stdio.h>
#include <poll.h>
#include <unistd.h>

// poll轮询
/*
struct pollfd
  {
    int fd;			        File descriptor to poll.  
    short int events;		Types of events poller cares about.  
    short int revents;		Types of events that actually occurred.  
  };
*/

int main()
{
    struct pollfd fds[1];
    fds[0].fd = 0; // stdin
    fds[0].events = POLLIN;
    fds[0].revents = 0;

    int ret = poll(fds, 1, -1);

    if (ret == -1) perror("poll error");
    else if (ret == 0) printf("timeout\n");
    else
    {
        if (fds[0].revents & POLLIN)
        {
            char buf[20];
            read(0, buf, sizeof(buf));
            printf("S: %s", buf);
        }
    }
    return 0;
}