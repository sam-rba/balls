#define CL_TARGET_OPENCL_VERSION 110

#include <stdlib.h>
#include <stdio.h>
#include <GL/glew.h>
#include <GL/glut.h>
#include <GL/glx.h>
#include <CL/cl_gl.h>

#include "sysfatal.h"

#define PROG_FILE "balls.cl"
#define KERNEL_FUNC "balls"
#define VERTEX_SHADER "balls.vert"
#define FRAGMENT_SHADER "balls.frag"

enum { WIDTH = 640, HEIGHT = 480 };

void initGL(int argc, char *argv[]);
void initCL(void);
void initShaders(void);
char *readFile(const char *filename, size_t *size);
void compileShader(GLint shader);

static cl_context context;
static cl_command_queue queue;
static cl_kernel kernel;

int
main(int argc, char *argv[]) {
	initGL(argc, argv);

	initCL();

	clReleaseContext(context);
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
	cl_program prog;
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
