#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <CL/cl_gl.h>
#ifndef WINDOWS
#include <GL/glx.h>
#endif

#include "balls.h"
#include "sysfatal.h"

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

static int getDevicePlatform(cl_platform_id platforms[], int nPlatforms, cl_device_type devType, cl_device_id *device);
static void printPlatform(cl_platform_id platform);
static void printDevice(cl_device_id device);
static void printBuildLog(cl_program prog, cl_device_id device);
static cl_kernel createKernel(cl_program prog, const char *kernelFunc);

extern cl_context cpuContext, gpuContext;
extern cl_command_queue cpuQueue, gpuQueue;
extern cl_kernel moveKernel, collideWallsKernel, collideBallsKernel, genVerticesKernel;

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
	printf("CPU platform: ");
	printPlatform(cpuPlatform);
	printf("CPU device: ");
	printDevice(cpuDevice);

	/* Get GPU device. */
	i = getDevicePlatform(platforms, nPlatforms, CL_DEVICE_TYPE_GPU, &gpuDevice);
	if (i < 0)
		sysfatal("No GPU device available.\n");
	gpuPlatform = platforms[i];
	printf("GPU platform: ");
	printPlatform(gpuPlatform);
	printf("GPU device: ");
	printDevice(gpuDevice);

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
	err = readFile(PROG_FILE, &progBuf, &progSize);
	if (err != 0)
		sysfatal("Failed to read %s\n", PROG_FILE);
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

	clReleaseProgram(cpuProg);
	clReleaseProgram(gpuProg);
}

/*
 * Find a platform with a certain type of device. Sets *device and returns the index
 * of the platform that it belongs to. Returns -1 if none of the platforms have the
 * specified type of device.
 */
static int
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

static void
printPlatform(cl_platform_id platform) {
	int err;
	size_t size;
	char *buf;

	/* Get size of string. */
	err = clGetPlatformInfo(platform, CL_PLATFORM_NAME, 0, NULL, &size);
	if (err < 0) {
		printf("ERROR getting platform name\n");
		return;
	}

	if ((buf = malloc(size+1)) == NULL) {
		printf("ERROR allocating buffer\n");
		return;
	}

	/* Get platform name. */
	err = clGetPlatformInfo(platform, CL_PLATFORM_NAME, size, buf, NULL);
	if (err < 0) {
		printf("ERROR getting platform name\n");
		return;
	}
	buf[size] = '\0';
	printf("%s\n", buf);
	free(buf);
}

static void
printDevice(cl_device_id device) {
	int err;
	size_t size;
	char *buf;
	cl_bool available;

	/* Get size of string. */
	err = clGetDeviceInfo(device, CL_DEVICE_NAME, 0, NULL, &size);
	if (err < 0) {
		printf("ERROR getting device name\n");
		return;
	}

	if ((buf = malloc(size+1)) == NULL) {
		printf("ERROR allocating buffer\n");
		return;
	}

	/* Get device name. */
	err = clGetDeviceInfo(device, CL_DEVICE_NAME, size, buf, NULL);
	if (err < 0) {
		printf("ERROR getting device nam\n");
		return;
	}
	buf[size] = '\0';
	printf("%s ", buf);
	free(buf);

	/* Get availability. */
	err = clGetDeviceInfo(device, CL_DEVICE_AVAILABLE, sizeof(available), &available, NULL);
	if (err < 0) {
		printf("ERROR getting device availability\n");
		return;
	}
	printf("(%savailable)\n", (!available) ? "un" : "");
}

static void
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

static cl_kernel
createKernel(cl_program prog, const char *kernelFunc) {
	cl_kernel kernel;
	int err;

	kernel = clCreateKernel(prog, kernelFunc, &err);
	if (err < 0)
		sysfatal("Failed to create kernel '%s': %d\n", kernelFunc, err);
	return kernel;
}
