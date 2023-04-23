CC=gcc
CFLAGS=-g -lssh -Wall

sftp:
	$(CC) $(CFLAGS) sftp.c -o sftp

clean:
	-rm sftp