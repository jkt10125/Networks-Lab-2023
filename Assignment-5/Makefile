# Makefile to build & install static library "libmsocket.a"
# Installation requires root permission

LIBPATH = /usr/local/lib
INCPATH = /usr/local/include

lib: mysocket.o
	@ echo Building library.......
	ar rcs -o libmsocket.a mysocket.o

mysocket.o: mysocket.c mysocket.h
	gcc -Wall -c mysocket.c

install:
	@ echo Installing library.......
	cp -p mysocket.h $(INCPATH)
	cp -p libmsocket.a $(LIBPATH)

clean:
	rm -f libmsocket.a mysocket.o