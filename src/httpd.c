#include "httpd.h"

#include <string.h>

#include "daemon.h"
#include "log.h"
#include "parsers.h"
#include "response.h"
#include "socket.h"

static char *next_arg(char *option)
{
    if (!strcmp(option, "start"))
        return "start";
    else if (!strcmp(option, "stop"))
        return "stop";
    else if (!strcmp(option, "reload"))
        return "reload";
    else if (!strcmp(option, "restart"))
        return "restart";
    return NULL;
}

int main(int argc, char *argv[])
{
    static struct token *conf = NULL;
    if (argc < 2 || argc > 5)
        err(1, "Invalid number of arguments");
    int is_daemon = 0;
    char *a_option = NULL;
    int i = 1;
    for (; i < argc - 1; i++)
    {
        if (!strcmp(argv[i], "--dry-run"))
        {
            conf = conf_parse(argv[argc - 1]);
            if (!conf)
                err(2, "Invalid configuration file");
            destroy_tokens(conf);
            return 0;
        }
        if (!strcmp(argv[i], "-a") && i < argc - 1)
        {
            if (a_option)
                err(1, "Invalid argument");
            a_option = next_arg(argv[i + 1]);
            if (!a_option)
                err(1, "-a option requires an action");
            is_daemon = 1;
            i++;
        }
        else
            err(1, "Invalid argument");
    }
    conf = conf_parse(argv[argc - 1]);
    if (!conf)
        err(2, "Invalid configuration file");
    FILE *fd = NULL;
    if (!(is_daemon && !strcmp(a_option, "reload")))
        fd = set_up_log(get_token(conf, LOG), get_token(conf, LOG_FILE),
                        is_daemon);
    if (is_daemon)
        return daemonize(a_option, conf, argv[argc - 1]);
    int server = connect_server(conf, argv[argc - 1]);
    if (server)
        return 1; // no Connection no should be we're done
    destroy_tokens(conf);
    if (fd && fd != stdout)
        fclose(fd);
    return 0;
}
