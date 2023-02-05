#include "parsers.h"

#include <string.h>

#include "annex.h"

static enum my_key_t get_key(char *key)
{
    if (!my_strcmp(key, "log_file"))
        return LOG_FILE;
    else if (!my_strcmp(key, "log"))
        return LOG;
    else if (!my_strcmp(key, "pid_file"))
        return PID_FILE;
    else if (!my_strcmp(key, "server_name"))
        return SERVER_NAME;
    else if (!my_strcmp(key, "port"))
        return PORT;
    else if (!my_strcmp(key, "ip"))
        return IP;
    else if (!my_strcmp(key, "default_file"))
        return DEFAULT_FILE;
    else if (!my_strcmp(key, "root_dir"))
        return ROOT_DIR;
    else if (!my_strcmp(key, "Host"))
        return HOST;
    else if (!my_strcmp(key, "Date"))
        return DATE;
    else if (!my_strcmp(key, "Content-Length"))
        return CONTENT_LENGTH;
    else if (!my_strcmp(key, "Body"))
        return BODY;
    else if (!my_strcmp(key, "GET"))
        return GET;
    else if (!my_strcmp(key, "HEAD"))
        return HEAD;
    else if (!my_strcmp(key, "UNKNOWN"))
        return UNKNOWN;
    else if (!my_strcmp(key, "client_ip"))
        return CLIENT_IP;
    return CONNECTION;
}

struct token *add_token(struct token *tokens, char *key, char *value,
                        enum tag_t tag)
{
    struct token *add = malloc(sizeof(struct token));
    if (!add)
        return tokens;
    add->key = get_key(key);
    add->tag = tag;
    add->next = tokens;
    if (add->key == LOG && !strcmp(value, "false")) // NULL means no logging
        add->value = NULL;
    else
        add->value = value;
    return add;
}

static struct token *default_file_check(struct token *tokens)
{
    struct token *t = tokens;
    while (t)
    {
        if (t->key == DEFAULT_FILE)
            break;
        t = t->next;
    }
    if (t)
        return tokens;
    // no it is a 404 not found
    return add_token(tokens, "default_file", strdup("index.html"), VHOSTS);
}

static struct token *log_check(struct token *tokens)
{
    struct token *t = tokens;
    while (t)
    {
        if (t->key == LOG)
            break;
        t = t->next;
    }
    if (t)
        return tokens;
    return add_token(tokens, "log", strdup("true"), VHOSTS);
}

static struct token *log_file_check(struct token *tokens)
{
    struct token *t = tokens;
    while (t)
    {
        if (t->key == LOG_FILE)
            break;
        t = t->next;
    }
    if (t)
        return tokens;
    return add_token(tokens, "log_file", strdup("std_out"), VHOSTS);
}

static int conf_check(struct token *tokens)
{
    struct token *t = tokens;
    int mnd_count = 0;
    while (t)
    {
        if (t->key < 5)
            mnd_count++;
        t = t->next;
    }
    return mnd_count == 5;
}

void destroy_tokens(struct token *tokens)
{
    struct token *curr = tokens;
    while (tokens)
    {
        tokens = tokens->next;
        if (curr->value)
            free(curr->value);
        if (curr)
            free(curr);
        curr = tokens;
    }
}

char *get_token(struct token *tokens, enum my_key_t key)
{
    struct token *t = tokens;
    while (t)
    {
        if (t->key == key)
            return t->value;
        t = t->next;
    }
    return NULL;
}

struct token *conf_parse(char *path)
{
    FILE *f = fopen(path, "r");
    if (!f)
        return NULL; // config file does not exit
    size_t size = 0;
    char *line = NULL;
    getline(&line, &size, f); // reads GLOBAL
    struct token *tokens = NULL;
    while (getline(&line, &size, f) != -1 && strcmp(line, "\n"))
    {
        char *key = strtok(line, " =\n");
        char *value = strdup(strtok(NULL, " =\n"));
        tokens = add_token(tokens, key, value, GLOBAL);
        if (!tokens)
        {
            fclose(f);
            return NULL;
        }
    }
    getline(&line, &size, f); // reads VHOSTS
    while (getline(&line, &size, f) != -1 && strcmp(line, "\n"))
    {
        char *key = strtok(line, " =\n");
        char *value = strdup(strtok(NULL, " =\n"));
        tokens = add_token(tokens, key, value, VHOSTS);
        if (!tokens)
        {
            fclose(f);
            return NULL;
        }
    }
    if (!conf_check(tokens))
    {
        destroy_tokens(tokens);
        fclose(f);
        return NULL;
    }
    tokens = default_file_check(tokens);
    tokens = log_check(tokens);
    tokens = log_file_check(tokens);
    fclose(f);
    return tokens;
}

static int check_key(char *key)
{
    return !my_strcmp(key, "Host") || !my_strcmp(key, "Content-Length")
        || !my_strcmp(key, "Connection");
}

struct token *init_err(char *err, enum my_key_t key)
{
    struct token *t = malloc(sizeof(struct token));
    t->key = key;
    t->value = strdup(err);
    t->tag = HTTP_MESSAGE;
    return t;
}

void bad_req(struct token **err)
{
    if (*err && strcmp((*err)->value, "400"))
    {
        free((*err)->value);
        (*err)->value = strdup("400");
    }
    else
        *err = init_err("400", ERROR);
}

struct token *exit_t(struct token *err, struct token *tokens)
{
    if (err)
    {
        err->next = tokens;
        return err;
    }
    return tokens;
}

struct token *mess_parse(char *request)
{
    struct token *tokens = NULL;
    struct token *err = NULL;
    char *curr = NULL;
    char *temp = NULL;
    curr = strtok_r(request, "\r\n", &temp);
    if (!request || !temp)
        return NULL;
    char *method = strtok(request, " ");
    if (strcmp(method, "GET") && strcmp(method, "HEAD"))
    {
        err = init_err("405", ERROR);
        method = "UNKNOWN";
    }
    char *target = strdup(strtok(NULL, " "));
    char *httpv = strtok(NULL, " ");
    if (strncmp(httpv, "HTTP/", 5))
        bad_req(&err);
    else if (strcmp(httpv, "HTTP/1.1") && !err)
        err = init_err("505", ERROR);
    tokens = add_token(tokens, method, target, HTTP_MESSAGE);
    enum my_key_t met = tokens->key;
    while ((curr = strtok_r(NULL, "\r\n", &temp)) && temp)
    {
        char *key = strtok(curr, " :");
        if (!check_key(key))
            continue;
        char *value = strdup(strtok(NULL, "\r\n"));
        if (!my_strcmp(key, "content-length"))
        {
            int len = atoi(value);
            if (strcmp(value, "0") && len <= 0)
                bad_req(&err);
        }
        if (get_token(tokens, get_key(key)))
            bad_req(&err);
        tokens = add_token(tokens, key, value, HTTP_MESSAGE);
        if (!tokens)
            return NULL;
    }
    if (met == HEAD)
        return exit_t(err, tokens);
    temp++;
    if (!get_token(tokens, HOST))
        bad_req(&err);
    return exit_t(err, tokens);
}
