typedef float float2 __attribute__ ((vector_size (2*sizeof(float))));

typedef struct {
	float2 min, max;
} Rectangle;

/*
 * A partition of the set of all possible collisions between pairs of balls.
 * Collisions within a cell of the partition can run concurrently.  Cells must
 * run sequentially.
*/
typedef struct {
	struct cell {
		size_t (*ballIndices)[2]; /* Array of pairs of ball indices. */
		size_t size; /* Length of ballIndices. */
	} *cells; /* Array of cells. */
	size_t size; /* Length of cell array. */
} Partition;

Partition partitionCollisions(size_t nBalls);
void freePartition(Partition part);
void printPartition(Partition part);

int isCollision(float2 p1, float r1, float2 p2, float r2);
Rectangle insetRect(Rectangle r, float n);

float randFloat(float lo, float hi);
float2 randPtInRect(Rectangle r);
float2 randVec(float xmin, float xmax, float ymin, float ymax);
