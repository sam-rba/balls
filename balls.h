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
	double m; /* mass [kg] */
	double r; /* radius [m] */
} Ball;

Point ptAddVec(Point p, Vector v);
Rectangle insetRect(Rectangle r, double n);

void collideWall(Ball *b, Rectangle wall);