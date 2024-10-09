#include <stdlib.h>
#include <iostream>
#include <math.h>
#include <GL/glut.h>
#include <oneapi/tbb.h>
#include <oneapi/tbb/flow_graph.h>

using namespace std;
using namespace oneapi::tbb;
using namespace oneapi::tbb::flow;

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
	float r, g, b;
} Color;

typedef struct {
	Point p; /* position [m] */
	Vector v; /* velocity [m/s] */
	double r; /* radius [m] */
	double m; /* mass [kg] */
	Color color;
} Ball;

class Collision {
public:
	Ball *b1, *b2;

	Collision(Ball *_b1, Ball *_b2) {
		b1 = _b1;
		b2 = _b2;
	}

	friend bool operator<(const Collision& a, const Collision& b) {
		return a.b1 < b.b1 || a.b2 < b.b2;
	}

	friend bool operator==(const Collision& a, const Collision& b) {
		return a.b1 == b.b1 && a.b2 == b.b2;
	}

	friend ostream& operator<<(ostream& os, Collision const & c) {
		return os << "(" << c.b1 << ", " << c.b2 << ")";
	}
};

vector<vector<Collision>> partitionCollisions(vector<Ball *> balls);

Point addPt(Point p, Point q);
Point subPt(Point p, Point q);
Point ptAddVec(Point p, Vector v);
Point ptSubVec(Point p, Vector v);
Point ptMulS(Point p, double s);
Point ptDivS(Point p, double s);
Point Pt(double x, double y);
Rectangle insetRect(Rectangle r, double n);
Point randPtInRect(Rectangle r);

Vector addVec(Vector v1, Vector v2);
Vector subVec(Vector v1, Vector v2);
Vector vecMulS(Vector v, double s);
Vector vecDivS(Vector v, double s);
double vecDot(Vector v1, Vector v2);
Vector unitNorm(Vector v);
double vecLen(Vector v);
Vector Vec(double x, double y);
Vector VecPt(Point p, Point q);

int isCollision(Point p1, double r1, Point p2, double r2);
void collideWall(Ball *b, Rectangle wall);
void collideBall(Ball *b1, Ball *b2);

double randDouble(double lo, double hi);
