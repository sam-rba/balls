#include <stdlib.h>
#include <GL/glut.h>
#include <oneapi/tbb.h>

using namespace std;

enum window { WIDTH = 800, HEIGHT = 600 };
enum keys { KEY_QUIT = 'q' };
enum { CIRCLE_SEGS = 32 };

typedef struct {
	double x, y;
} Point;

typedef struct {
	Point min, max;
} Rectangle;

void keyboard(unsigned char key, int x, int y);
void display(void);
void drawBg(void);
void drawCircle(double radius, Point p);
void reshape(int w, int h);

const static Rectangle bounds = {{-1.5, -1.0}, {1.5, 1.0}};

int
main(int argc, char *argv[]) {
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowSize(WIDTH, HEIGHT);
	glutCreateWindow("Balls");

	glutKeyboardFunc(keyboard);
	glutDisplayFunc(display);
	glutReshapeFunc(reshape);

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
	static Point p = {0.35, 0.35};

	glClearColor(0, 0, 0, 1);
	glClear(GL_COLOR_BUFFER_BIT);
	drawBg();
	drawCircle(0.25, p);

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
