#define RADIUS 0.15f

float
min(float a, float b) {
	if (a < b)
		return a;
	return b;
}

float
max(float a, float b) {
	if (a > b)
		return a;
	return b;
}

float
clamp(float x, float lo, float hi) {
	return min(hi, max(x, lo));
}

__kernel void
move(__global float2 *positions, __global float2 *velocities) {
	size_t id;

	id = get_global_id(0);
	positions[id] += velocities[id];
}

__kernel void
collideWalls(__global float2 *positions, __global float2 *velocities) {
	float2 min, max, p, v;
	size_t id;

	/* Set bounds. */
	min.x = -1.0 + RADIUS;
	min.y = -1.0 + RADIUS;
	max.x = 1.0 - RADIUS;
	max.y = 1.0 - RADIUS;

	id = get_global_id(0);
	p = positions[id];
	v = velocities[id];

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
genVertices(__global float2 *positions, __global float2 *vertices) {
	size_t ball, nsegs;
	float2 center;
	float theta;

	ball = get_group_id(0);
	center = positions[ball];

	nsegs = get_local_size(0)-2; /* Number of edge segments. */
	theta = 2.0f * M_PI_F * get_local_id(0) / nsegs;

	vertices[get_global_id(0)].x = center.x + RADIUS * cos(theta);
	vertices[get_global_id(0)].y = center.y + RADIUS * sin(theta);

	vertices[ball*get_local_size(0)] = center;
}
