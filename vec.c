#include "balls.h"

Vec
vsub(Vec v1, Vec v2) {
	return V(v1.x-v2.x, v1.y-v2.y);
}

Vec
vmuls(Vec v, int a) {
	return V(v.x*a, v.y*a);
}

Vec
vdivs(Vec v, int a) {
	if (a == 0)
		return V(0, 0);
	return V(v.x/a, v.y/a);
}

int
vdot(Vec v1, Vec v2) {
	return v1.x*v2.x + v1.y*v2.y;
}

int
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
V(int x, int y) {
	Vec v = {x, y};
	return v;
}

Vec
Vpt(Point p, Point q) {
	return V(p.x-q.x, p.y-q.y);
}
