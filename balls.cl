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
collideWalls(__global float2 *positions, __global float2 *velocities, __global float *radii) {
	size_t id;
	float2 p, v, min, max;
	float r;

	id = get_global_id(0);

	p = positions[id];
	v = velocities[id];
	r = radii[id];

	/* Set bounds. */
	min.x = -1.0 + r;
	min.y = -1.0 + r;
	max.x = 1.0 - r;
	max.y = 1.0 - r;

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
