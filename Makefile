CC = gcc
CFLAGS = -O3 `mysql_config --cflags`
LIBS = `mysql_config --libs`

all:	sscs mk_ships

sscs:	main.o pattern.o config.o database.o queue.o guided_ptr.o
	$(CC) $(CFLAGS) $(LIBS) -o sscs main.o pattern.o config.o database.o queue.o guided_ptr.o

mk_ships:	mk_ships.o pattern.o config.o database.o
	$(CC) $(CFLAGS) $(LIBS) -o mk_ships mk_ships.o pattern.o config.o database.o

clean:
	rm mk_ships sscs *.o core test/core
