#include <stdio.h>
#include <GL/glew.h>
#include <GL/glut.h>

#include "gl.h"
#include "balls.h"
#include "config.h"
#include "sysfatal.h"

#define VERTEX_SHADER "balls.vert"
#define FRAGMENT_SHADER "balls.frag"

static void initShaders(void);
static void compileShader(GLint shader);
static void genVertexBuffer(GLuint *vertexVBO, int nBalls);
static void genColorBuffer(GLuint *colorVBO, int nBalls);

void
initGL(int argc, char *argv[]) {
	GLenum err;

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowSize(WIDTH, HEIGHT);
	glutCreateWindow(WINDOW_TITLE);
	glClearColor(1, 1, 1, 1);

	if ((err = glewInit()) != GLEW_OK)
		sysfatal("Failed to initialize GLEW.\n");

	initShaders();
}

/* Create GL vertex and color buffers. */
void
genBuffers(GLuint *vertexVAO, GLuint *vertexVBO, GLuint *colorVBO, int nBalls) {
	glGenVertexArrays(1, vertexVAO);
	glBindVertexArray(*vertexVAO);
	genVertexBuffer(vertexVBO, nBalls);
	genColorBuffer(colorVBO, nBalls);
}

void
freeGL(GLuint vertexVAO, GLuint vertexVBO, GLuint colorVBO) {
	glDeleteBuffers(1, &vertexVBO);
	glDeleteBuffers(1, &vertexVAO);
	glDeleteBuffers(1, &colorVBO);
}

static void
initShaders(void) {
	GLuint vs, fs, prog;
	int err;
	char *vSrc, *fSrc;
	size_t vLen, fLen;

	vs = glCreateShader(GL_VERTEX_SHADER);
	fs = glCreateShader(GL_FRAGMENT_SHADER);

	err = readFile(VERTEX_SHADER, &vSrc, &vLen);
	if (err != 0)
		sysfatal("Failed to read '%s'\n", VERTEX_SHADER);
	err = readFile(FRAGMENT_SHADER, &fSrc, &fLen);
	if (err != 0)
		sysfatal("Failed to read '%s'\n", FRAGMENT_SHADER);

	glShaderSource(vs, 1, (const char **) &vSrc, (GLint *) &vLen);
	glShaderSource(fs, 1, (const char **) &fSrc, (GLint *) &fLen);

	compileShader(vs);
	compileShader(fs);

	free(vSrc);
	free(fSrc);

	prog = glCreateProgram();

	glBindAttribLocation(prog, 0, "in_coords");
	glBindAttribLocation(prog, 1, "in_colors");

	glAttachShader(prog, vs);
	glAttachShader(prog, fs);

	glLinkProgram(prog);
	glUseProgram(prog);
}

static void
compileShader(GLint shader) {
	GLint success;
	GLsizei logSize;
	GLchar *log;

	glCompileShader(shader);
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logSize);
		if ((log = malloc((logSize+1) * sizeof(GLchar))) == NULL)
			sysfatal("Failed to allocate space for shader compile log.\n");
		glGetShaderInfoLog(shader, logSize+1, NULL, log);
		log[logSize] = '\0';
		fprintf(stderr, "%s\n", log);
		free(log);
		exit(1);
	}
}

static void
genVertexBuffer(GLuint *vertexVBO, int nBalls) {
	glGenBuffers(1, vertexVBO);
	glBindBuffer(GL_ARRAY_BUFFER, *vertexVBO);
	glBufferData(GL_ARRAY_BUFFER, nBalls*CIRCLE_POINTS*2*sizeof(GLfloat), NULL, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);
}

static void
genColorBuffer(GLuint *colorVBO, int nBalls) {
	GLfloat (*colors)[3];
	GLfloat color[3];
	int i, j;

	if ((colors = malloc(nBalls*CIRCLE_POINTS*3*sizeof(GLfloat))) == NULL)
		sysfatal("Failed to allocate color array.\n");
	for (i = 0; i < nBalls; i++) {
		color[0] = randFloat(0, 1);
		color[1] = randFloat(0, 1);
		color[2] = randFloat(0, 1);
		for (j = 0; j < CIRCLE_POINTS; j++) {
			colors[i*CIRCLE_POINTS + j][0] = color[0];
			colors[i*CIRCLE_POINTS + j][1] = color[1];
			colors[i*CIRCLE_POINTS + j][2] = color[2];
		}
	}

	glGenBuffers(1, colorVBO);
	glBindBuffer(GL_ARRAY_BUFFER, *colorVBO);
	glBufferData(GL_ARRAY_BUFFER, nBalls*CIRCLE_POINTS*3*sizeof(GLfloat), colors, GL_STATIC_DRAW);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(1);

	free(colors);
}
