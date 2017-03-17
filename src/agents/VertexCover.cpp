/*
 * VertexCover.cpp
 *
 *  Created on: Sep 8, 2016
 *      Author: yuu
 */

#include "VertexCover.hpp"
#include "Constants.h"


VertexCover::VertexCover() {
}

VertexCover::VertexCover(int V) {
	this->V = V;
	adj.resize(V);
	for (int i = 0; i < V; ++i) {
		adj[i].insert(adj[i].begin(), V, false);

	}

	marked.insert(marked.begin(), V, false);
}

void VertexCover::addEdge(int v, int w) {
	adj[v][w] = true; // Add w to v’s list.
	adj[w][v] = true; // Since the graph is undirected
}

void VertexCover::addNode(int v) {
	marked[v] = true;
}

std::vector<bool> VertexCover::uniqueActionSet() {
	return marked;
}

std::vector<bool> VertexCover::minimalActionSet() {
	std::vector<bool> visited = marked;
//	for (int i = 0; i < V; i++)
//		visited[i] = marked[i];

//	std::list<int>::iterator i;

	// Consider all edges one by one
	for (int u = 0; u < V; u++) {
		// An edge is only picked when both visited[u] and visited[v]
		// are false
		if (visited[u] == false) {
			// Go through all adjacents of u and pick the first not
			// yet visited vertex (We are basically picking an edge
			// (u, v) from remaining edges.
			for (int i = 0; i < V; i++) {
				if (!adj[u][i]) {
					continue;
				}
				if (visited[i] == false) {
					// Add the vertices (u, v) to the result set.
					// We make the vertex u and v visited so that
					// all edges from/to them would be ignored
					visited[i] = true;
					visited[u] = true;
					break;
				}
			}
		}
	}
	return visited;
}

// The function to print vertex cover
void VertexCover::printVertexCover() {
	std::vector<bool> visited = minimalActionSet();

	printf("VertexCover:");
	for (int i = 0; i < V; i++)
		if (visited[i])
			printf("%s ", action_to_string((Action) i).c_str());
	printf("\n");
}
