CC = gcc
CFLAGS = -Wall
phony: MyOwnBrowser MyOwnHTTP
MyOwnBrowser: MyBrowser.o
	$(CC) MyBrowser.o -o MyOwnBrowser
MyOwnHTTP: MyHTTP.o
	$(CC) MyHTTP.o -o MyOwnHTTP
MyBrowser.o: MyBrowser.c
MyHTTP.o: MyHTTP.c
clean:
	rm *.o MyOwnBrowser MyOwnHTTP

GET http://127.0.0.1/index.html:20000