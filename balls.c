#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>
#include <GL/glew.h>
#include <GL/glut.h>
#include <CL/cl_gl.h>

#include "balls.h"
#include "sysfatal.h"
#include "gl.h"

#define nelem(arr) (sizeof(arr) / sizeof(arr[0]))

enum {
	MS_PER_S = 1000,
	FRAME_TIME_MS = MS_PER_S / FPS,
 };

const Rect bounds = { {-1.0, -1.0}, {1.0, 1.0} };

void setPositions(void);
void setVelocities(void);
void setRadii(void);
void setCollisions(void);
void configSharedData(void);
void setKernelArgs(void);
void animate(int v);
void move(void);
void collideBalls(void);
cl_event collideWalls(void);
void genVertices(void);
void display(void);
void copyPositionsToGpu(cl_event cpuEvent);
void reshape(int w, int h);
void keyboard(unsigned char key, int x, int y);
void freeCL(void);
void initShaders(void);
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

	genBuffers(&vertexVAO, &vertexVBO, &colorVBO, nBalls);

	configSharedData();

	setKernelArgs();

	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
	glutKeyboardFunc(keyboard);
	glutTimerFunc(0, animate, 0);

	glutMainLoop();

	freeCL();
	freeGL(vertexVAO, vertexVBO, colorVBO);
	freePartition(collisionPartition);
	free(positionsHostBuf);

	return 0;
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
	positionsCpuBuf = clCreateBuffer(cpuContext, CL_MEM_READ_WRITE | CL_MEM_USE_HOST_PTR, nBalls*2*sizeof(float), positionsHostBuf, &err);
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
	cl_event cpuEvent;
	clock_t tstart, elapsed;
	unsigned int nextFrame;

	tstart = clock();

	/* Start computing next frame on CPU. */
	move();
	collideBalls();
	cpuEvent = collideWalls();

	/* Display current frame with GPU. */
	genVertices();
	display();

	/* Copy next frame's positions from CPU to GPU. */
	copyPositionsToGpu(cpuEvent);

	/* Display next frame. */
	elapsed = (clock() - tstart) / (CLOCKS_PER_SEC / MS_PER_S);
	nextFrame = (elapsed > FRAME_TIME_MS) ? 0 : FRAME_TIME_MS-elapsed;
	glutTimerFunc(nextFrame, animate, 0);
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
collideBalls(void) {
	int i, err;

	for (i = 0; i < collisionPartition.size; i++) {
		err = clSetKernelArg(collideBallsKernel, 0, sizeof(collisionsCpuBufs[i]), collisionsCpuBufs+i);
		if (err < 0)
			sysfatal("Failed to set argument of collideBalls kernel.\n");
		err = clEnqueueNDRangeKernel(cpuQueue, collideBallsKernel, 1, NULL, &collisionPartition.cells[i].size, NULL, 0, NULL, NULL);
		if (err < 0)
			sysfatal("Couldn't enqueue kernel.\n");
	}
}

cl_event
collideWalls(void) {
	size_t size;
	cl_event event;
	int err;

	size = nBalls;
	err = clEnqueueNDRangeKernel(cpuQueue, collideWallsKernel, 1, NULL, &size, NULL, 0, NULL, &event);
	if (err < 0)
		sysfatal("Couldn't enqueue kernel.\n");
	return event;
}

void
genVertices(void) {
	int err;
	size_t localSize, globalSize;
	cl_event kernelEvent;

	glFinish();

	err = clEnqueueAcquireGLObjects(gpuQueue, 1, &vertexGpuBuf, 0, NULL, NULL);
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

	clEnqueueReleaseGLObjects(gpuQueue, 1, &vertexGpuBuf, 0, NULL, NULL);
	clFinish(gpuQueue);
	clReleaseEvent(kernelEvent);
}

void
display(void) {
	int i;

	glClear(GL_COLOR_BUFFER_BIT |GL_DEPTH_BUFFER_BIT);

	glBindVertexArray(vertexVAO);
	for (i = 0; i < nBalls; i++)
		glDrawArrays(GL_TRIANGLE_FAN, i*CIRCLE_POINTS, CIRCLE_POINTS);
	glBindVertexArray(0);

	frameCount();

	glutSwapBuffers();
}

/* Wait for the CPU to finish computing the new positions and then copy them to the GPU. */
void
copyPositionsToGpu(cl_event cpuEvent) {
	int err;

	err = clWaitForEvents(1, &cpuEvent);
	if (err < 0)
		sysfatal("Error waiting for CPU kernel to finish.\n");
	clReleaseEvent(cpuEvent);
	err = clEnqueueWriteBuffer(gpuQueue, positionsGpuBuf, CL_TRUE, 0, nBalls*2*sizeof(float), positionsHostBuf, 0, NULL, NULL);
	if (err < 0)
		sysfatal("Failed to copy positions from host to GPU.\n");
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
freeCL(void) {
	size_t i;

	clReleaseMemObject(positionsCpuBuf);
	clReleaseMemObject(positionsGpuBuf);
	clReleaseMemObject(velocitiesCpuBuf);
	clReleaseMemObject(radiiCpuBuf);
	clReleaseMemObject(radiiGpuBuf);
	for (i = 0; i < collisionPartition.size; i++)
		clReleaseMemObject(collisionsCpuBufs[i]);
	free(collisionsCpuBufs);
	clReleaseMemObject(vertexGpuBuf);

	clReleaseKernel(moveKernel);
	clReleaseKernel(collideWallsKernel);
	clReleaseKernel(collideBallsKernel);
	clReleaseKernel(genVerticesKernel);

	clReleaseCommandQueue(cpuQueue);
	clReleaseCommandQueue(gpuQueue);
	clReleaseContext(cpuContext);
	clReleaseContext(gpuContext);
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
