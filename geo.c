#include <stdlib.h>

#include "sysfatal.h"
#include "balls.h"

int
isCollision(Vector p1, float r1, Vector p2, float r2) {
	float dx, dy, rhs;

	dx = p1.x - p2.x;
	dy = p1.y - p2.y;
	rhs = r1 + r2;
	return (dx*dx + dy*dy) <= rhs*rhs;
}

Rect
insetRect(Rect r, float n) {
	r.min.x += n;
	r.min.y += n;

	r.max.y -= n;
	r.max.y -= n;

	return r;
}

/* Generate n circle coordinates within bounds such that no circles overlap. */
Vector *
noOverlapPositions(int n, Rect bounds, float radius) {
	Vector *ps;
	int i, j;

	if ((ps = malloc(n*sizeof(Vector))) == NULL)
		sysfatal("Failed to allocate position array.\n");

	bounds = insetRect(bounds, radius);
	for (i = 0; i < n; i++) {
		ps[i] = randPtInRect(bounds);
		for (j = 0; j < i; j++)
			if (isCollision(ps[j], radius, ps[i], radius))
				break;
		if (j < i) { /* Overlapping. */
			i--;
			continue;
		}
	}

	return ps;
}