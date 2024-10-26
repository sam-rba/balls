__kernel void
balls(__global float4 *coords1, __global float4 *colors1,
	__global float4 *coords2, __global float4 *colors2,
	__global float4 *coords3, __global float4 *colors3
) {
	coords1[0] = (float4)(-0.15f, -0.15f,  1.00f, -0.15f);
	coords1[1] = (float4)( 0.15f,  1.00f,  0.15f,  0.15f);
	coords1[2] = (float4)( 1.00f,  0.15f, -0.15f,  1.00f);
	
	colors1[0] = (float4)(0.00f, 0.00f, 0.00f, 0.25f);
	colors1[1] = (float4)(0.00f, 0.00f, 0.50f, 0.00f);
	colors1[2] = (float4)(0.00f, 0.75f, 0.00f, 0.00f);
	
	coords2[0] = (float4)(-0.30f, -0.30f,  0.00f, -0.30f);
	coords2[1] = (float4)( 0.30f,  0.00f,  0.30f,  0.30f);
	coords2[2] = (float4)( 0.00f,  0.30f, -0.30f,  0.00f);
	
	colors2[0] = (float4)(0.00f, 0.00f, 0.00f, 0.00f);
	colors2[1] = (float4)(0.25f, 0.00f, 0.00f, 0.50f);
	colors2[2] = (float4)(0.00f, 0.00f, 0.75f, 0.00f);
	
	coords3[0] = (float4)(-0.45f, -0.45f, -1.00f, -0.45f);
	coords3[1] = (float4)( 0.45f, -1.00f,  0.45f,  0.45f);
	coords3[2] = (float4)(-1.00f,  0.45f, -0.45f, -1.00f);
	
	colors3[0] = (float4)(0.00f, 0.00f, 0.00f, 0.00f);
	colors3[1] = (float4)(0.00f, 0.25f, 0.00f, 0.00f);
	colors3[2] = (float4)(0.50f, 0.00f, 0.00f, 0.75f);
}
