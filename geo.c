#include "balls.h"

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
