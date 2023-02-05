#include "socket.h"

#include <stddef.h>
#include <stdio.h>

#include "annex.h"
#include "log.h"
#include "parsers.h"
#include "response.h"

#define MAXEVENTS 10000

static int shdo = 0;
static struct token *conf = NULL;

static void handler(int signum)
{
    switch (signum)
    {
    case SIGINT:
        shdo = 1;
        close_file();
        break;
    case SIGUSR1:
        shdo = 2;
        log_others("received reload");
        break;
    default:
        break;
    }
}

static void _shutdown(void)
{
    struct sigaction sa;
    sa.sa_flags = 0;
    sa.sa_handler = handler;

    if (sigemptyset(&sa.sa_mask) == -1)
        log_others("mask failure");
    if (sigaction(SIGINT, &sa, NULL) == -1)
        log_others("sigaction failure");
    if (sigaction(SIGUSR1, &sa, NULL) == -1)
        log_others("sigaction failure");
}

static void destroy_events(struct list *shutlist)
{
    struct list *sh = shutlist;
    struct list *curr = shutlist;
    while (curr)
    {
        sh = curr;
        close(sh->event->fd);
        free(sh->event->buff);
        free(sh->event->ip);
        free(sh->event);
        curr = sh->next;
        free(sh);
    }
}

static struct list *add_event(struct list *shutlist, struct event_info *e)
{
    struct list *new = malloc(sizeof(struct list));
    new->event = e;
    new->next = shutlist;
    return new;
}

static struct list *del_event(struct list *shutlist, struct event_info *e)
{
    struct list *curr = shutlist;
    struct list *prev = NULL;
    while (curr && curr->next && curr->event && curr->event->fd != e->fd)
    {
        prev = curr;
        curr = curr->next;
    }
    close(curr->event->fd);
    free(curr->event->buff);
    free(curr->event->ip);
    if (!prev)
        shutlist = curr->next;
    else
        prev->next = curr->next;
    free(curr->event);
    free(curr);
    return shutlist;
}

static int make_socket_non_blocking(int sfd)
{
    int s = fcntl(sfd, F_SETFL, O_NONBLOCK);
    if (s == -1)
        return -1; // error case
    return 0;
}

static int create_and_bind(char *ip, char *port)
{
    struct addrinfo hints;
    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;
    struct addrinfo *list;
    int error = getaddrinfo(ip, port, &hints, &list);
    if (error == -1)
        return -1; // getaddrinfo failed error
    struct addrinfo *cursor;
    int sfd;
    for (cursor = list; cursor; cursor = cursor->ai_next)
    {
        sfd =
            socket(cursor->ai_family, cursor->ai_socktype, cursor->ai_protocol);
        if (sfd == -1)
            continue;
        int yes = 1;
        if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) != 0)
            continue;
        if (bind(sfd, cursor->ai_addr, cursor->ai_addrlen) != -1)
            break; // bind success
        close(sfd);
    }
    freeaddrinfo(list);
    if (!cursor)
        return -1; // no sfd used?? something
    return sfd;
}

static void handle_response(int sfd, struct token *conf, struct token *request,
                            char *ip)
{
    log_others("was able to parse request!");
    char *err = get_token(request, ERROR);
    if (err && !strcmp(err, "400 BAD REQUEST"))
        log_request(get_token(conf, SERVER_NAME), NULL, "Bad Request", ip);
    else
    {
        enum my_key_t method = get_token(request, HEAD)
            ? HEAD
            : (get_token(request, GET) ? GET : UNKNOWN);
        char *req_type =
            method == HEAD ? "HEAD" : (method == GET ? "GET" : "UNKNOWN");
        log_request(get_token(conf, SERVER_NAME), req_type,
                    get_token(request, method), ip);
    }
    if (shdo == 1)
        destroy_tokens(conf);
    respond(sfd, request, get_token(conf, ROOT_DIR));
    log_response(get_token(conf, SERVER_NAME), ip, request);
}

static int check_headers(struct event_info *e)
{
    char *index = strstr(e->buff, "\r\n\r\n");
    if (index)
        return index - e->buff;
    return 0;
}

