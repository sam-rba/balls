#define CL_TARGET_OPENCL_VERSION 110

#include <stdlib.h>
#include <stdio.h>
#include <GL/glew.h>
#include <GL/glut.h>
#include <GL/glx.h>
#include <CL/cl_gl.h>

#include "sysfatal.h"

#define nelem(arr) (sizeof(arr) / sizeof(arr[0]))

#define PROG_FILE "balls.cl"
#define KERNEL_FUNC "balls"
#define VERTEX_SHADER "balls.vert"
#define FRAGMENT_SHADER "balls.frag"

enum { WIDTH = 640, HEIGHT = 480 };
enum { NBALLS = 1, CIRCLE_SEGS = 16 };

void initGL(int argc, char *argv[]);
void initCL(void);
void configureSharedData(void);
void execKernel(void);
void freeCL(void);
void freeGL(void);
void initShaders(void);
char *readFile(const char *filename, size_t *size);
void compileShader(GLint shader);
void display(void);
void reshape(int w, int h);

static cl_context context;
cl_program prog;
static cl_command_queue queue;
static cl_kernel kernel;
GLuint positionVAO, positionVBO, vertexVAO, vertexVBO;
cl_mem positionBuf, vertexBuf;

int
main(int argc, char *argv[]) {
	initGL(argc, argv);

	initCL();

	configureSharedData();

	glutDisplayFunc(display);
	glutReshapeFunc(reshape);

	glutMainLoop();

	freeCL();
	freeGL();

	return 0;
}

void
initGL(int argc, char *argv[]) {
	GLenum err;

	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowSize(WIDTH, HEIGHT);
	glutCreateWindow("Balls");
	glClearColor(1, 1, 1, 1);

	if ((err = glewInit()) != GLEW_OK)
		sysfatal("Failed to initialize GLEW.\n");

	initShaders();
}

void
initCL(void) {
	cl_platform_id platform;
	cl_device_id device;
	cl_int err;
	char *progBuf, *progLog;
	size_t progSize, logSize;

	/* Get platform. */
	if (clGetPlatformIDs(1, &platform, NULL) < 0)
		sysfatal("No OpenCL platform available.\n");

	/* Configure properties for OpenGL interoperability. */
	#ifdef WINDOWS
		cl_context_properties properties[] = {
			CL_GL_CONTEXT_KHR, (cl_context_properties) wglGetCurrentContext(),
			CL_GLX_DISPLAY_KHR, (cl_context_properties) wctlGetCurrentDC(),
			CL_CONTEXT_PLATFORM, (cl_context_properties) platform,
			0
		};
	#else
		cl_context_properties properties[] = {
			CL_GL_CONTEXT_KHR, (cl_context_properties) glXGetCurrentContext(),
			CL_GLX_DISPLAY_KHR, (cl_context_properties) glXGetCurrentDisplay(),
			CL_CONTEXT_PLATFORM, (cl_context_properties) platform,
			0
		};
	#endif

	/* Get GPU device. */
	if (clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, NULL) < 0)
		sysfatal("No GPUs available.\n");

	/* Create context. */
	context = clCreateContext(properties, 1, &device, NULL, NULL, &err);
	if (err < 0)
		sysfatal("Failed to create context.\n");

	/* Create program from file. */
	progBuf = readFile(PROG_FILE, &progSize);
	prog = clCreateProgramWithSource(context, 1, (const char **) &progBuf, &progSize, &err);
	if (err < 0)
		sysfatal("Failed to create program.\n");
	free(progBuf);

	/* Build program. */
	err = clBuildProgram(prog, 0, NULL, NULL, NULL, NULL);
	if (err < 0) {
		/* Print build log. */
		clGetProgramBuildInfo(prog, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &logSize);
		progLog = malloc(logSize + 1);
		progLog[logSize] = '\0';
		clGetProgramBuildInfo(prog, device, CL_PROGRAM_BUILD_LOG, logSize+1, progLog, NULL);
		fprintf(stderr, "%s\n", progLog);
		free(progLog);
		exit(1);
	}

	/* Create command queue. */
	queue = clCreateCommandQueue(context, device, 0, &err);
	if (err < 0)
		sysfatal("Failed to create command queue.\n");

	/* Create kernel. */
	kernel = clCreateKernel(prog, KERNEL_FUNC, &err);
	if (err < 0)
		sysfatal("Failed to create kernel: %d\n", err);
}

