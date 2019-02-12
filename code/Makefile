CC = g++
CFLAGS = -std=c++11 -Wall

server: Server.cpp
	$(CC) $(CFLAGS) -o -lpthread $@ Server.cpp

clean::
	rm -f server
