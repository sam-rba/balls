#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <GL/glew.h>
#include <GL/glut.h>
#include <CL/cl_gl.h>
#ifndef WINDOWS
#include <GL/glx.h>
#endif

#include "sysfatal.h"
#include "balls.h"

#define nelem(arr) (sizeof(arr) / sizeof(arr[0]))
#ifdef WINDOWS
#define contextProperties(platform) { \
	CL_GL_CONTEXT_KHR, (cl_context_properties) wglGetCurrentContext(), \
	CL_WGL_HDC_KHR, (cl_context_properties) wglGetCurrentDC(), \
	CL_CONTEXT_PLATFORM, (cl_context_properties) (platform), \
	0 \
}
#else
#define contextProperties(platform) { \
	CL_GL_CONTEXT_KHR, (cl_context_properties) glXGetCurrentContext(), \
	CL_GLX_DISPLAY_KHR, (cl_context_properties) glXGetCurrentDisplay(), \
	CL_CONTEXT_PLATFORM, (cl_context_properties) (platform), \
	0 \
}
#endif

#define PROG_FILE "balls.cl"
#define MOVE_KERNEL_FUNC "move"
#define COLLIDE_WALLS_KERNEL_FUNC "collideWalls"
#define COLLIDE_BALLS_KERNEL_FUNC "collideBalls"
#define GEN_VERTICES_KERNEL_FUNC "genVertices"
#define VERTEX_SHADER "balls.vert"
#define FRAGMENT_SHADER "balls.frag"

#define RMIN 0.05 /* Minimum radius. */
#define RMAX 0.15 /* Maximum radius. */
#define VMAX_INIT 5.0 /* Maximum initial velocity. */

enum {
	WIDTH = 640,
	HEIGHT = 640,
};
enum {
	MS_PER_S = 1000,
	FRAME_TIME_MS = MS_PER_S / FPS,
 };
enum { KEY_QUIT = 'q' };
enum { NBALLS_DEFAULT = 3 };
enum { CIRCLE_POINTS = 24 }; /* Number of vertices per circle. */

const Rect bounds = { {-1.0, -1.0}, {1.0, 1.0} };

void initGL(int argc, char *argv[]);
void initCL(void);
int getDevicePlatform(cl_platform_id platforms[], int nPlatforms, cl_device_type devType, cl_device_id *device);
void printBuildLog(cl_program prog, cl_device_id device);
cl_kernel createKernel(cl_program prog, const char *kernelFunc);
void setPositions(void);
void setVelocities(void);
void setRadii(void);
void setCollisions(void);
void genBuffers(void);
void genVertexBuffer(void);
void setColors(void);
void configSharedData(void);
void setKernelArgs(void);
void animate(int v);
void display(void);
void reshape(int w, int h);
void keyboard(unsigned char key, int x, int y);
void move(void);
void collideWalls(void);
void collideBalls(void);
void genVertices(void);
void freeCL(void);
void freeGL(void);
void initShaders(void);
char *readFile(const char *filename, size_t *size);
void compileShader(GLint shader);
void frameCount(void);
void drawString(const char *str);
float *flatten(Vector *vs, int n);

int nBalls;
cl_context cpuContext, gpuContext;
cl_command_queue cpuQueue, gpuQueue;
cl_kernel moveKernel, collideWallsKernel, collideBallsKernel, genVerticesKernel;
GLuint vertexVAO, vertexVBO, colorVBO;
cl_mem positionsCpuBuf, positionsGpuBuf, velocitiesCpuBuf, radiiCpuBuf, radiiGpuBuf, *collisionsCpuBufs, vertexGpuBuf;
float *positionsHostBuf;
Partition collisionPartition;

