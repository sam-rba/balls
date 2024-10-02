#include <stdlib.h>
#include <GL/glut.h>
#include <oneapi/tbb.h>

#include "balls.h"

using namespace std;

enum {
	WIDTH = 800,
	HEIGHT = 600,

	KEY_QUIT = 'q',

	CIRCLE_SEGS = 32,

	FPS = 60,
	MS_PER_S = 1000,
	FRAME_TIME_MS = MS_PER_S / FPS,
};

void keyboard(unsigned char key, int x, int y);
void display(void);
void drawBg(void);
void drawCircle(double radius, Point p);
void reshape(int w, int h);
void animate(int v);

const static Rectangle bounds = {{-1.5, -1.0}, {1.5, 1.0}};
static Ball ball = {{0.25, 0.25}, {0.25, 0.25}, 0.200, 0.25};

int
main(int argc, char *argv[]) {
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowSize(WIDTH, HEIGHT);
	glutCreateWindow("Balls");

	glutKeyboardFunc(keyboard);
	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutTimerFunc(FRAME_TIME_MS, animate, 0);

	glClearColor(1.0, 1.0, 1.0, 1.0);

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
	drawBg();
	drawCircle(ball.r, ball.p);

	glutSwapBuffers();
}

void
drawBg(void) {
	glColor3f(1, 1, 1);
	glBegin(GL_QUADS);
		glVertex2f(bounds.min.x, bounds.min.y);
		glVertex2f(bounds.max.x, bounds.min.y);
		glVertex2f(bounds.max.x, bounds.max.y);
		glVertex2f(bounds.min.x, bounds.max.y);
	glEnd();
}

void
drawCircle(double radius, Point p) {
	int i;
	double theta, x, y;

	glColor3f(0.1, 0.6, 0.8);
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

void
animate(int v) {
	ball.p = ptAddVec(ball.p, ball.v);

	collideWall(&ball, bounds);

	display();
	glutTimerFunc(FRAME_TIME_MS, animate, 0);
}
