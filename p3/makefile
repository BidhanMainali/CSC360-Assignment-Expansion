
CC = gcc
CFLAGS = -std=c99 -D_DEFAULT_SOURCE -Wall -Wextra -g
COMMON = sfs_common.c
HEADERS = sfs_common.h

all: diskinfo disklist diskget diskput

diskinfo: diskinfo.c $(COMMON) $(HEADERS)
	$(CC) $(CFLAGS) -o diskinfo diskinfo.c $(COMMON)

disklist: disklist.c $(COMMON) $(HEADERS)
	$(CC) $(CFLAGS) -o disklist disklist.c $(COMMON)

diskget: diskget.c $(COMMON) $(HEADERS)
	$(CC) $(CFLAGS) -o diskget diskget.c $(COMMON)

diskput: diskput.c $(COMMON) $(HEADERS)
	$(CC) $(CFLAGS) -o diskput diskput.c $(COMMON)

clean:
	rm -f diskinfo disklist diskget diskput *.o
