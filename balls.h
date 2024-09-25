#include <u.h>
#include <libc.h>
#include <stdio.h>
#include <draw.h>
#include <cursor.h>

enum {
	RADIUS = 20,
};

typedef struct {
	double x, y;
} Vec;

typedef struct {
	Point p1, p2;
} Line;

typedef struct {
	Point p; /* position */
	Vec v; /* velocity */
	double m; /* mass */
} Ball;

Vec vsub(Vec v1, Vec v2);
Vec vmuls(Vec v, double a);
Vec vdivs(Vec v, double a);
double vdot(Vec v1, Vec v2);
double vlen(Vec v);
Vec unitnorm(Vec v);
Point ptaddv(Point p, Vec v);
Vec V(double x, double y);
Vec Vpt(Point p, Point q);

int iscollision(Point p, Point q);
void collideball(Ball *b1, const Ball *b2);
void collidewall(Ball *b, Rectangle wall);
