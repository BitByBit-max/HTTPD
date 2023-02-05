#ifndef SOCKET_H
#define SOCKET_H

#include <arpa/inet.h>
#include <fcntl.h>
#include <netdb.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "parsers.h"

struct event_info
{
    size_t b_read; // what has been read so far
    int fd;
    char *ip;
    char *buff;
    struct token *request;
};

struct list
{
    struct event_info *event;
    struct list *next;
};

int connect_server(struct token *conf, char *conf_file); 

#endif /* ! SOCKET_H */
