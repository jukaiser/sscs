CC = gcc
CFLAGS = -O3 `mysql_config --cflags`
LIBS = `mysql_config --libs`

all:	qdemo pdemo dbdemo cdemo

qdemo:	qdemo.o queue.o config.o
	$(CC) $(CFLAGS) $(LIBS) -o qdemo qdemo.o queue.o config.o

pdemo:	pdemo.o pattern.o config.o
	$(CC) $(CFLAGS) $(LIBS) -o pdemo pdemo.o pattern.o config.o

dbdemo: database.o pattern.o dbdemo.o config.o
	$(CC) $(CFLAGS) $(LIBS) -o dbdemo database.o pattern.o dbdemo.o config.o

cdemo:	cdemo.o config.o
	$(CC) $(CFLAGS) $(LIBS) -o cdemo cdemo.o config.o

clean:
	rm qdemo pdemo dbdemo *.o core
