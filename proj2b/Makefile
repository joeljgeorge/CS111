#NAME: Joel George
#EMAIL: joelgeorge03@gmail.com
#ID: 004786402

CC = gcc
CFLAGS = -g -Wall -Wextra -lpthread -lprofiler 

default: SortedList.c lab2_list.c
	$(CC) $(CFLAGS) lab2_list.c SortedList.c -o lab2_list

tests:  default
	@-bash data_collect_list_2b.sh 

profile: default
	@-LD_PRELOAD=/usr/lib64/libprofiler.so CPUPROFILE=./raw.gperf ./lab2_list --threads=12 --iterations=1000 --sync=s
	@-pprof --text ./lab2_list ./raw.gperf > profile.out
	@-pprof --list=thread_function ./lab2_list ./raw.gperf >> profile.out

graphs: clean tests 
	@-gnuplot lab2_list.gp

clean:
	rm -rf *.o *.so *.tgz lab2_list lab2a-004786402.tar.gz 

submission_files = README Makefile lab2_list.c lab2_list.gp lab2b_list.csv lab2b_1.png lab2b_2.png lab2b_3.png lab2b_4.png lab2b_5.png SortedList.c SortedList.h data_collect_list_2b.sh profile.out

dist: graphs
	tar -czf lab2b-004786402.tar.gz $(submission_files)

.PHONY: default clean dist tests graphs
