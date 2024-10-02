#include <math.h>

#include "balls.h"

Point
addPt(Point p, Point q) {
	p.x += q.x;
	p.y += q.y;
	return p;
}

Point
subPt(Point p, Point q) {
	p.x -= q.x;
	p.y -=q.y;
	return p;
}

Point
ptAddVec(Point p, Vector v) {
	p.x += v.x;
	p.y += v.y;
	return p;
}

Point
ptSubVec(Point p, Vector v) {
	p.x -= v.x;
	p.y -= v.y;
	return p;
}

Point
ptMulS(Point p, double s) {
	p.x *= s;
	p.y *= s;
	return p;
}

Point
ptDivS(Point p, double s) {
	p.x /= s;
	p.y /=s;
	return p;
}

Point
Pt(double x, double y) {
	Point p = {x, y};
	return p;
}

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

Rectangle
insetRect(Rectangle r, double n) {
	r.min.x += n;
	r.min.y += n;
	r.max.x -= n;
	r.max.y -= n;
	return r;
}

Point
randPtInRect(Rectangle r) {
	return Pt(randDouble(r.min.x, r.max.x), randDouble(r.min.y, r.max.y));
}
