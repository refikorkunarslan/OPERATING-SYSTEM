CC = gcc
CFLAGS = -c
VALG = valgrind -v

all:filesys

filesys: filesys.o
	$(CC) filesys.o -o fileSystemOper

filesys.o: filesys.c
	$(CC) $(CFLAGS) filesys.c
