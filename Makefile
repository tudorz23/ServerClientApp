CC = g++
CFLAGS = -Wall -Wextra -std=c++17 -g

TARGETS = server subscriber

all: $(TARGETS)

protocols.o: protocols.cpp
	$(CC) -c $(CFLAGS) protocols.cpp -o protocols.o

server.o: Server.cpp
	$(CC) -c $(CFLAGS) Server.cpp -o server.o

subscriber.o: Subscriber.cpp
	$(CC) -c $(CFLAGS) Subscriber.cpp -o subscriber.o

server_main.o: server_main.cpp
	$(CC) -c $(CFLAGS) server_main.cpp -o server_main.o

subscriber_main.o: subscriber_main.cpp
	$(CC) -c $(CFLAGS) subscriber_main.cpp -o subscriber_main.o

server: server.o server_main.o protocols.o
	$(CC) $(CFLAGS) server.o server_main.o protocols.o -o server

subscriber: subscriber.o subscriber_main.o protocols.o
	$(CC) $(CFLAGS) subscriber.o subscriber_main.o protocols.o -o subscriber

clean:
	rm -f *.o $(TARGETS)

pack:
	zip -FSr 323CA_Zaharia_Marius-Tudor_Tema2.zip Makefile *.cpp *.h README

.PHONY: all clean pack
