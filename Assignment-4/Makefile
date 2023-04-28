CC = gcc
all: MyOwnBrowser MyOwnHTTP
MyOwnBrowser: MyBrowser.o
	$(CC) MyBrowser.o -o MyOwnBrowser
MyOwnHTTP: MyHTTP.o
	$(CC) MyHTTP.o -o MyOwnHTTP
MyBrowser.o: MyBrowser.c
MyHTTP.o: MyHTTP.c
clean:
	rm *.o MyOwnBrowser MyOwnHTTP