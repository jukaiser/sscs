CC = gcc
CFLAGS = -O3 `mysql_config --cflags`
LIBS = `mysql_config --libs`

all:	qdemo pdemo dbdemo

qdemo:	qdemo.o queue.o
	$(CC) $(CFLAGS) $(LIBS) -o qdemo qdemo.o queue.o

pdemo:	pdemo.o pattern.o
	$(CC) $(CFLAGS) $(LIBS) -o pdemo pdemo.o pattern.o

dbdemo: database.o pattern.o dbdemo.o
	$(CC) $(CFLAGS) $(LIBS) -o dbdemo database.o pattern.o dbdemo.o

clean:
	rm qdemo pdemo dbdemo *.o core
