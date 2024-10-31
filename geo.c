#include <stddef.h>

#include "balls.h"

int
isCollision(Vector p1, float r1, Vector p2, float r2) {
	float dx, dy, rhs;

	dx = p1.x - p2.x;
	dy = p1.y - p2.y;
	rhs = r1 + r2;
	return (dx*dx + dy*dy) <= rhs*rhs;
}

Rectangle
insetRect(Rectangle r, float n) {
	r.min.x += n;
	r.min.y += n;

	r.max.y -= n;
	r.max.y -= n;

	return r;
}
