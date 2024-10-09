#include "balls.h"

static vector<Collision> completeGraph(vector<Ball *> balls);
static vector<Collision> matching(vector<Collision> edges);
static vector<Collision> diff(vector<Collision> a, vector<Collision> b);

/*
 * Partition the set of all possible collisions between balls into chunks that
 * can be computed concurrently. Collisions within a cell of the partition can
 * run concurrently. Cells must run sequentially.
 */
vector<vector<Collision>>
partitionCollisions(vector<Ball *> balls) {
	vector<vector<Collision>> partition;

	vector<Collision> edges = completeGraph(balls);
	while (!edges.empty()) {
		vector<Collision> cell = matching(edges); /* collisions can run concurrently if they don't share any vertices (balls) */

		cout << "cell collisions: ";
		for (Collision c : cell)
			cout << c << " ";
		cout << "\n";

		edges = diff(edges, cell);
		partition.push_back(cell);
	}
	return partition;
}

/* Construct a complete graph on a set of balls (nodes) where collisions are edges. */
static vector<Collision>
completeGraph(vector<Ball *> balls) {
	vector<Collision> edges;
	size_t i, j;

	for (i = 0; i < balls.size(); i++)
		for (j = i+1; j < balls.size(); j++)
			edges.push_back(Collision(balls.at(i), balls.at(j)));
	return edges;
}

/* Find an independent edge set. Two edges are independent if they don't share any vertices. */
static vector<Collision>
matching(vector<Collision> edges) {
	vector<Collision> matching;
	size_t i;

	for (Collision e : edges) {
		for (i = 0; i < matching.size(); i++)
			if (e.b1 == matching[i].b1 || e.b1 == matching[i].b2 || e.b2 == matching[i].b1 || e.b2 == matching[i].b2)
				break;
		if (i == matching.size()) /* no shared vertices */
			matching.push_back(e);
	}
	return matching;
}

/* Difference of sets. */
static vector<Collision>
diff(vector<Collision> a, vector<Collision> b) {
	vector<Collision> diff;
	set_difference(a.begin(), a.end(),
		b.begin(), b.end(),
		inserter(diff, diff.begin()));
	return diff;
}
