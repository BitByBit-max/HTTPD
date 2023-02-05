#ifndef DAEMON_H
#define DAEMON_H

#include <errno.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "log.h"
#include "parsers.h"
#include "socket.h"

int daemonize(char *option, struct token *conf, char *conf_file);

#endif /* ! DAEMON_H*/
