# Makefile for project myfind
# -----------------------------

CC = g++
CFLAGS = -Wall -Werror -g -std=c++17 -pthread
INCDIR = include/
SRCDIR = src
OBJDIR = obj


all: myfind.o
	$(CC) $(CFLAGS) -o myfind myfind.o

main.o: myfind.cpp
	$(CC) $(CFLAGS) -c myfind.cpp -o myfind.o

clean:
	rm -f main myfind *.o

