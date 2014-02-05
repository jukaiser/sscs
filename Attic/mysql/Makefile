CC = gcc
CFLAGS = -O3 `mysql_config --cflags`

all:	dummy

dummy:	dummy.o
	$(CC) `mysql_config --libs` -o dummy dummy.o

clean:
	rm dummy *.o
