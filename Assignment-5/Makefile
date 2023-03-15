# Makefile to build & install static library "libmsocket.a"
# Installation requires root permission

LIBPATH = /usr/local/lib
INCPATH = /usr/local/include

test: library server client

all: library install

library: mysocket.o
	@ echo
	@ echo Building library.......
	ar rcs -o libmsocket.a mysocket.o

mysocket.o: mysocket.c mysocket.h
	gcc -Wall -c mysocket.c

server: server.c libmsocket.a
	gcc -Wall -L. server.c -o server -lmsocket

client: client.c libmsocket.a
	gcc -Wall -L. client.c -o client -lmsocket

install:
	@ echo
	@ echo Installing library.......
	cp -p mysocket.h $(INCPATH)
	cp -p libmsocket.a $(LIBPATH)

clean:
	@ echo
	@ echo Removing files.......
	rm -f libmsocket.a mysocket.o
	rm -f server.o server client.o client response.*