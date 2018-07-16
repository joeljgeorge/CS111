#! /usr/bin/gnuplot
#
# purpose:
#	 generate data reduction graphs for the multi-threaded list project
#
# input: lab2_list.csv
#	1. test name
#	2. # threads
#	3. # iterations per thread
#	4. # lists
#	5. # operations performed (threads x iterations x (ins + lookup + delete))
#	6. run time (ns)
#	7. run time per operation (ns)
#	8. average wait-for-lock opeartion time (ns)
#
# output:
#	lab2_list-1.png ... cost per operation vs threads and iterations
#	lab2_list-2.png ... threads and iterations that run (un-protected) w/o failure
#	lab2_list-3.png ... threads and iterations that run (protected) w/o failure
#	lab2_list-4.png ... cost per operation vs number of threads
#
# Note:
#	Managing data is simplified by keeping all of the results in a single
#	file.  But this means that the individual graphing commands have to
#	grep to select only the data they want.
#
#	Early in your implementation, you will not have data for all of the
#	tests, and the later sections may generate errors for missing data.
#

# general plot parameters
set terminal png
set datafile separator ","

# how many threads/iterations we can run without failure (w/o yielding)
set title "List-1: Throughput vs Time per Operation"
set xlabel "# of Threads"
set logscale x 10
set ylabel "Throughput"
set logscale y 10
set output 'lab2b_1.png'

# grep out only single threaded, un-protected, non-yield results
plot \
     "< grep 'list-none-m,' lab2b_list.csv" using ($2):(1000000000/($7)) \
	title 'mutex' with points lc rgb 'red', \
     "< grep 'list-none-s,' lab2b_list.csv" using ($2):(1000000000)/(($7)) \
	title 'spin locks' with points lc rgb 'green'


set title "List-2: Time for completion vs. Thread Count"
set xlabel "Threads"
set logscale x 2
set xrange [0.75:]
set ylabel "Time (ns)"
set logscale y 10
set output 'lab2b_2.png'
# note that unsuccessful runs should have produced no output
plot \
     "< grep -e 'list-none-m,[0-9]*,1000' lab2b_list.csv" using ($2):($8) \
	title 'wait-for-lock time' with points lc rgb 'green', \
     "<grep -e 'list-none-m,[0-9]*,1000' lab2b_list.csv" using ($2):($7) \
        title 'Avg. time per op' with points lc rgb 'red'

set title "List-3: Successful Iterations vs. Thread Count"
unset logscale x
set xrange [0:14]
set xlabel "Thread counts"
set ylabel "Successful iterations"
set logscale y 10
set output 'lab2b_3.png'

plot \
    "< grep 'list-id-none,1' lab2b_list.csv" using (1):($3) \
	with points lc rgb "red" title "unprotected", \
    "< grep 'list-id-none,2,' lab2b_list.csv" using (2):($3) \
	with points lc rgb "red" title "", \
    "< grep 'list-id-none,4,' lab2b_list.csv" using (4):($3) \
	with points lc rgb "red" title "", \
    "< grep 'list-id-none,8,' lab2b_list.csv" using (8):($3) \
	with points lc rgb "red" title "", \
    "< grep 'list-id-none,12,' lab2b_list.csv" using (12):($3) \
        with points lc rgb "red" title "", \
    "< grep 'list-id-none,16,' lab2b_list.csv" using (16):($3) \
        with points lc rgb "red" title "", \
    "< grep 'list-id-s,1' lab2b_list.csv" using (1):($3) \
        with points lc rgb "blue" title "sync=s", \
    "< grep 'list-id-s,2,' lab2b_list.csv" using (2):($3) \
        with points lc rgb "blue" title "", \
    "< grep 'list-id-s,4,' lab2b_list.csv" using (4):($3) \
        with points lc rgb "blue" title "", \
    "< grep 'list-id-s,8,' lab2b_list.csv" using (8):($3) \
        with points lc rgb "blue" title "", \
    "< grep 'list-id-s,12,' lab2b_list.csv" using (12):($3) \
        with points lc rgb "blue" title "", \
    "< grep 'list-id-s,16,' lab2b_list.csv" using (16):($3) \
        with points lc rgb "blue" title "", \
     "< grep 'list-id-m,1' lab2b_list.csv" using (1):($3) \
        with points lc rgb "green" title "sync=m", \
    "< grep 'list-id-m,2,' lab2b_list.csv" using (2):($3) \
        with points lc rgb "green" title "", \
    "< grep 'list-id-m,4,' lab2b_list.csv" using (4):($3) \
        with points lc rgb "green" title "", \
    "< grep 'list-id-m,8,' lab2b_list.csv" using (8):($3) \
        with points lc rgb "green" title "", \
    "< grep 'list-id-m,12,' lab2b_list.csv" using (12):($3) \
        with points lc rgb "green" title "", \
    "< grep 'list-id-m,16,' lab2b_list.csv" using (16):($3) \
        with points lc rgb "green" title ""

# "no valid points" is possible if even a single iteration can't run
#

# unset the kinky x axis
unset xtics
set xtics

set title "List-4: Mutex Scalability"
set xlabel "Threads"
set logscale x 2
unset xrange
set xrange [0.75:]
set ylabel "Throughput"
set logscale y
set output 'lab2b_4.png'
set key left top
plot \
     "< grep -e 'list-none-m,[0-9]*,1000,' lab2b_list.csv" using ($2):(1000000000/($7)) \
     with points lc rgb 'blue' title '

unset xtics
set xtics

set title "List-5: Spin locks scalability"
set xlabel "Threads"
set logscale x 2
unset xrange
set xrange [0.75:]
set ylabel "Throughput"
set logscale y
set output 'lab2b_5.png'
set key left top
plot \
     "< grep -e 'list-none-s,[0-9]*,1000,' lab2b_list.csv" using ($2):(1000000000\
/($7)) \
        title 'list w/mutex' with points lc rgb 'blue'
