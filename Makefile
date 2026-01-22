PROG = minh-server
OBJS = main.o parameter.o socket.o protocol.o server.o process.o signals.o procname_common.o protocol/http/http.o log/log.o
CFLAGS = -Wall -Wextra -Werror -I. -I./include -I./log -I./protocol -I./protocol/http -I./protocol/websocket -g
CC = gcc

all:$(PROG)

$(PROG):$(OBJS)
	$(CC) $(OBJS) -o $@ -lpthread

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean

clean:
	rm -rf ${PROG} $(OBJS)