int
main(int argc, char *argv[]) {
	nBalls = NBALLS_DEFAULT;
	if (argc > 1) {
		if (sscanf(argv[1], "%d", &nBalls) != 1 || nBalls < 1) {
			printf("usage: balls [number of balls]\n");
			return 1;
		}
	}

	initGL(argc, argv);

	initCL();

	setPositions();
	setVelocities();
	setRadii();
	setCollisions();

	genBuffers();
	setColors();
	configSharedData();

	setKernelArgs();

	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keyboard);
	glutTimerFunc(0, animate, 0);

	glutMainLoop();

	freeCL();
	freeGL();
	freePartition(collisionPartition);
	free(positionsHostBuf);

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
	cl_uint nPlatforms;
	cl_platform_id *platforms, cpuPlatform, gpuPlatform;
	size_t i;
	cl_device_id cpuDevice, gpuDevice;
	cl_int err;
	cl_program cpuProg, gpuProg;
	char *progBuf;
	size_t progSize;

	/* Get platforms. */
	if (clGetPlatformIDs(0, NULL, &nPlatforms) < 0)
		sysfatal("Can't get OpenCL platforms.\n");
	if ((platforms = malloc(nPlatforms*sizeof(cl_platform_id))) == NULL)
		sysfatal("Failed to allocate platform array.\n");
	if (clGetPlatformIDs(nPlatforms, platforms, NULL) < 0)
		sysfatal("Can't get OpenCL platforms.\n");

	/* Get CPU device. */
	i = getDevicePlatform(platforms, nPlatforms, CL_DEVICE_TYPE_CPU, &cpuDevice);
	if (i < 0)
		sysfatal("No CPU device available.\n");
	cpuPlatform = platforms[i];

	/* Get GPU device. */
	i = getDevicePlatform(platforms, nPlatforms, CL_DEVICE_TYPE_GPU, &gpuDevice);
	if (i < 0)
		sysfatal("No GPU device available.\n");
	gpuPlatform = platforms[i];

	/* Configure properties for OpenGL interoperability. */
	cl_context_properties cpuProperties[] = contextProperties(cpuPlatform);
	cl_context_properties gpuProperties[] = contextProperties(gpuPlatform);

	/* Create contexts. */
	cpuContext = clCreateContext(cpuProperties, 1, &cpuDevice, NULL, NULL, &err);
	if (err < 0)
		sysfatal("Failed to create CPU context.\n");
	gpuContext = clCreateContext(gpuProperties, 1, &gpuDevice, NULL, NULL, &err);
	if (err < 0)
		sysfatal("Failed to create GPU context.\n");

	free(platforms);

	/* Create program from file. */
	progBuf = readFile(PROG_FILE, &progSize);
	cpuProg = clCreateProgramWithSource(cpuContext, 1, (const char **) &progBuf, &progSize, &err);
	if (err < 0)
		sysfatal("Failed to create CPU program.\n");
	gpuProg = clCreateProgramWithSource(gpuContext, 1, (const char **) &progBuf, &progSize, &err);
	if (err < 0)
		sysfatal("Failed to create GPU program.\n");
	free(progBuf);

	/* Build program. */
	err = clBuildProgram(cpuProg, 0, NULL, "-I./", NULL, NULL);
	if (err < 0) {
		fprintf(stderr, "Failed to build CPU program.\n");
		printBuildLog(cpuProg, cpuDevice);
		exit(1);
	}
	err = clBuildProgram(gpuProg, 0, NULL, "-I./", NULL, NULL);
	if (err < 0) {
		/* Print build log. */
		fprintf(stderr, "Failed to build GPU program.\n");
		printBuildLog(gpuProg, gpuDevice);
		exit(1);
	}

	/* Create command queues. */
	cpuQueue = clCreateCommandQueue(cpuContext, cpuDevice, 0, &err);
	if (err < 0)
		sysfatal("Failed to create CPU command queue.\n");
	gpuQueue = clCreateCommandQueue(gpuContext, gpuDevice, 0, &err);
	if (err < 0)
		sysfatal("Failed to create GPU command queue.\n");

	/* Create kernels. */
	moveKernel = createKernel(cpuProg, MOVE_KERNEL_FUNC);
	collideWallsKernel = createKernel(cpuProg, COLLIDE_WALLS_KERNEL_FUNC);
	collideBallsKernel = createKernel(cpuProg, COLLIDE_BALLS_KERNEL_FUNC);
	genVerticesKernel = createKernel(gpuProg, GEN_VERTICES_KERNEL_FUNC);
}

/*
 * Find a platform with a certain type of device. Sets *device and returns the index
 * of the platform that it belongs to. Returns -1 if none of the platforms have the
 * specified type of device.
 */
int
getDevicePlatform(cl_platform_id platforms[], int nPlatforms, cl_device_type devType, cl_device_id *device) {
	int i, err;

	for (i = 0; i < nPlatforms; i++) {
		err = clGetDeviceIDs(platforms[i], devType, 1, device, NULL);
		if (err == CL_SUCCESS) {
			return i;
		}
	}
	return -1;
}

