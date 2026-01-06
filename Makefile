PROG = server
OBJS = server.o init_socket.o handler_pars.o init_signal.o http.o http_curl.o conf_parse.o
CFLAGS = -Wall -Wextra -Werror -I. -I./include -g
CC = gcc

all:$(PROG)

$(PROG):$(OBJS)
	$(CC) $(OBJS) -o $@ -lcurl

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean

clean:
	rm -rf ${PROG} $(OBJS)
