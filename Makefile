# Makefile for project myfind
# -----------------------------

CC = g++
CFLAGS = -Wall -Werror -g -std=c++17
INCDIR = include/
SRCDIR = src
OBJDIR = obj


all: main.o
	$(CC) $(CFLAGS) -o main $(OBJDIR)/main.o

main.o: main.cpp
	$(CC) $(CFLAGS) -c main.cpp -o $(OBJDIR)/main.o

clean:
	rm -f main $(OBJDIR)/*.o *.o

