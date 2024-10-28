#include <stdlib.h>
#include <time.h>

#include "balls.h"

static float randFloat(float lo, float hi);

float2
randPtInRect(Rectangle r) {
	float2 pt = {
		randFloat(r.min[0], r.max[0]),
		randFloat(r.min[1], r.max[1])
	};
	return pt;
}

float2
randVec(float xmin, float xmax, float ymin, float ymax) {
	float2 v = {randFloat(xmin, xmax), randFloat(ymin, ymax)};
	return v;
}

static float
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
