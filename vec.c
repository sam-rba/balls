#include "balls.h"

Vec
vadd(Vec v1, Vec v2) {
	return V(v1.x+v2.x, v1.y+v2.y);
}

Vec
vsub(Vec v1, Vec v2) {
	return V(v1.x-v2.x, v1.y-v2.y);
}

Vec
vmuls(Vec v, double a) {
	return V(v.x*a, v.y*a);
}

Vec
vdivs(Vec v, double a) {
	if (a == 0)
		return V(0, 0);
	return V(v.x/a, v.y/a);
}

double
vdot(Vec v1, Vec v2) {
	return v1.x*v2.x + v1.y*v2.y;
}

double
vlen(Vec v) {
	return sqrt(v.x*v.x + v.y*v.y);
}

Vec
unitnorm(Vec v) {
	return vdivs(v, vlen(v));
}

Point
ptaddv(Point p, Vec v) {
	p.x += v.x;
	p.y += v.y;
	return p;
}

Vec
V(double x, double y) {
	Vec v = {x, y};
	return v;
}

Vec
Vpt(Point p, Point q) {
	double dx, dy;

	dx = (double) q.x - (double) p.x;
	dy = (double) q.y - (double) p.y;
	return V(dx, dy);
}
