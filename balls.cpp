#include <stdlib.h>
#include <iostream>
#include <GL/glut.h>
#include <oneapi/tbb.h>

#include "balls.h"

using namespace std;

#define VMAX_INIT 0.05
	/* max initial velocity [m/frame] */
#define RMIN 0.05
#define RMAX 0.10
	/* min/max radius [m] */
#define DENSITY 1500.0
	/* density of ball [kg/m^3] */
#define G (9.81/FPS/FPS)
	/* acceleration of gravity [m/(frame*frame)] */

enum {
	WIDTH = 800,
	HEIGHT = 600,

	KEY_QUIT = 'q',

	CIRCLE_SEGS = 32,

	FPS = 60,
	MS_PER_S = 1000,
	FRAME_TIME_MS = MS_PER_S / FPS,

	NBALLS_DEFAULT = 3,
};

void keyboard(unsigned char key, int x, int y);
void display(void);
void drawBackground(void);
void drawCircle(double radius, Point p, Color color);
void reshape(int w, int h);
vector<Ball> makeBalls(unsigned int n);
vector<Point> noOverlapCircles(unsigned int n);
double mass(double radius);
double volume(double radius);
void animate(int v);
Color randColor(void);

const static Rectangle bounds = {{-1.5, -1.0}, {1.5, 1.0}};

static vector<Ball> balls;

int
main(int argc, char *argv[]) {
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowSize(WIDTH, HEIGHT);
	glutCreateWindow("Balls");

	glutKeyboardFunc(keyboard);
	glutDisplayFunc(display);
	glutReshapeFunc(reshape);

	balls = makeBalls(NBALLS_DEFAULT);

	glutTimerFunc(FRAME_TIME_MS, animate, 0);

	glutMainLoop();

	return 1;
}

void
keyboard(unsigned char key, int x, int y) {
	if (key == KEY_QUIT)
		glutDestroyWindow(glutGetWindow());
}

void
display(void) {
	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	drawBackground();
	for (Ball b : balls)
		drawCircle(b.r, b.p, b.color);

	glutSwapBuffers();
}

void
drawBackground(void) {
	glColor3f(1, 1, 1);
	glBegin(GL_QUADS);
		glVertex2f(bounds.min.x, bounds.min.y);
		glVertex2f(bounds.max.x, bounds.min.y);
		glVertex2f(bounds.max.x, bounds.max.y);
		glVertex2f(bounds.min.x, bounds.max.y);
	glEnd();
}

void
drawCircle(double radius, Point p, Color color) {
	int i;
	double theta, x, y;

	glColor3f(color.r, color.g, color.b);
	glBegin(GL_TRIANGLE_FAN);
		glVertex2f(p.x, p.y);
		for (i = 0; i <= CIRCLE_SEGS; i++) {
			theta = 2.0 * M_PI * i / CIRCLE_SEGS;
			x = radius * cosf(theta);
			y = radius * sinf(theta);
			glVertex2f(x+p.x, y+p.y);
		}
	glEnd();
}

void
reshape(int w, int h) {
	double ratio;

	glViewport(0, 0, w, h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	ratio = (double) w / (double) h;
	if (ratio >= 1.0)
		glOrtho(-ratio, ratio, -1, 1, -1, 1);
	else
		glOrtho(-1, 1, -1.0/ratio, 1.0/ratio, -1, 1);
	glMatrixMode(GL_MODELVIEW);
}

/* return n-length vector of Balls with random attributes */
vector<Ball>
makeBalls(unsigned int n) {
	vector<Ball> balls(n);
	vector<Point> ps = noOverlapCircles(n);
	unsigned int i;
	
	for (i = 0; i < n; i++) {
		cout << "Creating ball " << i << "\n";
		balls[i].p = ps[i];

		balls[i].v.x = randDouble(-VMAX_INIT, VMAX_INIT);
		balls[i].v.y = randDouble(-VMAX_INIT, VMAX_INIT);

		balls[i].r = randDouble(RMIN, RMAX);

		balls[i].m = mass(balls[i].r);

		balls[i].color = randColor();
	}
	return balls;
}

/* return n-length vector of positions of non-overlapping circles with radius RMAX within the bounds */
vector<Point>
noOverlapCircles(unsigned int n) {
	vector<Point> ps(n);
	Rectangle r;
	unsigned int i, j;

	r = insetRect(bounds, RMAX);
	for (i = 0; i < n; i++) {
		cout << "Create non-overlapping circle " << i << "\n";
		ps[i] = randPtInRect(r);
		for (j = 0; j < i; j++) /* TODO: parallel reduce */
			if (isCollision(ps[j], RMAX, ps[i], RMAX))
				break;
		if (j < i) { /* overlapping */
			i--;
			continue;
		}
	}
	return ps;
}

/* mass [kg] of ball as function of radius [m] */
double
mass(double radius) {
	return volume(radius) * DENSITY;
}

/* volume [m^3] of ball as function of radius [m] */
double
volume(double radius) {
	return 4.0 * M_PI * radius*radius*radius / 3.0;
}

void
animate(int v) {
	size_t i, j;

	/* TODO: parallel */
	for (i = 0; i < balls.size(); i++) {
		for (j = i+1; j < balls.size(); j++)
			collideBall(&balls[i], &balls[j]);
		balls[i].v.y -= G;
		balls[i].p = ptAddVec(balls[i].p, balls[i].v);
		collideWall(&balls[i], bounds);
	}

	display();
	glutTimerFunc(FRAME_TIME_MS, animate, 0);
}

double
randDouble(double lo, double hi) {
	double r, diff;
	static int isInitialized = 0;

	if (!isInitialized) { /* first call */
		srand(time(0));
		isInitialized = 1;
	}

	r = (double) rand() / (double) RAND_MAX;
	diff = hi - lo;
	return lo + r*diff;
}

Color
randColor(void) {
	Color color;

	color.r = randDouble(0, 1);
	color.g = randDouble(0, 1);
	color.b = randDouble(0, 1);
	return color;
}
