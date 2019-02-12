CC = g++
CFLAGS = -std=c++11 -Wall

server: Server.cpp
	$(CC) $(CFLAGS) -lpthread -o $@ Server.cpp

clean::
	rm -f server
