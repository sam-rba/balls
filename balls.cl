#define RADIUS 0.15f

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
