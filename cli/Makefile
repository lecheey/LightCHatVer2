CC = g++
SRC = main.cpp
TARGET = client

lightchatcli: $(SRC) lib
	$(CC) -o $(TARGET) $(SRC) -L. -lMyLib

lib: client.o func.o sha1.o
	ar rc libMyLib.a client.o func.o sha1.o

libclient: client.cpp client.h
	$(CC) -o client.o client.cpp -c

libfunc: func.cpp func.h
	$(CC) -o func.o func.cpp -c

libsha1: sha1.cpp sha1.h
	$(CC) -o sha1.o sha1.cpp -c

clear:
	rm *.o *.a
