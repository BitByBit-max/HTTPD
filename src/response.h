#ifndef RESPONSE_H
#define RESPONSE_H

#include <errno.h>
#include <fcntl.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <time.h>
#include <unistd.h>

#include "annex.h"
#include "log.h"
#include "parsers.h"

int respond(int sfd, struct token *request, char *root_dir);

#endif /* ! RESPONSE_H */
