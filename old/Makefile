# The program we wat to build and what it depends on and how to build it
hello: hello.o PDate.o
	gcc -o hello hello.o PDate.o
# hello.o depends o hello.c and its build by runnint the command gc-c hello.c
hello.o : hello.c
	gcc -c hello.c
# build PDate.o   
PDate.o : PDate.c
	gcc -c PDate.c
# What to do if make is run as:
# make clean
# remove all object and executables
clean:
	rm *.o hello
