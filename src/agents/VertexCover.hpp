/*
 * VertexCover.h
 *
 *  Created on: Sep 8, 2016
 *      Author: yuu
 */

#ifndef SRC_AGENTS_VERTEXCOVER_HPP_
#define SRC_AGENTS_VERTEXCOVER_HPP_

#include <list>
#include <vector>

// This class represents a undirected graph using adjacency list
class VertexCover {
	int V; // No. of vertices
	std::vector<std::vector<bool>> adj; // Pointer to an array containing adjacency lists
	std::vector<bool> marked; // Marked vertex is forced to be in the cover.
public:
	VertexCover(int V); // Constructor
	void addEdge(int v, int w); // function to add an edge to graph
	void addNode(int v);
	void printVertexCover(); // prints vertex cover
	std::vector<bool> uniqueActionSet();
	std::vector<bool> minimalActionSet();
};
#endif /* SRC_AGENTS_VERTEXCOVER_HPP_ */
