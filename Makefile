CC=gcc
CFLAGS=-std=c99 -Wall -Wextra -Werror -Wvla -g -fsanitize=address -D_XOPEN_SOURCE=200809L
SRC=src/httpd.c src/annex.c src/log.c src/parsers.c src/response.c src/socket.c src/daemon.c
OBJ=${SRC:.c=.o}

all: httpd

httpd: $(OBJ)
	$(CC) -o httpd $(OBJ) $(CFLAGS)

%.o: %.c
	$(CC) -c $(CFLAGS) -o $@ $<

check: httpd
	python3 -m pytest

clean:
	$(RM) httpd $(OBJ)
	$(RM) src/.DS_Store .DS_Store
	$(RM) -r tests/__pycache__ .pytest_cache
