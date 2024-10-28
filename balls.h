typedef float float2 __attribute__ ((vector_size (2*sizeof(float))));

typedef struct {
	float2 min, max;
} Rectangle;

int isCollision(float2 p1, float r1, float2 p2, float r2);
Rectangle insetRect(Rectangle r, float n);

float randFloat(float lo, float hi);
float2 randPtInRect(Rectangle r);
float2 randVec(float xmin, float xmax, float ymin, float ymax);