void
printBuildLog(cl_program prog, cl_device_id device) {
	size_t size;
	char *log;

	clGetProgramBuildInfo(prog, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &size);
	if ((log = malloc(size + 1)) == NULL)
		sysfatal("Failed to allocate program build log buffer.\n");
	log[size] = '\0';
	clGetProgramBuildInfo(prog, device, CL_PROGRAM_BUILD_LOG, size+1, log, NULL);
	fprintf(stderr, "%s\n", log);
	free(log);
}

cl_kernel
createKernel(cl_program prog, const char *kernelFunc) {
	cl_kernel kernel;
	int err;

	kernel = clCreateKernel(prog, kernelFunc, &err);
	if (err < 0)
		sysfatal("Failed to create kernel '%s': %d\n", kernelFunc, err);
	return kernel;
}

void
setPositions(void) {
	Vector *positions;
	int err;

	/* Generate initial ball positions. */
	positions = noOverlapPositions(nBalls, bounds, RMAX);
	positionsHostBuf = flatten(positions, nBalls);
	free(positions);

	/* Create CPU buffer. */
	positionsCpuBuf = clCreateBuffer(cpuContext, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, nBalls*2*sizeof(float), positionsHostBuf, &err);
	if (err < 0)
		sysfatal("Failed to allocate CPU position buffer.\n");

	/* Create GPU buffer. */
	positionsGpuBuf = clCreateBuffer(gpuContext, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, nBalls*2*sizeof(float), positionsHostBuf, &err);
	if (err < 0)
		sysfatal("Failed to allocate GPU position buffer.\n");
}

void
setVelocities(void) {
	float *velocitiesHostBuf;
	int i, err;

	/* Generate initial ball velocities. */
	if ((velocitiesHostBuf = malloc(nBalls*2*sizeof(float))) == NULL)
		sysfatal("Failed to allocate velocity array.\n");
	for (i = 0; i < nBalls; i++) {
		velocitiesHostBuf[2*i] = randFloat(-VMAX_INIT, VMAX_INIT);
		velocitiesHostBuf[2*i+1] = randFloat(-VMAX_INIT, VMAX_INIT);
	}

	/* Create device-side buffer. */
	velocitiesCpuBuf = clCreateBuffer(cpuContext, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR, nBalls*2*sizeof(float), velocitiesHostBuf, &err);
	if (err < 0)
		sysfatal("Failed to allocate velocity buffer.\n");

	free(velocitiesHostBuf);
}

void
setRadii(void) {
	float *radiiHostBuf;
	int i, err;

	/* Generate radii. */
	if ((radiiHostBuf = malloc(nBalls*sizeof(float))) == NULL)
		sysfatal("Failed to allocate radii array.\n");
	for (i = 0; i < nBalls; i++)
		radiiHostBuf[i] = randFloat(RMIN, RMAX);

	/* Create CPU buffer. */
	radiiCpuBuf = clCreateBuffer(cpuContext, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, nBalls*sizeof(float), radiiHostBuf, &err);
	if (err <0)
		sysfatal("Failed to allocate radii CPU buffer.\n");

	/* Create GPU buffer. */
	radiiGpuBuf = clCreateBuffer(gpuContext, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, nBalls*sizeof(float), radiiHostBuf, &err);
	if (err <0)
		sysfatal("Failed to allocate radii GPU buffer.\n");

	free(radiiHostBuf);
}

void
setCollisions(void) {
	int i, err;

	collisionPartition = partitionCollisions(nBalls);
	printf("Collision partition:\n");
	printPartition(collisionPartition);

	/* Allocate array of buffers. */
	if ((collisionsCpuBufs = malloc(collisionPartition.size*sizeof(cl_mem))) == NULL)
		sysfatal("Failed to allocate collision buffers.\n");
	for (i = 0; i < collisionPartition.size; i++) {
		/* Create device-side buffer. */
		collisionsCpuBufs[i] = clCreateBuffer(cpuContext, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, collisionPartition.cells[i].size*2*sizeof(size_t), collisionPartition.cells[i].ballIndices, &err);
		if (err < 0)
			sysfatal("Failed to allocate collision buffer.\n");
	}
}

/* Create GL vertex and color buffers. */
void
genBuffers(void) {
	glGenVertexArrays(1, &vertexVAO);
	glBindVertexArray(vertexVAO);

	/* Generate vertex buffer. */
	genVertexBuffer();

	/* Generate color buffer. */
	glGenBuffers(1, &colorVBO);
}

