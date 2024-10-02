#include "balls.h"

static double clamp(double v, double lo, double hi);
static double min(double a, double b);
static double max(double a, double b);

int
isCollision(Point p1, double r1, Point p2, double r2) {
	double dx, dy;

	dx = p1.x - p2.x;
	dy = p1.y - p2.y;
	return (dx*dx + dy*dy) <= (r1+r2)*(r1+r2);
}

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

void
collideBall(Ball *b1, Ball *b2) {
	Point midpt;
	Vector n, distv;
	double mrat, coef;

	if (!isCollision(b1->p, b1->r, b2->p, b2->r))
		return;

	/* set position of collision */
	midpt = ptDivS(addPt(b1->p, b2->p), 2);
	n = unitNorm(VecPt(b1->p, b2->p));
	b1->p = ptSubVec(midpt, vecMulS(n, b1->r));
	b2->p = ptAddVec(midpt, vecMulS(n, b2->r));

	/* reaction velocity */

	mrat = 2.0 * b2->m / (b1->m + b2->m);
	distv = VecPt(b2->p, b1->p);
	coef = vecDot(subVec(b1->v, b2->v), distv) / (vecLen(distv)*vecLen(distv));
	b1->v = subVec(b1->v, vecMulS(distv, mrat*coef));

	mrat = 2.0 * b1->m / (b1->m + b2->m);
	distv = VecPt(b1->p, b2->p);
	coef = vecDot(subVec(b2->v, b1->v), distv) / (vecLen(distv)*vecLen(distv));
	b2->v = subVec(b2->v, vecMulS(distv, mrat*coef));
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
