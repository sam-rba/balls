#include <math.h>

#include "balls.h"

Vector
addVec(Vector v1, Vector v2) {
	v1.x += v2.x;
	v1.y += v2.y;
	return v1;
}

Vector
subVec(Vector v1, Vector v2) {
	v1.x -= v2.x;
	v1.y -= v2.y;
	return v1;
}

Vector
vecMulS(Vector v, double s) {
	v.x *= s;
	v.y *= s;
	return v;
}

Vector
vecDivS(Vector v, double s) {
	v.x /= s;
	v.y /= s;
	return v;
}

double
vecDot(Vector v1, Vector v2) {
	return v1.x*v2.x + v1.y*v2.y;
}

Vector
unitNorm(Vector v) {
	return vecDivS(v, vecLen(v));
}

double
vecLen(Vector v) {
	return sqrt(v.x*v.x + v.y*v.y);
}

Vector
Vec(double x, double y) {
	Vector v = {x, y};
	return v;
}

Vector
VecPt(Point p, Point q) {
	double dx, dy;

	dx = q.x - p.x;
	dy = q.y - p.y;
	return Vec(dx, dy);
}
