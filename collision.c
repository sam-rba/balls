#include "balls.h"

int
iscollision(Point p, Point q) {
	int dx, dy;

	dx = p.x - q.x;
	dy = p.y-q.y;
	return (dx*dx + dy*dy) < 4*RADIUS*RADIUS;
}

/* TODO: remove
int
collision(Ball b1, Ball b2, Point *p) {
	Point l1, l2, d;
	int dx, dy, closestdistsq, backdist, mvmtveclen;

	l1 = b1.p;
	l2 = Pt(b.p.x+b.v.x, b.p.y+b.v.y);
	d = closestpointonline(L(l1, l2), b2.p));

	dx = b2.p.x - d.x;
	dy = b2.p.y - d.y;
	closestdistsq = dx*dx + dy*dy;

	if (closestdistsq > 4*RADIUS*RADIUS)
		return 0;

	backdist = sqrt(4*RADIUS*RADIUS - closestdistsq);
	mvmtveclen = vlen(b1.v);
	p->x = d.x - backdist * (b1.v.x / mvmtveclen);
	p->y = d.y - backdist * (b1.v.y / mvmtveclen);
	return 1;
}

Vec
vpostcollision(Ball b1, Ball b2) {
	Point c1, c2;
	int dcx, dcy, d;
	Vec n;

	if (!collision(b1, b2, &c1)) {
		printf("warning: vpostcollision called, but no collision\n");
		return b1.v;
	}
	if (!collision(b2, b1, &c2)) {
		printf("warning: vpostcollision called, but no collision\n");
		return b1.v;
	}

	d = norm(vsub(c1, c2));
	n = V(dcx/d, dcy/d);
	p = 2 * (vdot(b1.v, n) - vdot(b2.v, n)) / (b1.m + b2.m);
	
	b1.v.x = b1.v.x - p * b1.m * n.x;
	b1.v.y = b1.v.y - p * b1.m * n.y;
	return b1.v;
}

Point
closestpointonline(Line l, Point p) {
	int a, b, c1, c2, d;
	Point c;
	
	a = l.p2.y - l.p1.y;
	b = l.p1.x - l.p2.x;

	c1 = a*l.p1.x + b*l.p1.y;
	c2 = -b*p.x + a*p.y;

	d = a*a - -b*b;
	if (d != 0) {
		c.x = (a*c1 - b*c2) / d;
		c.y = (a*c2 + b*c1) / d;
	} else {
		c = p;
	}

	return c;
}
*/
