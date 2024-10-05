#include "balls.h"

#define EPSILON 1e-7
	/* account for floating-point error when testing for collision */

static void setPosition(Ball *b1, Ball *b2);
static Vector reaction(Ball b1, Ball b2);
static double clamp(double v, double lo, double hi);
static double min(double a, double b);
static double max(double a, double b);

int
isCollision(Point p1, double r1, Point p2, double r2) {
	double dx, dy, rhs;

	dx = p1.x - p2.x;
	dy = p1.y - p2.y;
	rhs = r1 + r2 + EPSILON;
	return (dx*dx + dy*dy) <= rhs*rhs;
}

void
collideWall(Ball *b, Rectangle wall) {
	wall = insetRect(wall, b->r + EPSILON);

	if (b->p.x < wall.min.x || b->p.x > wall.max.x) {
		b->p.x = clamp(b->p.x, wall.min.x, wall.max.x);
		b->v.x = -b->v.x;
	}
	if (b->p.y < wall.min.y || b->p.y > wall.max.y) {
		b->p.y = clamp(b->p.y, wall.min.y, wall.max.y);
		b->v.y = -b->v.y;
	}
}

void
collideBall(Ball *b1, Ball *b2) {
	Vector v1, v2;

	if (!isCollision(b1->p, b1->r, b2->p, b2->r))
		return;

	setPosition(b1, b2);

	v1 = reaction(*b1, *b2);
	v2 = reaction(*b2, *b1);
	b1->v = v1;
	b2->v = v2;
}

/* set the positions of b1 and b2 at the moment of collsion */
static void
setPosition(Ball *b1, Ball *b2) {
	Point mid;
	Vector n;

	mid = ptDivS(addPt(b1->p, b2->p), 2);
	n = unitNorm(VecPt(b1->p, b2->p));
	b1->p = ptSubVec(mid, vecMulS(n, b1->r + EPSILON));
	b2->p = ptAddVec(mid, vecMulS(n, b2->r + EPSILON));
}

/* return the velocity of b1 after colliding with b2 */
static Vector
reaction(Ball b1, Ball b2) {
	double mrat, coef;
	Vector distv;

	mrat = 2.0 * b2.m / (b1.m + b2.m);
	distv = VecPt(b2.p, b1.p);
	coef = vecDot(subVec(b1.v, b2.v), distv) / (vecLen(distv)*vecLen(distv));
	return subVec(b1.v, vecMulS(distv, mrat*coef));
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