void
configureSharedData(void) {
	int err;

	/* Create position array. */
	glGenVertexArrays(1, &positionVAO);
	glBindVertexArray(positionVAO);

	/* Create vertex array. */
	glGenVertexArrays(1, &vertexVAO);
	glBindVertexArray(vertexVAO);

	/* Create position buffer. */
	glGenBuffers(1, &positionVBO);
	glBindBuffer(GL_ARRAY_BUFFER, positionVBO);
	glBufferData(GL_ARRAY_BUFFER, 2 * NBALLS * sizeof(GLfloat), NULL, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);

	/* Create vertex buffer. */
	glGenBuffers(1, &vertexVBO);
	glBindBuffer(GL_ARRAY_BUFFER, vertexVBO);
	glBufferData(GL_ARRAY_BUFFER, 2 * (CIRCLE_SEGS+2) * sizeof(GLfloat), NULL, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);

	/* Create CL memory object from position buffer. */
	positionBuf = clCreateFromGLBuffer(context, CL_MEM_WRITE_ONLY, positionVBO, &err);
	if (err < 0)
		sysfatal("Failed to create buffer object from VBO.\n");

	/* Create CL memory object from vertex buffer. */
	vertexBuf = clCreateFromGLBuffer(context, CL_MEM_WRITE_ONLY, vertexVBO, &err);
	if (err < 0)
		sysfatal("Failed to create buffer object from VBO.\n");

	/* Set kernel arguments. */
	err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &positionBuf);
	err |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &vertexBuf);
	if (err < 0)
		sysfatal("Failed to set kernel arguments.\n");
}

void
execKernel(void) {
	int err;
	size_t globalSize;
	cl_event kernelEvent;

	glFinish();

	err = clEnqueueAcquireGLObjects(queue, 1, &vertexBuf, 0, NULL, NULL);
	if (err < 0)
		sysfatal("Couldn't acquire the GL objects.\n");

	globalSize = CIRCLE_SEGS+1;
	err = clEnqueueNDRangeKernel(queue, kernel, 1, NULL, &globalSize, NULL, 0, NULL, &kernelEvent);
	if (err < 0)
		sysfatal("Couldn't enqueue kernel.\n");

	err = clWaitForEvents(1, &kernelEvent);
	if (err < 0)
		sysfatal("Couldn't enqueue the kernel.\n");

	clEnqueueReleaseGLObjects(queue, 1, &vertexBuf, 0, NULL, NULL);
	clFinish(queue);
	clReleaseEvent(kernelEvent);
}

void
freeCL(void) {
	clReleaseMemObject(positionBuf);
	clReleaseMemObject(vertexBuf);
	clReleaseKernel(kernel);
	clReleaseCommandQueue(queue);
	clReleaseProgram(prog);
	clReleaseContext(context);
}

void
freeGL(void) {
	glDeleteBuffers(1, &positionVBO);
	glDeleteBuffers(1, &positionVAO);
	glDeleteBuffers(1, &vertexVBO);
	glDeleteBuffers(1, &vertexVAO);
}

void
initShaders(void) {
	GLuint vs, fs, prog;
	char *vSrc, *fSrc;
	size_t vLen, fLen;

	vs = glCreateShader(GL_VERTEX_SHADER);
	fs = glCreateShader(GL_FRAGMENT_SHADER);

	vSrc = readFile(VERTEX_SHADER, &vLen);
	fSrc = readFile(FRAGMENT_SHADER, &fLen);

	glShaderSource(vs, 1, (const char **) &vSrc, (GLint *) &vLen);
	glShaderSource(fs, 1, (const char **) &fSrc, (GLint *) &fLen);

	compileShader(vs);
	compileShader(fs);

	prog = glCreateProgram();

	glBindAttribLocation(prog, 0, "in_coords");
	glBindAttribLocation(prog, 1, "in_colors");

	glAttachShader(prog, vs);
	glAttachShader(prog, fs);

	glLinkProgram(prog);
	glUseProgram(prog);
}

char *
readFile(const char *filename, size_t *size) {
	FILE *f;
	char *buf;

	if ((f = fopen(filename, "r")) == NULL)
		sysfatal("Failed to open file '%s'\n", filename);
	fseek(f, 0, SEEK_END);
	*size = ftell(f);
	if ((buf = malloc((*size + 1) * sizeof(char))) == NULL) {
		fclose(f);
		sysfatal("Failed to allocate file buffer for '%s'\n", filename);
	}
	rewind(f);
	fread(buf, sizeof(char), *size, f);
	buf[*size] = '\0';
	fclose(f);
	return buf;
}

void
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

void
display(void) {
	glClear(GL_COLOR_BUFFER_BIT |GL_DEPTH_BUFFER_BIT);

	execKernel();

	glBindVertexArray(vertexVAO);
	glDrawArrays(GL_TRIANGLE_FAN, 0, CIRCLE_SEGS+2);

	glBindVertexArray(0);
	glutSwapBuffers();
	glutPostRedisplay();
}

void
reshape(int w, int h) {
	glViewport(0, 0, (GLsizei) w, (GLsizei) h);
}
