#include "epoll.h"

#include "parsers.h"
#include "epoll.h"

#define MAXEVENTS 64

static int make_socket_non_blocking(int sfd)
{
    int s = fcntl(sfd, F_SETFL, O_NONBLOCK);
    if (s == -1)
        return -1; //error case
    return 0;
}

#if 0
static void handle_request(int listening, struct token *conf, int efd,
                           struct epoll_event *event)
{
    _shutdown();
    while (1)
    {
        if (shdo)
            close(listening);
        int waiting = epoll_wait(efd, events, MAXEVENTS, -1);
        if (waiting == -1)
        {
            log_others("epoll wait failed");
            exit(1);
        }
        for (int i = 0; i < waiting; i++)
        {
            if (events[i].data.fd == listening) // is this true ?
            {
                struct sockaddr client;
                socklen_t addr_size;
                int sock = accept(listening, &client, &addr_size);
                if (shdo)
                    close(sock);
                void *addr = &client;
                struct sockaddr_in *getip = addr;
                struct in_addr ipaddr = getip->sin_addr;
                char ip[INET_ADDRSTRLEN];
                inet_ntop(AF_INET, &ipaddr, ip, INET_ADDRSTRLEN);
                if (sock == -1)
                    continue;
                make_socket_non_blocking(sock);
                event.events = EPOLLIN | EPOLLET;
                struct event_info *e_i =
                    calloc(1, sizeof(struct event_info)); // when free?
                e_i->buff = malloc(8192);
                e_i->fd = sock;
                event[i].data.ptr = e_i;
                if (epoll_ctl(efd, EPOLL_CTL_ADD, sock, &event) == -1)
                {
                    log_others("epoll ctl ADD failed");
                    exit(1);
                }
            }
            else if (events[i].events & EPOLLIN)
            {
                struct event_info *e_i = event[i].data.ptr;
                ssize_t nb_read = my_recv(sock, e_i->buff + e_i->b_read,
                                          3000); // read body read all in while
                if (!nb_read)
                    continue; // client disconnected
                e_i->b_read += nbread;
                e_i->buff[nb_read] = '\0';
                char *req = malloc(nb_read);
                if (shdo)
                    free(req);
                req = strncpy(req, e_i->buff, nb_read);
                struct token *request = mess_parse(req);
                if (!request)
                    continue; // no tokens
                if (shdo)
                    destroy_tokens(request);
                char *char_size = get_target(request, CONTENT_LENGTH)
                    ? get_target(request, CONTENT_LENGTH)
                    : "0";
                size_t b_size = atoi(char_size);
                if (b_size)
                {
                    char buff[b_size];
                    get_body(sock, buff, 3000, b_size);
                }
                free(req);
            }
            else if (events[i].events & EPOLLOUT)
            {
                handle_response(conf, ip, sock, request);
                // should the following actually be here ?
                struct event_info *e_i = events[i].data.fd;
                free(e_i->buff);
                free(events[i].data.fd);
            }
        }
        destroy_tokens(request);
        close(sock);
    }
}

int connect_server(char *ip, char *port, struct token *conf)
{
    if (!ip || !port) // is it necessary?? no
        return -1; // invalid ip port err case
    log_others("preparing server"); // to log!
    int listening = create_and_bind(ip, port);
    if (listening == -1)
        return 2; // error cant listen idk?
    s = make_socket_non_blocking(listening);
    if (listen(listening, 10) == -1) // modify number to 100000!
        return 3; // error
    efd = epoll_create1(0);
    if (efd == -1)
        return -1; //error case
    struct epoll_event server = NULL;
    server.data.fd = listenning;
    server.events = EPOLLIN | EPOLLET;
    s = epoll_ctl(efd, EPOLL_CTL_ADD, sfd, &event);
    if (s == -1)
        return -1; //error case
    struct epoll_event *events = calloc(MAXEVENTS, sizeof(struct epoll_event)); //Buffer where events are
                                              //returned => can I put server in
                                              //it ?
    events[1] = server;
    handle_request(listening, conf, efd, events);
    close(listening);
    return 0;
}

int epoll(int argc, char *argv[])
{
    /* The event loop */
    while (1)
    {
        int n, i;
        n = epoll_wait(efd, events, MAXEVENTS, -1);
        for (i = 0; i < n; i++)
        {
            if ((events[i].events & EPOLLERR) || (events[i].events & EPOLLHUP)
                || (!(events[i].events & EPOLLIN)))
            {
                // An error has occured on this fd, or the socket is not ready
                // for reading (why were we notified then?)
                fprintf(stderr, "epoll error");
                close(events[i].data.fd);
                continue;
            }
            else if (sfd == events[i].data.fd)
            {
                /* We have a notification on the listening socket, which
   means one or more incoming connections. */
                while (1)
                {
                    struct sockaddr in_addr;
                    socklen_t in_len;
                    int infd;
                    char hbuf[NI_MAXHOST], sbuf[NI_MAXSERV];

                    in_len = sizeof in_addr;
                    infd = accept(sfd, &in_addr, &in_len);
                    if (infd == -1)
                    {
                        if ((errno == EAGAIN) || (errno == EWOULDBLOCK))
                        {
                            /* We have processed all incoming connections. */
                            break;
                        }
                        else
                        {
                            perror("accept");
                            break;
                        }
                    }
                    s = getnameinfo(&in_addr, in_len, hbuf, sizeof hbuf, sbuf,
                                    sizeof sbuf,
                                    NI_NUMERICHOST | NI_NUMERICSERV);
                    if (s == 0)
                    {
                        printf("Accepted connection on descriptor %d "
                               "(host=%s, port=%s)",
                               infd, hbuf, sbuf);
                    }
                    /* Make the incoming socket non-blocking and add it to the
   list of fds to monitor. */
                    s = make_socket_non_blocking(infd);
                    if (s == -1)
                        abort();
                    event.data.fd = infd;
                    event.events = EPOLLIN | EPOLLET;
                    s = epoll_ctl(efd, EPOLL_CTL_ADD, infd, &event);
                    if (s == -1)
                    {
                        perror("epoll_ctl");
                        abort();
                    }
                }
                continue;
            }
            else
            {
                /* We have data on the fd waiting to be read. Read and
   display it. We must read whatever data is available
   completely, as we are running in edge-triggered mode
   and won't get a notification again for the same
   data. */
                int done = 0;
                while (1) // no need for that can just say while(!done) it's
                          // just a read
                {
                    ssize_t count;
                    char buf[512];
                    count = read(events[i].data.fd, buf, sizeof buf);
                    if (count == -1)
                    {
                        /* If errno == EAGAIN, that means we have read all
   data. So go back to the main loop. */
                        if (errno != EAGAIN)
                        {
                            perror("read");
                            done = 1;
                        }
                        break;
                    }
                    else if (count == 0)
                    {
                        /* End of file. The remote has closed the
   connection. */
                        done = 1;
                        break;
                    }
                    /* Write the buffer to standard output */
                    s = write(1, buf, count);
                    if (s == -1)
                    {
                        perror("write");
                        abort();
                    }
                }
                if (done)
                {
                    printf("Closed connection on descriptor %d",
                           events[i].data.fd);
                    /* Closing the descriptor will make epoll remove it
   from the set of descriptors which are monitored. */
                    close(events[i].data.fd);
                }
            }
        }
    }
    free(events);
    close(sfd);
    return EXIT_SUCCESS;
}
#endif
