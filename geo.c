#include <stdlib.h>
#include <time.h>

#include "balls.h"

static float randFloat(float lo, float hi);

int
isCollision(float2 p1, float r1, float2 p2, float r2) {
	float2 dist;
	float rhs;

	dist = p1 - p2;
	rhs = r1 + r2;
	return (dist[0]*dist[0] + dist[1]*dist[1]) <= rhs*rhs;
}

Rectangle
insetRect(Rectangle r, float n) {
	r.min += n;
	r.max -= n;
	return r;
}

float2
randPtInRect(Rectangle r) {
	float2 pt = {
		randFloat(r.min[0], r.max[0]),
		randFloat(r.min[1], r.max[1])
	};
	return pt;
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
