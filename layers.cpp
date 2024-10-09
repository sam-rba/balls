#include "balls.h"

static vector<Collision> completeGraph(vector<Ball *> balls);
static vector<Collision> matching(vector<Collision> edges);
static vector<Collision> diff(vector<Collision> a, vector<Collision> b);

vector<vector<Collision>>
partitionCollisions(vector<Ball *> balls) {
	vector<vector<Collision>> layers;

	vector<Collision> collisions = completeGraph(balls);
	while (!collisions.empty()) {
		cout << "\nadd layer\n";
		cout << "layers.size(): " << layers.size() << "\n";

		vector<Collision> layerCollisions = matching(collisions);
		cout << "layer collisions: ";
		for (Collision c : layerCollisions)
			cout << c << " ";
		cout << "\n";
		collisions = diff(collisions, layerCollisions);

		vector<Collision> thisLayer;
		for (Collision c : layerCollisions) {
			cout << "create node with " << c << "\n";
			thisLayer.push_back(c);
		}
		cout << "thisLayer.size(): " << thisLayer.size() << "\n";
		layers.push_back(thisLayer);
	}
	return layers;
}

static vector<Collision>
completeGraph(vector<Ball *> balls) {
	vector<Collision> edges;
	size_t i, j;

	for (i = 0; i < balls.size(); i++)
		for (j = i+1; j < balls.size(); j++)
			edges.push_back(Collision(balls.at(i), balls.at(j)));
	return edges;
}

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

static vector<Collision>
diff(vector<Collision> a, vector<Collision> b) {
	vector<Collision> diff;
	set_difference(a.begin(), a.end(),
		b.begin(), b.end(),
		inserter(diff, diff.begin()));
	return diff;
}
