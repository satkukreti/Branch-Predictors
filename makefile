all: predictors

predictors: predictors.o
	g++ predictors.o -o predictors

predictors.o:
	g++ -c predictors.cpp

clean:
	rm -f *.o predictors
