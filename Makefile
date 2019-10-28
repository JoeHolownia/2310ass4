CC = gcc
CFLAGS = -std=gnu99 -g -Wall -pedantic -pthread
DEPS = network.h linkedLists.h util.h channel.h messaging.h
OBJ = main.o network.o linkedLists.o util.o channel.o messaging.o

.PHONY: all clean
.DEFAULT_GOAL := all

all: 2310depot clean

2310depot: $(OBJ)
	$(CC) $(CFLAGS) -o 2310depot $(OBJ)

main.o: main.c $(DEPS)
	$(CC) $(CFLAGS) -c main.c

network.o: network.c network.h linkedLists.h channel.h messaging.h
	$(CC) $(CFLAGS) -c network.c

messaging.o: messaging.c messaging.h util.h linkedLists.h
	$(CC) $(CFLAGS) -c messaging.c

channel.o: channel.c channel.h
	$(CC) $(CFLAGS) -c channel.c

linkedLists.o: linkedLists.c linkedLists.h
	$(CC) $(CFLAGS) -c linkedLists.c

util.o: util.c util.h
	$(CC) $(CFLAGS) -c util.c

clean:
	rm *.o