#NAME: Joel George
#EMAIL: joelgeorge03@gmail.com
#ID: 004786402

CC = gcc
CFLAGS = -g -Wall -Wextra

default: lab1a.c
	$(CC) $(CFLAGS) lab1a.c -o lab1a

clean:
	rm -f *.o *.so *.tgz lab1a lab1a-004786402.tar.gz

submission_files = README Makefile lab1a.c

dist:
	tar -czf lab1a-004786402.tar.gz $(submission_files)

.PHONY: default clean dist
