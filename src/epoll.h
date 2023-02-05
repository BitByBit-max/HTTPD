#include <sys/epoll.h>
#include <stddef.h>

struct event_info
{
    size_t b_read; // what has been read so far
    int fd;
    char buff[];
};
