#NAME: Joel George
#EMAIL: joelgeorge03@gmail.com
#ID: 004786402

CC = gcc
CFLAGS_TCP = -Wall -Wextra -lmraa -lm 
CFLAGS_TLS = -Wall -Wextra -lmraa -lm -lssl -lcrypto 

default: lab4c_tcp.c lab4c_tls.c
	$(CC) $(CFLAGS_TCP) lab4c_tcp.c -o lab4c_tcp
	$(CC) $(CFLAGS_TLS) lab4c_tls.c -o lab4c_tls 

clean:
	rm -rf *.o *.so *.tgz lab4c_tls lab4c_tcp lab4c-004786402.tar.gz 

submission_files = README Makefile lab4c_tcp.c lab4c_tls.c

dist: 
	tar -czf lab4c-004786402.tar.gz $(submission_files)

.PHONY: default clean dist
