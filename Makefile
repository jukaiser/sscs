CC = gcc
CFLAGS = -O3

all:	qdemo

qdemo:	qdemo.o queue.o
	$(CC) $(CFLAGS) -o qdemo qdemo.o queue.o
