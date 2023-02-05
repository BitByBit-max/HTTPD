#include "annex.h"

#include <errno.h>
#include <netdb.h>
#include <string.h>

int len(int value)
{
    int c = 0;
    while (value != 0)
    {
        value = value / 10;
        c++;
    }
    return c;
}

char *my_itoa(int value, char *s)
{
    if (value == 0)
    {
        s[0] = value + '0';
        s[1] = '\0';
    }
    else
    {
        int length = len(value);
        int div = 1;
        for (int i = 0; i < length - 1; i++)
        {
            div *= 10;
        }
        int i = 0;
        if (value < 0)
        {
            s[i] = '-';
            i++;
            value = -value;
        }
        length += i;
        for (; i < length; i++)
        {
            s[i] = (value / div) + '0';
            value = value % div;
            div /= 10;
        }
        s[i] = '\0';
    }
    return s;
}

char *my_concat(const char *s1, const char *s2)
{
    char *result = malloc(strlen(s1) + strlen(s2) + 1);
    if (result)
    {
        char *p = result;
        while (*s1)
            *p++ = *s1++;
        while ((*p++ = *s2++))
            continue;
    }
    return result;
}

int my_strcmp(char *s1, char *s2) // non-case-sensitive strcmp
{
    while ((*s1 != '\0') && (*s2 != '\0'))
    {
        char c1 = *s1;
        char c2 = *s2;
        if (('A' <= c1) && (c1 <= 'Z'))
            c1 += 32;
        if (('A' <= c2) && (c2 <= 'Z'))
            c2 += 32;
        if (c1 > c2)
            return 1;
        if (c1 < c2)
            return -1;
        s1++;
        s2++;
    }
    return *s1 - *s2;
}

char *get_date(void)
{
    char *date = malloc(29 * sizeof(char));
    size_t maxsize = 29;
    char *format = "%a, %d %b %Y %H:%M:%S";
    time_t t;
    struct tm *tmp;
    time(&t);
    tmp = localtime(&t);
    strftime(date, maxsize, format, tmp);
    char *gmt = date;
    gmt = my_concat(gmt, " GMT");
    free(date);
    return gmt;
}

int my_recv(int sfd, void *buff, ssize_t len)
{
    ssize_t tread = 0;
    ssize_t read = 0;
    while ((read = recv(sfd, buff, len - tread, 0)) > 0)
    {
        tread += read;
    }
    if (read == -1 && (errno == EWOULDBLOCK || errno == EAGAIN))
        return tread;
    if (read == 0)
        return 0;
    return tread;
}

int get_body(int sfd, void *buff, int len, int b_size)
{
    int tread = 0;
    int read = 0;
    while ((read = recv(sfd, buff, len - tread, 0)) > 0)
    {
        tread += read;
        if (tread == b_size)
            break;
    }
    return tread;
}

static int my_send(int sfd, void *buff, int len, int b_size)
{
    int tsent = 0;
    int sent = 0;
    while ((sent = send(sfd, buff, len - tsent, 0)) > 0)
    {
        tsent += sent;
        if (tsent == b_size)
            break;
    }
    return tsent;
}

int my_sendfile(int sfd, int fd, int len)
{
    char *buff = malloc(sizeof(char) * len);
    int err = read(fd, buff, len); // check max cap of read!
    if (err == -1)
    {
        free(buff);
        return -1;
    }
    my_send(sfd, buff, err, len);
    free(buff);
    return 0;
}