/* Generate GL vertex buffer. */
void
genVertexBuffer(void) {
	glGenBuffers(1, &vertexVBO);
	glBindBuffer(GL_ARRAY_BUFFER, vertexVBO);
	glBufferData(GL_ARRAY_BUFFER, nBalls*CIRCLE_POINTS*2*sizeof(GLfloat), NULL, GL_DYNAMIC_DRAW);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(0);
}

/* Set ball colors in the GL vertex color buffer. */
void
setColors(void) {
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

	glBindBuffer(GL_ARRAY_BUFFER, colorVBO);
	glBufferData(GL_ARRAY_BUFFER, nBalls*CIRCLE_POINTS*3*sizeof(GLfloat), colors, GL_STATIC_DRAW);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, 0);
	glEnableVertexAttribArray(1);

	free(colors);
}

/* Create CL memory object from vertex buffer. */
void
configSharedData(void) {
	int err;

	vertexGpuBuf = clCreateFromGLBuffer(gpuContext, CL_MEM_WRITE_ONLY, vertexVBO, &err);
	if (err < 0)
		sysfatal("Failed to create buffer object from VBO.\n");
}

void
setKernelArgs(void) {
	int err;

	err = clSetKernelArg(moveKernel, 0, sizeof(positionsCpuBuf), &positionsCpuBuf);
	err |= clSetKernelArg(moveKernel, 1, sizeof(velocitiesCpuBuf), &velocitiesCpuBuf);

	err |= clSetKernelArg(collideWallsKernel, 0, sizeof(positionsCpuBuf), &positionsCpuBuf);
	err |= clSetKernelArg(collideWallsKernel, 1, sizeof(velocitiesCpuBuf), &velocitiesCpuBuf);
	err |= clSetKernelArg(collideWallsKernel, 2, sizeof(radiiCpuBuf), &radiiCpuBuf);

	err |= clSetKernelArg(collideBallsKernel, 1, sizeof(positionsCpuBuf), &positionsCpuBuf);
	err |= clSetKernelArg(collideBallsKernel, 2, sizeof(velocitiesCpuBuf), &velocitiesCpuBuf);
	err |= clSetKernelArg(collideBallsKernel, 3, sizeof(radiiCpuBuf), &radiiCpuBuf);

	err |= clSetKernelArg(genVerticesKernel, 0, sizeof(positionsGpuBuf), &positionsGpuBuf);
	err |= clSetKernelArg(genVerticesKernel, 1, sizeof(radiiCpuBuf), &radiiGpuBuf);
	err |= clSetKernelArg(genVerticesKernel, 2, sizeof(vertexGpuBuf), &vertexGpuBuf);

	if (err < 0)
		sysfatal("Failed to set kernel arguments.\n");
}

void
animate(int v) {
	cl_event readEvent;
	int err;
	clock_t tstart, elapsed;
	unsigned int nextFrame;

	tstart = clock();

	move();
	collideBalls();
	collideWalls();

	/* Copy new positions from CPU to host asynchronously. */
	err = clEnqueueReadBuffer(cpuQueue, positionsCpuBuf, CL_FALSE, 0, nBalls*2*sizeof(float), positionsHostBuf, 0, NULL, &readEvent);
	if (err < 0)
		sysfatal("Failed to copy positions from CPU to host.\n");

	display();

	/* Copy new positions from host to GPU. */
	err = clEnqueueWriteBuffer(gpuQueue, positionsGpuBuf, CL_TRUE, 0, nBalls*2*sizeof(float), positionsHostBuf, 1, &readEvent, NULL);
	if (err < 0)
		sysfatal("Failed to copy positions from host to GPU.\n");

	clReleaseEvent(readEvent);

	elapsed = (clock() - tstart) / (CLOCKS_PER_SEC / MS_PER_S);
	nextFrame = (elapsed > FRAME_TIME_MS) ? 0 : FRAME_TIME_MS-elapsed;
	glutTimerFunc(nextFrame, animate, 0);
}

void
display(void) {
	int i;

	glClear(GL_COLOR_BUFFER_BIT |GL_DEPTH_BUFFER_BIT);

	genVertices();

	glBindVertexArray(vertexVAO);
	for (i = 0; i < nBalls; i++)
		glDrawArrays(GL_TRIANGLE_FAN, i*CIRCLE_POINTS, CIRCLE_POINTS);
	glBindVertexArray(0);

	frameCount();

	glutSwapBuffers();
}

void
reshape(int w, int h) {
	glViewport(0, 0, (GLsizei) w, (GLsizei) h);
}

