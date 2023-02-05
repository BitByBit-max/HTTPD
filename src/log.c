#include "log.h"

#include "parsers.h"

static FILE *fd = NULL;

FILE *daemon_log(char *log_file)
{
    if (!strcmp(log_file, "std_out"))
        fd = fopen("HTTPd.log", "w+");
    else
        fd = fopen(log_file, "w+");
    return fd;
}

FILE *reg_log(char *log_file)
{
    if (strcmp(log_file, "std_out"))
        fd = fopen(log_file, "w+");
    else
        fd = stdout;
    return fd;
}

FILE *set_up_log(char *log, char *log_file, int is_daemon)
{
    if (fd)
        close_file();
    if (log)
        return is_daemon ? daemon_log(log_file) : reg_log(log_file);
    return fd;
}

void close_file(void)
{
    if (fd && fd != stdout)
    {
        fclose(fd);
        fd = NULL;
    }
}

void log_request(char *serv_name, char *req_type, char *target, char *client_ip)
{
    if (!fd)
        return;
    char *date = get_date();
    if (!req_type)
        fprintf(fd, "%s %s received %s from %s\n", date, serv_name, target,
                client_ip);
    else
        fprintf(fd, "%s %s received %s on '%s' from %s\n", date, serv_name,
                req_type, target, client_ip);
    free(date);
    fflush(fd);
}

void log_response(char *serv_name, char *client_ip, struct token *request)
{
    if (!fd)
        return;
    char *status = get_token(request, ERROR);
    if (!status)
        status = "200";
    char *date = get_date();
    if (!strcmp(status, "400"))
    {
        fprintf(fd, "%s %s responding with %s to %s\n", date, serv_name, status,
                client_ip);
        fflush(fd);
        free(date);
        return;
    }
    char *target = get_token(request, GET);
    char *method = "GET";
    if (!target)
    {
        target = get_token(request, HEAD);
        method = "HEAD";
        if (!target)
        {
            target = get_token(request, UNKNOWN);
            method = "UNKNOWN";
            status = "405";
        }
    }

    fprintf(fd, "%s %s responding with %s to %s for %s on '%s'\n", date,
            serv_name, status, client_ip, method, target);
    fflush(fd);
    free(date);
}

void log_others(char *s) //, void *var1, char *var2)
{
    if (!fd)
        return;
    fprintf(fd, "%s\n", s);
    fflush(fd);
}
