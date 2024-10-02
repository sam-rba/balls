#include "balls.h"

Point
ptAddVec(Point p, Vector v) {
	p.x += v.x;
	p.y += v.y;
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

Point
randPtInRect(Rectangle r) {
	return Pt(randDouble(r.min.x, r.max.x), randDouble(r.min.y, r.max.y));
}
