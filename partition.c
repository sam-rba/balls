#include <stdlib.h>

#include "sysfatal.h"
#include "balls.h"

/*
 * A graph on the set of balls. Nodes are ball indices and edges are potential
 * collisions between two balls.
 */
typedef struct {
	size_t (*edges)[2]; /* Array of edges. Each edge endpoint is a ball index. */
	size_t nEdges; /* Length of edge array. */
} Graph;

static Partition newPartition(void);
static Graph completeGraph(size_t nBalls);
static Graph matching(Graph g);
static Graph addEdge(Graph g, size_t edge[2]);
static Graph removeEdges(Graph g, Graph edges);
static Graph removeEdge(Graph g, size_t edge[2]);
static Partition addCell(Partition part, Graph cell);
static Graph newGraph(void);
static void freeGraph(Graph g);

/*
 * Partition the set of all possible collisions between pairs of balls into
 * chunks that can be computed concurrently. Collisions within a cell of the
 * partition can run concurrently. Cells must run sequentially.
 */
Partition
partitionCollisions(size_t nBalls) {
	Partition part;
	Graph graph, cell;

	part = newPartition();
	graph = completeGraph(nBalls);
	while (graph.nEdges > 0) {
		cell = matching(graph);
		graph = removeEdges(graph, cell);
		part = addCell(part, cell);
	}

	freeGraph(graph);

	return part;
}

void
freePartition(Partition part) {
	while (part.size-- > 0)
		free(part.cells[part.size].ballIndices);
	free(part.cells);
}

/* Allocate an empty partition. Partition should be freed by the caller after use. */
static Partition
newPartition(void) {
	Partition part;

	if ((part.cells = malloc(0)) == NULL)
		sysfatal("Failed to allocate partition.\n");
	part.size = 0;

	return part;
}

/* Create a complete graph on the set of balls. */
Graph
completeGraph(size_t nBalls) {
	Graph g;
	size_t e, i, j;

	g.nEdges = nBalls*(nBalls-1)/2;
	if ((g.edges = malloc(g.nEdges * 2*sizeof(size_t))) == NULL)
		sysfatal("Failed to allocate graph.\n");

	e = 0;
	for (i = 0; i < nBalls; i++) {
		for (j = i+1; j < nBalls; j++) {
			g.edges[e][0] = i;
			g.edges[e++][1] = j;
		}
	}

	return g;
}

/*
 * Find and return an independent edge set in g. Two edges are independent if
 * they don't share any vertices.
 */
Graph
matching(Graph g) {
	Graph match;
	size_t e, m;

	match = newGraph();
	for (e = 0; e < g.nEdges; e++) {
		for (m = 0; m < match.nEdges; m++)
			if (g.edges[e][0] == match.edges[m][0]
				|| g.edges[e][0] == match.edges[m][1]
				|| g.edges[e][1] == match.edges[m][0]
				|| g.edges[e][1] == match.edges[m][1]
			)
				break;
		if (m == match.nEdges) /* No shared vertices. */
			match = addEdge(match, g.edges[e]);
	}

	return match;
}

Graph
addEdge(Graph g, size_t edge[2]) {
	if ((g.edges = realloc(g.edges, (g.nEdges+1)*2*sizeof(size_t))) == NULL)
		sysfatal("Failed to add edge to graph.\n");
	g.edges[g.nEdges][0] = edge[0];
	g.edges[g.nEdges][1] = edge[1];
	g.nEdges++;
	return g;
}

static Graph
removeEdges(Graph g, Graph edges) {
	size_t i;

	for (i = 0; i < edges.nEdges; i++)
		g = removeEdge(g, edges.edges[i]);
	return g;
}

Graph
removeEdge(Graph g, size_t edge[2]) {
	size_t i, j;

	for (i = 0; i < g.nEdges; i++) {
		if (g.edges[i][0] == edge[0] && g.edges[i][1] == edge[1]) {
			/* Shift left. */
			for (j = i+1; j < g.nEdges; j++) {
				g.edges[j-1][0] = g.edges[j][0];
				g.edges[j-1][1] = g.edges[j][1];
			}

			/* Shrink array. */
			if ((g.edges = realloc(g.edges, (g.nEdges-1)*2*sizeof(size_t))) == NULL)
				sysfatal("Failed to reallocate graph edge array.\n");
			g.nEdges--;

			break;
		}
	}

	return g;
}

Partition
addCell(Partition part, Graph cell) {
	if ((part.cells = realloc(part.cells, (part.size+1)*2*sizeof(size_t))) == NULL)
		sysfatal("Failed to grow partition array.\n");
	part.cells[part.size].ballIndices = cell.edges;
	part.cells[part.size].size = cell.nEdges;
	part.size++;
	return part;
}

static Graph
newGraph(void) {
	Graph g;

	if ((g.edges = malloc(0)) == NULL)
		sysfatal("Failed to allocate graph.\n");
	g.nEdges = 0;
	return g;
}

static void
freeGraph(Graph g) {
	free(g.edges);
}
