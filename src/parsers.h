#ifndef PARSERS_H
#define PARSERS_H

#include <err.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>

enum tag_t
{
    GLOBAL,
    VHOSTS,
    HTTP_MESSAGE,
};

enum my_key_t
{
    PID_FILE,
    SERVER_NAME,
    PORT,
    IP,
    ROOT_DIR,
    CLIENT_IP,
    LOG_FILE,
    LOG,
    DEFAULT_FILE,
    DATE,
    HOST,
    CONNECTION,
    CONTENT_LENGTH,
    BODY,
    GET,
    HEAD,
    UNKNOWN,
    ERROR,
};

struct token
{
    enum tag_t tag;
    enum my_key_t key;
    char *value;
    struct token *next;
};

struct token *add_token(struct token *tokens, char *key, char *value,
                        enum tag_t tag);
struct token *init_err(char *err, enum my_key_t key);
void bad_req(struct token **err);
struct token *conf_parse(char *path);
struct token *mess_parse(char *request);
struct token *exit_t(struct token *err, struct token *tokens);
char *get_token(struct token *tokens, enum my_key_t key);
void destroy_tokens(struct token *tokens);

#endif /* ! PARSERS_H */
