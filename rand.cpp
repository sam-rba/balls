#include "balls.h"

int
randInt(int lo, int hi) {
	return (rand() % (hi-lo)) + lo;
}

double
randDouble(double lo, double hi) {
	double r, diff;

	r = (double) rand() / (double) RAND_MAX;
	diff = hi - lo;
	return lo + r*diff;
}
