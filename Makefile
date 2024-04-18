CC = g++
CFLAGS = -Wall -Wextra -std=c++17 -g

TARGETS = server subscriber

build: $(TARGETS)

server.o: Server.cpp Server.h
	$(CC) -c $(CFLAGS) Server.cpp

subscriber.o: Subscriber.cpp Subscriber.h
	$(CC) -c $(CFLAGS) Subscriber.cpp

server_main.o: server_main.cpp
	$(CC) -c $(CFLAGS) server_main.cpp

subscriber_main.o: subscriber_main.cpp
	$(CC) -c $(CFLAGS) subscriber_main.cpp

server: server.o server_main.o
	$(CC) $(CFLAGS) server.o server_main.o

subscriber: subscriber.o subscriber_main.o
	$(CC) $(CFLAGS) subscriber.o subscriber_main.o

clean:
	rm -f *.o $(TARGETS)

pack:
	zip -FSr 323CA_Zaharia_Marius-Tudor_Tema2.zip Makefile *.cpp *.h README

.PHONY:  build clean pack
