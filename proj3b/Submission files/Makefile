#NAME: Joel George, David Feng
#EMAIL: joelgeorge03@gmail.com, david.x.feng@ucla.edu
#ID: 004786402, 604756204

default:
	rm -rf lab3b
	echo '#!/usr/local/cs/bin/python3' > lab3b
	cat lab3b.py >> lab3b
	chmod +x lab3b

clean:
	rm -rf *.o *.so *.tgz lab3b lab3b-004786402.tar.gz 

submission_files = README Makefile lab3b.py

dist: 
	tar -czf lab3b-004786402.tar.gz $(submission_files)

.PHONY: default clean dist
