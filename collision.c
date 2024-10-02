#include "balls.h"

static double clamp(double v, double lo, double hi);
static double min(double a, double b);
static double max(double a, double b);

void
collideWall(Ball *b, Rectangle wall) {
	wall = insetRect(wall, b->r);

	if (b->p.x < wall.min.x || b->p.x > wall.max.x) {
		b->p.x = clamp(b->p.x, wall.min.x, wall.max.x);
		b->v.x = -b->v.x;
	}
	if (b->p.y < wall.min.y || b->p.y > wall.max.y) {
		b->p.y = clamp(b->p.y, wall.min.y, wall.max.y);
		b->v.y = -b->v.y;
	}
}

static double
clamp(double v, double lo, double hi) {
	return min(hi, max(v, lo));
}

static double
min(double a, double b) {
	return (a < b) ? a : b;
}

static double
max(double a, double b) {
	return (a > b) ? a : b;
}
