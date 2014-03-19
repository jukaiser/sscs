CC = gcc
#CFLAGS = -O3 `mysql_config --cflags`
#LIBS = `mysql_config --libs`
CFLAGS = -O3 -I/usr/include/mysql -fmessage-length=0 -D_FORTIFY_SOURCE=2 -fstack-protector -funwind-tables -fasynchronous-unwind-tables -g -DNO_PROFILING -DPIC -fPIC -fno-strict-aliasing -g
#CFLAGS = -O3 -I/usr/include/mysql -fmessage-length=0 -D_FORTIFY_SOURCE=2 -fstack-protector -funwind-tables -fasynchronous-unwind-tables -g -DPIC -fPIC -fno-strict-aliasing -g
LIBS = -L/usr/lib64 -lmysqlclient -lpthread -lz -lm -lssl -lcrypto -ldl


all:	sscs mk_ships show_reaction recipe

sscs:	main.o pattern.o config.o database.o queue.o reaction.o profile.o
	$(CC) $(CFLAGS) $(LIBS) -o sscs main.o pattern.o config.o database.o queue.o reaction.o profile.o

recipe:	recipe.o pattern.o config.o database.o profile.o
	$(CC) $(CFLAGS) $(LIBS) -o recipe recipe.o pattern.o config.o database.o profile.o

show_reaction:	show.o pattern.o config.o database.o profile.o
	$(CC) $(CFLAGS) $(LIBS) -o show_reaction show.o pattern.o config.o database.o profile.o

mk_ships:	mk_ships.o pattern.o config.o profile.o
	$(CC) $(CFLAGS) $(LIBS) -o mk_ships mk_ships.o pattern.o config.o profile.o

clean:
	rm show_reaction mk_ships sscs recipe *.o core test/core
