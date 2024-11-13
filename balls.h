typedef struct {
	float x, y;
} Vector;

typedef struct {
	Vector min, max;
} Rect;

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

int readFile(const char *filename, char **contents, size_t *size);

Partition partitionCollisions(size_t nBalls);
void freePartition(Partition part);
void printPartition(Partition part);

int isCollision(Vector p1, float r1, Vector p2, float r2);
Rect insetRect(Rect r, float n);
Vector *noOverlapPositions(int n, Rect bounds, float radius);

float randFloat(float lo, float hi);
Vector randPtInRect(Rect r);
