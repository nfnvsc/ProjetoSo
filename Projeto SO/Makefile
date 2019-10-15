# Makefile, versao 1
# Sistemas Operativos, DEI/IST/ULisboa 2019-20

CC   = gcc
LD   = gcc
CFLAGS =-Wall -std=gnu99 -pthread -I../
LDFLAGS=-lm -pthread
MUTEX = -DMUTEX
RWLOCK = -DRWLOCK

# A phony target is one that is not really the name of a file
# https://www.gnu.org/software/make/manual/html_node/Phony-Targets.html
.PHONY: all clean run

all: tecnicofs-nosync tecnicofs-mutex tecnicofs-rwlock

tecnicofs-nosync: lib/bst.o fs.o-nosync main.o-nosync
	$(LD) $(CFLAGS) $(LDFLAGS) -o tecnicofs-nosync lib/bst.o lib/hash.o fs.o main.o 

tecnicofs-mutex: lib/bst.o fs.o-mutex main.o-mutex
	$(LD) $(CFLAGS) $(LDFLAGS) $(MUTEX) -o tecnicofs-mutex lib/bst.o lib/hash.o fs.o main.o 

tecnicofs-rwlock: lib/bst.o fs.o-rwlock main.o-rwlock
	$(LD) $(CFLAGS) $(LDFLAGS) $(MUTEX) -o tecnicofs-rwlock lib/bst.o lib/hash.o fs.o main.o 

lib/bst.o: lib/bst.c lib/bst.h
	$(CC) $(CFLAGS) -o lib/bst.o -c lib/bst.c

lib/hash.o: lib/hash.c lib/hash.h
	$(CC) $(CFLAGS) -o lib/hash.o -c lib/hash.c

#NOSYNC
fs.o-nosync: fs.c fs.h lib/bst.h lib/hash.o
	$(CC) $(CFLAGS) -o fs.o -c fs.c
main.o-nosync: main.c fs.h lib/bst.h
	$(CC) $(CFLAGS) -o main.o -c main.c
#MUTEX
fs.o-mutex: fs.c fs.h lib/bst.h lib/hash.o
	$(CC) $(CFLAGS) $(MUTEX) -o fs.o -c fs.c
main.o-mutex: main.c fs.h lib/bst.h
	$(CC) $(CFLAGS) $(MUTEX) -o main.o -c main.c

#RWLOCK
fs.o-rwlock: fs.c fs.h lib/bst.h lib/hash.o
	$(CC) $(CFLAGS) $(RWLOCK) -o fs.o -c fs.c
main.o-rwlock: main.c fs.h lib/bst.h
	$(CC) $(CFLAGS) $(RWLOCK) -o main.o -c main.c


clean:
	@echo Cleaning...
	rm -f lib/*.o *.o tecnicofs-nosync tecnicofs-rwlock tecnicofs-mutex

run: tecnicofs
	./tecnicofs

