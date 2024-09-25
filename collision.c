#include "balls.h"

static int clamp(int v, int lo, int hi);
static int min(int a, int b);
static int max(int a, int b);

int
iscollision(Point p, Point q) {
	int dx, dy;

	dx = p.x - q.x;
	dy = p.y - q.y;
	return (dx*dx + dy*dy) <= 4*RADIUS*RADIUS;
}

void
collideball(Ball *b1, const Ball *b2) {
	Point midpoint;
	Vec d, n;
	double magnitude;

	if (!iscollision(b1->p, b2->p))
		return;
	midpoint = divpt(addpt(b1->p, b2->p), 2);
	d = Vpt(b2->p, b1->p);
	b1->p = ptaddv(midpoint, vmuls(unitnorm(d), RADIUS));

	printf("collision (%d,%d), (%d,%d)\n", b1->p.x, b1->p.y, b2->p.x, b2->p.y);

	printf("oldv: (%2.2f,%2.2f\n", b1->v.x, b1->v.y);

	n = unitnorm(Vpt(b2->p, b1->p));
	magnitude = 2*(vdot(b1->v, n) - vdot(b2->v, n)) / (b1->m + b2->m);

	printf("n: (%2.2f,%2.2f), magnitude: %2.2f\n", n.x, n.y, magnitude);

	b1->v = vsub(b1->v, vmuls(n, magnitude * b1->m));

	printf("newv: (%2.2f,%2.2f)\n", b1->v.x, b1->v.y);
}

void
collidewall(Ball *b, Rectangle wall) {
	if (b->p.x < wall.min.x+RADIUS || b->p.x > wall.max.x-RADIUS) {
		b->p.x = clamp(b->p.x, wall.min.x+RADIUS, wall.max.x-RADIUS);
		printf("clamped to %d\n", b->p.x);
		b->v.x = -b->v.x;
	}
	if (b->p.y < wall.min.y+RADIUS || b->p.y > wall.max.y-RADIUS) {
		b->p.y = clamp(b->p.y, wall.min.y+RADIUS, wall.max.y-RADIUS);
		b->v.y = -b->v.y;
	}
}

static int
clamp(int v, int lo, int hi) {
	return min(hi, max(v, lo));
}

static int
min(int a, int b) {
	return (a < b) ? a : b;
}

static int
max(int a, int b) {
	return (a > b) ? a : b;
}