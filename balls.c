#define CL_TARGET_OPENCL_VERSION 110

#include <stdlib.h>
#include <stdio.h>
#include <GL/glut.h>
#include <GL/glx.h>
#include <CL/cl_gl.h>

#include "sysfatal.h"

#define PROG_FILE "balls.cl"
#define KERNEL_FUNC "balls"

enum { WIDTH = 640, HEIGHT = 480 };

void initGL(int argc, char *argv[]);
void initCL(void);
char *readFile(const char *filename, size_t *size);

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
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA);
	glutInitWindowSize(WIDTH, HEIGHT);
	glutCreateWindow("Balls");
	glClearColor(1, 1, 1, 1);
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
configSharedData(void) {

}

void
execKernel(void) {

}

void
display(void) {

}
