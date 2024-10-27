#define RADIUS 0.75

__kernel void
balls(__global float4 *vertices, float tick) {
	uint id;
	int longitude, latitude;
	float sign, phi, theta;

	id = get_global_id(0);

	longitude = id / 16;
	latitude = id % 16;

	sign = -2.0f * (longitude % 2) + 1.0f;
	phi = 2.0f * M_PI_F * longitude / 16 + tick;
	theta = M_PI_F * latitude / 16;

	vertices[id].x = RADIUS * sin(theta) * cos(phi);
	vertices[id].y = RADIUS * sign * cos(theta);
	vertices[id].z = RADIUS * sin(theta) * sin(phi);
	vertices[id].w = 1.0f;
}
