# Makefile to build & install static library "libmsocket.a"
# Installation requires root permission

LIBPATH = /usr/local/lib
INCPATH = /usr/local/include

libmsocket.a: mysocket.o
	@ echo
	@ echo Building library.......
	ar rcs -o libmsocket.a mysocket.o -lpthread

mysocket.o: mysocket.c mysocket.h
	gcc -Wall -c mysocket.c

all: libmsocket.a install clean

install:
	@ echo
	@ echo Installing library.......
	cp -p mysocket.h $(INCPATH)
	cp -p libmsocket.a $(LIBPATH)

test: server client

server: server.c libmsocket.a
	gcc -Wall -I. -L. server.c -o server -lmsocket

client: client.c libmsocket.a
	gcc -Wall -I. -L. client.c -o client -lmsocket

clean:
	@ echo
	@ echo Removing files.......
	rm -f libmsocket.a mysocket.o
	rm -f server.o server client.o client response.*