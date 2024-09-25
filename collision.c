#include "balls.h"

int
iscollision(Point p, Point q) {
	int dx, dy;

	dx = p.x - q.x;
	dy = p.y - q.y;
	return (dx*dx + dy*dy) <= 4*RADIUS*RADIUS;
}
