#include "response.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>

#include "parsers.h"

static char *get_status(char *err_code)
{
    if (!strcmp(err_code, "400"))
        return "HTTP/1.1 400 Bad Error\r\n";
    else if (!strcmp(err_code, "405"))
        return "HTTP/1.1 405 Method Not Allowed\r\n";
    else if (!strcmp(err_code, "403"))
        return "HTTP/1.1 403 Forbidden\r\n";
    else if (!strcmp(err_code, "404"))
        return "HTTP/1.1 404 Not Found\r\n";
    else if (!strcmp(err_code, "505"))
        return "HTTP/1.1 505 HTTP Version Not Supported\r\n";
    return NULL;
}

static char *get_target(struct token *request, char *root_dir)
{
    char *target = get_token(request, GET);
    if (!target)
        target = get_token(request, HEAD);
    if (!target)
        target = get_token(request, UNKNOWN);
    if (strlen(target) >= 7 && !strncmp("http://", target, 7))
        target += 7;
    else if (*target != '/')
        return NULL;
    char *tmp = NULL;
    char *theader = target;
    char *t1 = NULL;
    char *t2 = NULL;
    if (*target != '/')
    {
        strtok_r(target, "/", &tmp);
        if (tmp)
            t1 = my_concat("/", tmp);
        else
            theader = "/";
    }
    struct stat s;
    if (!stat(target, &s))
    {
        if (s.st_mode & S_IFDIR)
        {
            char *df = get_token(request, DEFAULT_FILE);
            if (!df)
                df = "index.html";
            if (t1)
            {
                t2 = my_concat(t1, df);
                free(t1);
            }
            else
                t2 = my_concat(theader, df);
        }
    }
    if (t2)
    {
        theader = my_concat(root_dir, t2);
        free(t2);
    }
    else
        theader = my_concat(root_dir, theader);
    return theader;
}

static char *get_time(void)
{
    char *gmt = "Date: ";
    char *date = get_date();
    gmt = my_concat(gmt, date);
    char *gmt2 = my_concat(gmt, "\r\n");
    free(gmt);
    free(date);
    return gmt2;
}

static void send_headers(int sfd, int len)
{
    char *date = get_time();
    send(sfd, date, strlen(date), 0);
    free(date);
    char *len_str = malloc(sizeof(char) * 500);
    len_str = my_itoa(len, len_str);
    char *content_len = "Content-Length: ";
    content_len = my_concat(content_len, len_str);
    char *content_len2 = content_len;
    content_len2 = my_concat(content_len2, "\r\n");
    free(len_str);
    send(sfd, content_len2, strlen(content_len2), 0);
    free(content_len);
    free(content_len2);
    char *cheader = "Connection: close\r\n"; // no need to change
    send(sfd, cheader, strlen(cheader), 0);
    send(sfd, "\r\n", 2, 0);
}

int respond(int sfd, struct token *request, char *root_dir)
{
    enum my_key_t e = get_token(request, ERROR) != NULL ? ERROR : UNKNOWN;
    char *e_val = get_token(request, e);
    struct token *err = NULL;
    if (e_val)
        err = init_err(e_val, e);
    char *target = get_target(request, root_dir);
    if (!target)
        bad_req(&err);
    struct stat s;
    int len = 0;
    errno = 0;
    if (!stat(target, &s))
        len = s.st_size;
    else
        bad_req(&err);
    if (errno == EACCES && !err)
        err = init_err("403", ERROR);
    char *status = "HTTP/1.1 200 OK\r\n";
    if (err)
        status = get_status(err->value);
    send(sfd, status, strlen(status), 0);
    send_headers(sfd, len);
    if (get_token(request, GET))
    {
        int fd = open(target, O_RDONLY);
        if (my_sendfile(sfd, fd, len) == -1)
            log_others("my_sendfile failed");
        close(fd);
    }
    free(target);
    return 0;
}
