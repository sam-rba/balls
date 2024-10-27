#define RADIUS 0.75f

__kernel void
balls(__global float2 *position, __global float2 *vertices) {
	size_t id, nsegs;
	float theta;

	position[0].x = 0.0f;
	position[0].y = 0.0f;

	/* Center of circle. */
	vertices[0].x = position[0].x;
	vertices[0].y = position[0].y;

	id = get_global_id(0);
	nsegs = get_global_size(0)-1;
	theta = 2.0f * M_PI_F * id / nsegs;
	vertices[id+1].x = position[0].x + RADIUS * cos(theta);
	vertices[id+1].y = position[0].y + RADIUS * sin(theta);
}
