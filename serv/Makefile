CC = g++
SRC = main.cpp
SQLPATH = -L/usr/lib/mysql -lmysqlclient
TARGET = server

lightchatserver: $(SRC) lib
	$(CC) -o $(TARGET) $(SQLPATH) $(SRC) -L. -lMyLib

lib: server.o func.o
	ar rc libMyLib.a server.o func.o

libserver: server.cpp server.h
	$(CC) -o server.o server.cpp -c

libfunc: func.cpp func.h
	$(CC) -o func.o func.cpp -c

clear:
	rm *.o *.a
