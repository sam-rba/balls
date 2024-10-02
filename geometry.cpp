#include "balls.h"

Point
ptAddVec(Point p, Vector v) {
	p.x += v.x;
	p.y += v.y;
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
