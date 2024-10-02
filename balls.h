#include <stdlib.h>

typedef struct {
	double x, y;
} Point;

typedef struct {
	Point min, max;
} Rectangle;

typedef struct {
	double x, y;
} Vector;

typedef struct {
	Point p; /* position [m] */
	Vector v; /* velocity [m/s] */
	double r; /* radius [m] */
	double m; /* mass [kg] */
} Ball;

Point ptAddVec(Point p, Vector v);
Point Pt(double x, double y);
Rectangle insetRect(Rectangle r, double n);
Point randPtInRect(Rectangle r);

int isCollision(Point p1, double r1, Point p2, double r2);
void collideWall(Ball *b, Rectangle wall);

int randInt(int lo, int hi);
double randDouble(double lo, double hi);