void
keyboard(unsigned char key, int x, int y) {
	if (key == KEY_QUIT)
		glutDestroyWindow(glutGetWindow());
}

void
move(void) {
	size_t size;
	int err;

	size = nBalls;
	err = clEnqueueNDRangeKernel(cpuQueue, moveKernel, 1, NULL, &size, NULL, 0, NULL, NULL);
	if (err < 0)
		sysfatal("Couldn't enqueue kernel.\n");
}

void
collideWalls(void) {
	size_t size;
	int err;

	size = nBalls;
	err = clEnqueueNDRangeKernel(cpuQueue, collideWallsKernel, 1, NULL, &size, NULL, 0, NULL, NULL);
	if (err < 0)
		sysfatal("Couldn't enqueue kernel.\n");
}

void
collideBalls(void) {
	int i, err;

	for (i = 0; i < collisionPartition.size; i++) {
		err = clSetKernelArg(collideBallsKernel, 0, sizeof(collisionsCpuBufs[i]), collisionsCpuBufs+i);
		if (err < 0)
			sysfatal("Failed to set argument of %s kernel.\n", COLLIDE_BALLS_KERNEL_FUNC);
		err = clEnqueueNDRangeKernel(cpuQueue, collideBallsKernel, 1, NULL, &collisionPartition.cells[i].size, NULL, 0, NULL, NULL);
		if (err < 0)
			sysfatal("Couldn't enqueue kernel.\n");
	}
}

void
genVertices(void) {
	int err;
	size_t localSize, globalSize;
	cl_event kernelEvent;

	glFinish();

	err = clEnqueueAcquireGLObjects(gpuQueue, 1, &vertexBuf, 0, NULL, NULL);
	if (err < 0)
		sysfatal("Couldn't acquire the GL objects.\n");

	localSize = CIRCLE_POINTS;
	globalSize = nBalls * localSize;
	err = clEnqueueNDRangeKernel(gpuQueue, genVerticesKernel, 1, NULL, &globalSize, &localSize, 0, NULL, &kernelEvent);
	if (err < 0)
		sysfatal("Couldn't enqueue kernel.\n");

	err = clWaitForEvents(1, &kernelEvent);
	if (err < 0)
		sysfatal("Couldn't enqueue the kernel.\n");

	clEnqueueReleaseGLObjects(gpuQueue, 1, &vertexBuf, 0, NULL, NULL);
	clFinish(queue);
	clReleaseEvent(kernelEvent);
}

void
freeCL(void) {
	size_t i;

	clReleaseMemObject(positions);
	clReleaseMemObject(velocities);
	clReleaseMemObject(radii);
	for (i = 0; i < collisionPartition.size; i++)
		clReleaseMemObject(collisions[i]);
	free(collisions);
	clReleaseMemObject(vertexBuf);

	clReleaseKernel(moveKernel);
	clReleaseKernel(collideWallsKernel);
	clReleaseKernel(collideBallsKernel);
	clReleaseKernel(genVerticesKernel);

	clReleaseCommandQueue(queue);
	clReleaseProgram(prog);
	clReleaseContext(context);
}

void
freeGL(void) {
	glDeleteBuffers(1, &vertexVBO);
	glDeleteBuffers(1, &vertexVAO);
	glDeleteBuffers(1, &colorVBO);
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
frameCount(void) {
	static int fps = 0;
	static int nFrames = 0;
	static time_t t0 = 0;
	static char str[16];
	time_t t1;

	t1 = time(NULL);
	if (t1 > t0) {
		fps = nFrames;
		nFrames = 0;
		t0 = t1;
	}

	snprintf(str, nelem(str), "%d FPS", fps);
	drawString(str);

	nFrames++;
}

void
drawString(const char *str) {
	size_t i, n;

	glColor3f(0, 0, 0);
	glRasterPos2f(-0.9, 0.9);

	n = strlen(str);
	for (i = 0; i < n; i++)
		glutBitmapCharacter(GLUT_BITMAP_8_BY_13, str[i]);
}

/*
 * Flatten an array of n vectors into an array of 2n floats. vs[i].x is at
 * position 2i+0, and vs[i].y is at position 2i+1 in the returned array.
 */
float *
flatten(Vector *vs, int n) {
	float *arr;

	if ((arr = malloc(2*n*sizeof(float))) == NULL)
		sysfatal("Failed to allocate vector array.\n");
	while (n-- > 0) {
		arr[2*n+0] = vs[n].x;
		arr[2*n+1] = vs[n].y;
	}
	return arr;
}