#if 0
static int read_event(int efd, int i, struct epoll_event ready_l[],
                      struct list *shutlist)
{
    struct event_info *e_i = ready_l[i].data.ptr;
    if (!e_i->buff)
        e_i->buff = malloc(8192);
    if (shdo == 1)
        free(e_i->buff);
    ssize_t nb_read = my_recv(e_i->fd, e_i->buff + e_i->b_read, 3000);
    if (!nb_read)
    {
        del_event(shutlist, e_i);
        return 0; // client disconnected
    }
    e_i->b_read += nb_read;
    int headers = check_headers(e_i);
    if (!headers) // we haven't gotten all the headers yet
        return 0;
    e_i->buff[headers] = '\0';
    char *req = malloc(headers + 1);
    if (shdo == 1)
        free(req);
    req = strcpy(req, e_i->buff);
    if (!e_i->request)
        e_i->request = mess_parse(req);
    if (!e_i->request)
        return 0; // no tokens
    if (shdo == 1)
        destroy_tokens(e_i->request);
    char *char_size = get_token(e_i->request, CONTENT_LENGTH)
        ? get_token(e_i->request, CONTENT_LENGTH)
        : "0";
    size_t b_size = 0;
    if (char_size)
        b_size = atoi(char_size);
    if (e_i->b_read - headers < b_size)
    {
        char *buff = malloc(b_size);
        get_body(e_i->fd, buff, 3000, b_size);
        free(buff);
    }
    ready_l[i].events = EPOLLOUT;
    epoll_ctl(efd, EPOLL_CTL_MOD, e_i->fd, &ready_l[i]);
    free(req);
    return 0;
}
static int create_event(int efd, int listening, int i,
                        struct epoll_event ready_l[])
{
    struct event_info *e_i = ready_l[i].data.ptr;
    ready_l[i].data.ptr = e_i;
    struct sockaddr client;
    socklen_t addr_size;
    int sock = accept(listening, &client, &addr_size);
    if (shdo == 1)
        close(sock);
    void *addr = &client;
    struct sockaddr_in *getip = addr;
    struct in_addr ipaddr = getip->sin_addr;
    char *ip = malloc(INET_ADDRSTRLEN);
    if (shdo == 1)
        free(ip);
    inet_ntop(AF_INET, &ipaddr, ip, INET_ADDRSTRLEN);
    e_i->ip = ip;
    if (sock == -1)
        return 0;
    make_socket_non_blocking(sock);
    ready_l[i].events = EPOLLIN;
    e_i->fd = sock;
    if (epoll_ctl(efd, EPOLL_CTL_ADD, sock, &ready_l[i]) == -1)
    {
        log_others("epoll ctl ADD failed");
        exit(1);
    }
    return 0;
}
#endif

