#include <u.h>
#include <libc.h>
#include <draw.h>
#include <cursor.h>

enum {
	RADIUS = 20,
};

typedef struct {
	int x, y;
} Vec;

typedef struct {
	Point p1, p2;
} Line;

typedef struct {
	Point p; /* position */
	Vec v; /* velocity */
	int m; /* mass */
} Ball;

Vec vsub(Vec v1, Vec v2);
Vec vmuls(Vec v, int a);
Vec vdivs(Vec v, int a);
int vdot(Vec v1, Vec v2);
int vlen(Vec v);
Vec unitnorm(Vec v);
Point ptaddv(Point p, Vec v);
Vec V(int x, int y);
Vec Vpt(Point p, Point q);

int iscollision(Point p, Point q);