CC = g++
CFLAGS = -c -std=c++11
VALG = valgrind -v
PTHREAD = -pthread

all:part2

part2: part2.o
	$(CC) part2.o -o sortArrays $(PTHREAD) -std=c++11

part2.o: part2.cpp
	$(CC) $(CFLAGS) part2.cpp
clean:
	rm -rf *o sortArrays
