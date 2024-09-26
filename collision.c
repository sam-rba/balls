#include "balls.h"

static int clamp(int v, int lo, int hi);
static int min(int a, int b);
static int max(int a, int b);

int
iscollision(Point p1, uint r1, Point p2, uint r2) {
	int dx, dy;

	dx = p1.x - p2.x;
	dy = p1.y - p2.y;
	return (dx*dx + dy*dy) <= (r1+r2)*(r1+r2);
}

void
collideball(Ball *b1, const Ball *b2) {
	Vec n, distv;
	double dist, coef, mrat;

	if (!iscollision(b1->p, b1->r, b2->p, b2->r))
		return;

	printf("collision (%d,%d), (%d,%d)\n", b1->p.x, b1->p.y, b2->p.x, b2->p.y);
	printf("oldv: (%2.2f,%2.2f\n", b1->v.x, b1->v.y);
	printf("m1(%f) m2(%f)\n", b1->m, b2->m);

	n = unitnorm(Vpt(b2->p, b1->p));
	dist = b1->r + b2->r;
	b1->p = ptaddv(b2->p, vmuls(n, dist));

	distv = Vpt(b2->p, b1->p);
	coef = vdot(vsub(b1->v, b2->v), distv) / (vlen(distv)*vlen(distv));
	mrat = 2.0 * b2->m / (b1->m + b2->m);
	b1->v = vsub(b1->v, vmuls(distv, mrat*coef));

	printf("dist(%f,%f) coef(%f) mrat(%f) v(%f,%f)\n", distv.x, distv.y, coef, mrat, b1->v.x, b1->v.y);
}

void
collidewall(Ball *b, Rectangle wall) {
	wall = insetrect(wall, b->r);

	if (b->p.x < wall.min.x || b->p.x > wall.max.x) {
		b->p.x = clamp(b->p.x, wall.min.x, wall.max.x);
		printf("clamped to %d\n", b->p.x);
		b->v.x = -b->v.x;
	}
	if (b->p.y < wall.min.y || b->p.y > wall.max.y) {
		b->p.y = clamp(b->p.y, wall.min.y, wall.max.y);
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