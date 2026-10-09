CC = gcc

all: parallel memory psum

parallel:
	$(CC) parallel_min_max.c ../../lab3/src/utils.c ../../lab3/src/find_min_max.c -I../../lab3/src -o parallel

memory:
	$(CC) process_memory.c -o memory

sum.o: sum.c sum.h
	$(CC) -c sum.c -o sum.o

libsum.a: sum.o
	ar rcs libsum.a sum.o

psum: libsum.a
	$(CC) parallel_sum.c ../../lab3/src/utils.c -I../../lab3/src -L. -lsum -pthread -o psum

clean:
	rm -f parallel memory psum sum.o libsum.a