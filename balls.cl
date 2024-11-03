#include "config.h"

#define G (9.81f / FPS / FPS)
#define DENSITY 1500.0f

int isCollision(float2 p1, float r1, float2 p2, float r2);
void setPosition(float2 *p1, float r1, float2 *p2, float r2);
void setVelocity(float2 p1, float2 *v1, float r1, float2 p2, float2 *v2, float r2);
float2 unitNorm(float2 v);
float fdot(float2 a, float2 b);
float len(float2 v);
float mass(float radius);
float volume(float radius);

__kernel void
move(__global float2 *positions, __global float2 *velocities) {
	size_t id;

	id = get_global_id(0);
	velocities[id].y -= G;
	positions[id] += velocities[id];
}

__kernel void
collideWalls(__global float2 *positions, __global float2 *velocities, __global float *radii) {
	size_t id;
	float2 p, v, min, max;
	float r;

	id = get_global_id(0);

	p = positions[id];
	v = velocities[id];
	r = radii[id];

	/* Set bounds. */
	min = -1.0f + r + FLT_EPSILON;
	max = 1.0f - r - FLT_EPSILON;

	/* Check for collision with bounds. */
	if (p.x <= min.x || p.x >= max.x) {
		p.x = clamp(p.x, min.x, max.x);
		v.x = -v.x;
	}
	if (p.y <= min.y || p.y >= max.y) {
		p.y = clamp(p.y, min.y, max.y);
		v.y = -v.y;
	}

	/* Write back. */
	positions[id] = p;
	velocities[id] = v;
}

__kernel void
collideBalls(
	__global size_t *ballIndices,
	__global float2 *positions,
	__global float2 *velocities,
	__global float *radii
) {
	size_t id, i1, i2;
	float2 p1, p2, v1, v2;
	float r1, r2;

	id = get_global_id(0);
	i1 = ballIndices[2*id];
	i2 = ballIndices[2*id+1];

	p1 = positions[i1];
	p2 = positions[i2];
	v1 = velocities[i1];
	v2 = velocities[i2];
	r1 = radii[i1];
	r2 = radii[i2];

	if (!isCollision(p1, r1, p2, r2))
		return;
	setPosition(&p1, r1, &p2, r2);
	setVelocity(p1, &v1, r1, p2, &v2, r2);

	positions[i1] = p1;
	positions[i2] = p2;
	velocities[i1] = v1;
	velocities[i2] = v2;
}

__kernel void
genVertices(__global float2 *positions, __global float *radii, __global float2 *vertices) {
	size_t ball, nsegs;
	float2 center;
	float r, theta;

	ball = get_group_id(0);
	center = positions[ball];
	r = radii[ball];

	nsegs = get_local_size(0)-2; /* Number of edge segments. */
	theta = 2.0f * M_PI_F * get_local_id(0) / nsegs;

	vertices[get_global_id(0)].x = center.x + r * cos(theta);
	vertices[get_global_id(0)].y = center.y + r * sin(theta);

	vertices[ball*get_local_size(0)] = center;
}

/* Return true if the two balls are colliding. */
int
isCollision(float2 p1, float r1, float2 p2, float r2) {
	float2 dist;
	float rhs;

	dist = p1 - p2;
	rhs = r1 + r2 + FLT_EPSILON;
	return (dist.x*dist.x + dist.y*dist.y) <= rhs*rhs;
}

/* Set the positions of two balls at the moment of collision. */
void
setPosition(float2 *p1, float r1, float2 *p2, float r2) {
	float2 mid, n;

	mid = (*p1 + *p2) / 2.0f;
	n = unitNorm(*p2 - *p1);
	*p1 = mid - (n*r1 + FLT_EPSILON);
	*p2 = mid + (n*r2 + FLT_EPSILON);
}

/* Set the velocities of two balls after collision. */
void
setVelocity(float2 p1, float2 *v1, float r1, float2 p2, float2 *v2, float r2) {
	float m1, m2;
	float2 dp, dv, j;

	m1 = mass(r1);
	m2 = mass(r2);

	dp = p2 - p1;
	dv = *v2 - *v1;
	j = dp * 2.0f * m1 * m2 * fdot(dv, dp) / ((r1+r2)*(r1+r2) * (m1+m2));

	*v1 = *v1 + j/m1;
	*v2 = *v2 - j/m2;
}

float2
unitNorm(float2 v) {
	return v / len(v);
}

float
fdot(float2 a, float2 b) {
	return a.x*b.x + a.y*b.y;
}

float
len(float2 v) {
	return sqrt(v.x*v.x + v.y*v.y);
}

float
mass(float radius) {
	return volume(radius) * DENSITY;
}

float
volume(float radius) {
	return 4.0 * M_PI_F * radius*radius*radius / 3.0;
}
