#NAME: Joel George
#EMAIL: joelgeorge03@gmail.com
#ID: 004786402

CC = gcc
CFLAGS = -Wall -Wextra -lmraa -lm 

default: lab4b.c
	$(CC) $(CFLAGS) lab4b.c -o lab4b 

check: default
	@-echo "START" >> command
	@-echo "STOP" >> command
	@-echo "PERIOD=5" >> command
	@-echo "START" >> command
	@-echo "OFF" >> command
	@-cat command | ./lab4b --log=test.txt
	@-rm command

clean:
	rm -rf *.o *.so *.tgz lab4b lab4b-004786402.tar.gz 

submission_files = README Makefile lab4b.c

dist: 
	tar -czf lab4b-004786402.tar.gz $(submission_files)

.PHONY: default clean dist check
