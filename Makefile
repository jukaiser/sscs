CC = gcc
CFLAGS = -O3

all:	qdemo pdemo

qdemo:	qdemo.o queue.o
	$(CC) $(CFLAGS) -o qdemo qdemo.o queue.o

pdemo:	pdemo.o pattern.o
	$(CC) $(CFLAGS) -o pdemo pdemo.o pattern.o

clean:
	rm qdemo pdemo *.o
