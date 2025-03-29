## Makefile - MNIST KMeans Clustering
CPPFLAGS = -std=c++20 -Wall -Werror -pedantic -ggdb
PROGRAMS = hw5_extra_credit

all : $(PROGRAMS)

MNISTPixel.o : MNISTPixel.cpp MNISTPixel.h
	mpic++ $(CPPFLAGS) -c $< -o $@

MNISTKMeansMPI.o : MNISTKMeansMPI.h KMeansMPI.h MNISTPixel.h
	mpic++ $(CPPFLAGS) -c $< -o $@

hw5_extra_credit.o : hw5_extra_credit.cpp MNISTKMeansMPI.h MNISTPixel.h
	mpic++ $(CPPFLAGS) -c $< -o $@

hw5_extra_credit : hw5_extra_credit.o MNISTPixel.o
	mpic++ $(CPPFLAGS) $^ -o $@

run_hw5_ec : hw5_extra_credit
	mpirun -n 2 ./hw5_extra_credit

valgrind : hw5_extra_credit
	mpirun -n 2 valgrind --leak-check=full --show-leak-kinds=all ./hw5_extra_credit

clean :
	rm -f $(PROGRAMS) *.o *.html
