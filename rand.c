#include <stdlib.h>
#include <time.h>

#include "balls.h"

float
randFloat(float lo, float hi) {
	float r, diff;
	static int isInitialized = 0;

	if (!isInitialized) { /* First call. */
		srand(time(0));
		isInitialized = 1;
	}

	r = (float) rand() / RAND_MAX;
	diff = hi - lo;
	return lo + r*diff;
}

Vector
randPtInRect(Rect r) {
	Vector pt = {
		randFloat(r.min.x, r.max.x),
		randFloat(r.min.y, r.max.y)
	};
	return pt;
}
