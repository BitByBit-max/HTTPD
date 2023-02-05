#ifndef LOG_H
#define LOG_H

#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "annex.h"
#include "parsers.h"

FILE *set_up_log(char *log, char *log_file, int is_daemon);
void log_request(char *serv_name, char *req_type, char *target,
                 char *client_ip);
void log_response(char *serv_name, char *client_ip, struct token *request);
void log_others(char *s); //, void *var1, char *var2);
void close_file(void);

#endif /* LOG_H */
