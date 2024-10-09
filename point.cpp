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

Rectangle
insetRect(Rectangle r, double n) {
	r.min.x += n;
	r.min.y += n;
	r.max.x -= n;
	r.max.y -= n;
	return r;
}
