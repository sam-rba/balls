#include "balls.h"

double
randDouble(double lo, double hi) {
	double r, diff;
	static int isInitialized = 0;

	if (!isInitialized) { /* first call */
		srand(time(0));
		isInitialized = 1;
	}

	r = (double) rand() / (double) RAND_MAX;
	diff = hi - lo;
	return lo + r*diff;
}

Color
randColor(void) {
	Color color;

	color.r = randDouble(0, 1);
	color.g = randDouble(0, 1);
	color.b = randDouble(0, 1);
	return color;
}

Point
randPtInRect(Rectangle r) {
	return Pt(randDouble(r.min.x, r.max.x), randDouble(r.min.y, r.max.y));
}
