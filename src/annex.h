#ifndef ANNEX_H
#define ANNEX_H

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/uio.h>
#include <time.h>
#include <unistd.h>

char *my_itoa(int value, char *s);
int my_recv(int sfd, void *buff, ssize_t len);
int get_body(int sfd, void *buff, int len, int b_size);
char *my_concat(const char *s1, const char *s2);
int my_strcmp(char *s1, char *s2);
char *get_date(void);
int my_sendfile(int sfd, int fd, int len);

#endif /* !ANNEX_H */
