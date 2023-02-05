#include "daemon.h"

#include <errno.h>
#include <stddef.h>
#include <stdio.h>

#include "log.h"
#include "parsers.h"

static void reload_process(char *pid_file)
{
    FILE *f = fopen(pid_file, "r");
    if (!f)
        return;
    char *line = NULL;
    size_t size = 0;
    ssize_t s = getline(&line, &size, f);
    if (s > 0)
    {
        int pid = atoi(line);
        if (pid > 0)
        {
            if (!kill(pid, 0))
                kill(pid, SIGUSR1);
        }
        free(line);
    }
    fclose(f);
}

static void check_process(char *pid_file)
{
    FILE *f = fopen(pid_file, "r");
    char *line = NULL;
    size_t size = 0;
    ssize_t s = getline(&line, &size, f);
    if (s > 0)
    {
        int pid = atoi(line);
        if (pid > 0)
        {
            if (!kill(pid, 0))
            {
                log_others("OOPS! Another process is alive");
                fclose(f);
                exit(1);
            }
        }
        free(line);
    }
    fclose(f);
}

static int start_daemon(struct token *conf, char *conf_file)
{
    char *pid_file = get_token(conf, PID_FILE);
    struct stat s;
    if (!(stat(pid_file, &s) == -1 && errno == ENOENT))
        check_process(pid_file);
    FILE *f = fopen(pid_file, "w");
    pid_t pid = fork();
    if (pid == -1)
    {
        fclose(f);
        log_others("Fork failed");
        return 1;
    }
    if (!pid)
    {
        fclose(f);
        int server = connect_server(conf, conf_file);
        if (server)
            return 1; // no Connection
    }
    if (pid)
        fprintf(f, "%d\n", pid);
    destroy_tokens(conf);
    fclose(f);
    return 0;
}

static int stop_daemon(struct token *conf, int re)
{
    FILE *f = NULL;
    char *pid_file = get_token(conf, PID_FILE);
    struct stat s;
    if (!stat(pid_file, &s))
    {
        f = fopen(pid_file, "r");
        char *line = NULL;
        size_t size = 0;
        ssize_t s = getline(&line, &size, f);
        if (s > 0)
        {
            int pid = atoi(line);
            if (pid > 0)
                kill(pid, SIGTERM);
        }
        free(line);
    }
    if (f)
        fclose(f);
    remove(pid_file);
    if (!re)
    {
        close_file();
        destroy_tokens(conf);
    }
    return 0;
}

static int reload_daemon(struct token *conf)
{
    char *pid_file = get_token(conf, PID_FILE);
    if (pid_file)
    {
        struct stat s;
        if (!stat(pid_file, &s))
            reload_process(pid_file);
    }
    destroy_tokens(conf);
    return 0;
}

static int restart_daemon(struct token *conf, char *conf_file)
{
    stop_daemon(conf, 1);
    return start_daemon(conf, conf_file);
}

int daemonize(char *option, struct token *conf, char *conf_file)
{
    if (!strcmp(option, "start"))
        return start_daemon(conf, conf_file);
    else if (!strcmp(option, "stop"))
        return stop_daemon(conf, 0);
    else if (!strcmp(option, "reload"))
        return reload_daemon(conf);
    else if (!strcmp(option, "restart"))
        return restart_daemon(conf, conf_file);
    return 1;
}
