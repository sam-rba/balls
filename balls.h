#include <u.h>
#include <libc.h>
#include <stdio.h>
#include <draw.h>
#include <cursor.h>

typedef struct {
	double x, y;
} Vec;

typedef struct {
	Point p1, p2;
} Line;

typedef struct {
	Point p; /* position [pixels] */
	Vec v; /* velocity [m/s] */
	uint r; /* radius [pixels] */
	double m; /* mass [kg] */
} Ball;

Vec vadd(Vec v1, Vec v2);
Vec vsub(Vec v1, Vec v2);
Vec vmuls(Vec v, double a);
Vec vdivs(Vec v, double a);
double vdot(Vec v1, Vec v2);
double vlen(Vec v);
Vec unitnorm(Vec v);
Point ptaddv(Point p, Vec v);
Vec V(double x, double y);
Vec Vpt(Point p, Point q);

void drawbg(Image *walls, Image *bg);
Image *alloccircle(int fg, int bg, uint radius);
void drawcircle(Image *m, Point pos);

int iscollision(Point p1, uint r1, Point p2, uint r2);
void collideball(Ball *b1, const Ball *b2);
void collidewall(Ball *b, Rectangle wall);
