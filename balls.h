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

Point addPt(Point p, Point q);
Point subPt(Point p, Point q);
Point ptAddVec(Point p, Vector v);
Point ptSubVec(Point p, Vector v);
Point ptMulS(Point p, double s);
Point ptDivS(Point p, double s);
Point Pt(double x, double y);

Vector addVec(Vector v1, Vector v2);
Vector subVec(Vector v1, Vector v2);
Vector vecMulS(Vector v, double s);
Vector vecDivS(Vector v, double s);
double vecDot(Vector v1, Vector v2);
Vector unitNorm(Vector v);
double vecLen(Vector v);
Vector Vec(double x, double y);
Vector VecPt(Point p, Point q);

Rectangle insetRect(Rectangle r, double n);
Point randPtInRect(Rectangle r);

int isCollision(Point p1, double r1, Point p2, double r2);
void collideWall(Ball *b, Rectangle wall);
void collideBall(Ball *b1, Ball *b2);

int randInt(int lo, int hi);
double randDouble(double lo, double hi);
