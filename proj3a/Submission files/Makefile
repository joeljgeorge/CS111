#NAME: Joel George
#EMAIL: joelgeorge03@gmail.com
#ID: 004786402

CC = gcc
CFLAGS = -Wall -Wextra -g 

default: lab3a.c
	$(CC) $(CFLAGS) lab3a.c -o lab3a 

clean:
	rm -rf *.o *.so *.tgz lab3a lab3a-004786402.tar.gz 

submission_files = README Makefile lab3a.c ext2_fs.h
dist: 
	tar -czf lab3a-004786402.tar.gz $(submission_files)

.PHONY: default clean dist