static void handle_request(int listening, char *conf_file, int efd,
                           struct epoll_event ready_l[])
{
    struct token *c = NULL;
    struct list *shutlist = NULL;
    _shutdown();
    while (shdo != 1)
    {
        if (shdo == 1)
            close(listening);
        int waiting = epoll_wait(efd, ready_l, MAXEVENTS, -1);
        if (waiting == -1)
        {
            if (errno == EINTR)
                continue;
            log_others("epoll wait failed");
            destroy_tokens(conf);
            exit(1);
        }
        for (int i = 0; i < waiting; i++)
        {
            struct event_info *e_i = ready_l[i].data.ptr;
            if (e_i->fd == listening)
            {
                struct event_info *e_i = calloc(1, sizeof(struct event_info));
                shutlist = add_event(shutlist, e_i);
                if (shdo == 1)
                    destroy_events(shutlist);
                ready_l[i].data.ptr = e_i;
                struct sockaddr client;
                socklen_t addr_size;
                int sock = accept(listening, &client, &addr_size);
                if (shdo == 1)
                    close(sock);
                void *addr = &client;
                struct sockaddr_in *getip = addr;
                struct in_addr ipaddr = getip->sin_addr;
                char *ip = malloc(INET_ADDRSTRLEN);
                if (shdo == 1)
                    free(ip);
                inet_ntop(AF_INET, &ipaddr, ip, INET_ADDRSTRLEN);
                e_i->ip = ip;
                if (sock == -1)
                    continue;
                make_socket_non_blocking(sock);
                ready_l[i].events = EPOLLIN;
                e_i->fd = sock;
                if (epoll_ctl(efd, EPOLL_CTL_ADD, sock, &ready_l[i]) == -1)
                {
                    log_others("epoll ctl ADD failed");
                    exit(1);
                }
                //          if (create_event(efd, listening, i, ready_l))
                //                continue;
            }
            else if (ready_l[i].events == EPOLLIN)
            {
                struct event_info *e_i = ready_l[i].data.ptr;
                if (!e_i->buff)
                    e_i->buff = malloc(8192);
                if (shdo == 1)
                    free(e_i->buff);
                ssize_t nb_read =
                    my_recv(e_i->fd, e_i->buff + e_i->b_read, 3000);
                if (!nb_read)
                {
                    del_event(shutlist, e_i);
                    continue; // client disconnected
                }
                e_i->b_read += nb_read;
                int headers = check_headers(e_i);
                if (!headers) // we haven't gotten all the headers yet
                    continue;
                e_i->buff[headers] = '\0';
                char *req = malloc(headers + 1);
                if (shdo == 1)
                    free(req);
                req = strcpy(req, e_i->buff);
                if (!e_i->request)
                    e_i->request = mess_parse(req);
                if (!e_i->request)
                    continue; // no tokens
                if (shdo == 1)
                    destroy_tokens(e_i->request);
                char *char_size = get_token(e_i->request, CONTENT_LENGTH)
                    ? get_token(e_i->request, CONTENT_LENGTH)
                    : "0";
                size_t b_size = 0;
                if (char_size)
                    b_size = atoi(char_size);
                if (e_i->b_read - headers < b_size)
                {
                    char *buff = malloc(b_size);
                    get_body(e_i->fd, buff, 3000, b_size);
                    free(buff);
                }
                ready_l[i].events = EPOLLOUT;
                epoll_ctl(efd, EPOLL_CTL_MOD, e_i->fd, &ready_l[i]);
                free(req);
                //            if (read_event(efd, i, ready_l, shutlist))
                //              continue;
            }
            else if (ready_l[i].events == EPOLLOUT)
            {
                struct event_info *e_i = ready_l[i].data.ptr;
                if (shdo == 2)
                {
                    c = conf;
                    conf = conf_parse(conf_file);
                    set_up_log(get_token(conf, LOG), get_token(conf, LOG_FILE),
                               1);
                    shdo = 0;
                }
                handle_response(e_i->fd, conf, e_i->request, e_i->ip);
                destroy_tokens(e_i->request);
                e_i->request = NULL;
                del_event(shutlist, e_i);
            }
        }
    }
    destroy_tokens(c);
    destroy_events(shutlist);
}

int connect_server(struct token *config, char *conf_file)
{
    struct epoll_event ready_l[MAXEVENTS]; // Buffer where events
    conf = config;
    char *ip = get_token(conf, IP);
    char *port = get_token(conf, PORT);
    if (!ip || !port) // is it necessary?? no
        return -1; // invalid ip port err case
    log_others("preparing server"); // to log!
    int listening = create_and_bind(ip, port);
    if (listening == -1)
        return 2; // error cant listen idk?
    make_socket_non_blocking(listening);
    if (listen(listening, 10) == -1) // modify number to 100000!
        return 3; // error
    int efd = epoll_create1(0);
    if (efd == -1)
        return -1; // error case
    struct event_info *e_i = calloc(1, sizeof(struct event_info)); // when free?
    struct epoll_event server;
    server.data.ptr = e_i;
    e_i->fd = listening;
    server.data.ptr = e_i;
    server.events = EPOLLIN;
    int s = epoll_ctl(efd, EPOLL_CTL_ADD, listening, &server);
    if (s == -1)
        return -1; // error case
    handle_request(listening, conf_file, efd, ready_l);
    free(e_i);
    close(listening);
    return 0;
}
