/*
 * ActionSequenceDetection.cpp
 *
 *  Created on: Aug 11, 2016
 *      Author: yuu
 */

#include "DominatedActionSequenceDetection.hpp"

#include "SearchTree.hpp"
#include <random>
#include <algorithm>

DominatedActionSequenceDetection::DominatedActionSequenceDetection(
		Settings & settings, StellaEnvironment* _env) :
		m_env(_env) {
	junk_decision_frame = settings.getInt("junk_decision_frame", false);

	if (junk_decision_frame < 0) {
		junk_decision_frame = 300; // 5 seconds in game
	}

	permutate_action = settings.getBool("permutate_action", false);
	action_length = 1;
}

DominatedActionSequenceDetection::~DominatedActionSequenceDetection() {
}



std::vector<Action> DominatedActionSequenceDetection::sortByNovelty(
		vector<bool> marked, vector<bool> minset) {
	std::vector<Action> unique;
	std::vector<Action> cover;
	std::vector<Action> notin;
	for (int i = 0; i < action_permutation.size(); ++i) {
		if (marked[action_permutation[i]]) {
			unique.push_back(action_permutation[i]);
		} else if (minset[action_permutation[i]]) {
			cover.push_back(action_permutation[i]);
		} else {
			notin.push_back(action_permutation[i]);
		}
	}
	unique.insert(unique.end(), cover.begin(), cover.end());
	unique.insert(unique.end(), notin.begin(), notin.end());
	return unique;
}

// Apply recursive call
void DominatedActionSequenceDetection::searchNode(TreeNode* node, int seqLength,
		std::vector<bool>& isSequenceUsed) {
	assert(num_sequences(seqLength) == isSequenceUsed.size());
// 1. get actionList for the node
	getUsedSequenceList(node, seqLength, isSequenceUsed);

// 2. recursively call this for child nodes
	NodeList childList = sortNodeList(node->v_children);
	for (int i = 0; i < childList.size(); ++i) {
		searchNode(childList[i], seqLength, isSequenceUsed);
	}
}

int DominatedActionSequenceDetection::seqToInt(std::vector<Action> sequence) {
	int ret = 0;
	for (int i = 0; i < sequence.size(); ++i) {
		ret = ret * PLAYER_A_MAX + (int) sequence[i];
	}
	return ret;
}

std::vector<Action> DominatedActionSequenceDetection::intToSeq(int seqInt,
		int seqLength) {
	std::vector<Action> sequence;
	sequence.resize(seqLength);
	int rest = seqInt;

	for (int i = seqLength - 1; i >= 0; --i) {
		sequence[i] = (Action) (rest % PLAYER_A_MAX);
		rest = rest / PLAYER_A_MAX;
	}
	return sequence;
}

bool nodeActionSort(const TreeNode* l, const TreeNode* r) {
	return l->act < r->act;
}

NodeList DominatedActionSequenceDetection::sortNodeList(NodeList childList) {
	NodeList sortedList = childList;
	std::sort(sortedList.begin(), sortedList.end(), nodeActionSort);
	return sortedList;
}

TreeNode * DominatedActionSequenceDetection::getResultingNode(TreeNode * root,
		std::vector<Action> sequence) {
	TreeNode* curr = root;
	int currPath = 0;

	while (currPath < sequence.size()) {
		if (curr->is_leaf()) {
			return nullptr;
		}
		NodeList r = curr->v_children;
		assert(r.size() > 0);
		assert(0 <= sequence[currPath] && sequence[currPath] < PLAYER_A_MAX);

		bool found = false;
		for (int i = 0; i < r.size(); ++i) {
			if (r[i]->act == sequence[currPath]) {
				curr = r[i];
				currPath++;
				found = true;
				break;
			}
		}
		if (!found) {
			return nullptr;
		}
	}

	if (currPath == sequence.size()) {
		return curr;
	} else {

		return nullptr;
	}
}

// number of possible action sequences
int DominatedActionSequenceDetection::num_sequences(int seqLength) const {
	if (seqLength == 0) {
		return 0;
	}
	int size = 1;
	for (int i = 0; i < seqLength; ++i) {
		size *= PLAYER_A_MAX;
	}
	return size;
}

// Action permutation
int DominatedActionSequenceDetection::permutateToOriginalAction(int input,
		int seqLength) {
	if (!permutate_action) {
		return input;
	}
	vector<Action> seq = intToSeq(input, seqLength);
	for (int j = 0; j < seq.size(); ++j) {
		for (int p = 0; p < PLAYER_A_MAX; ++p) {
			if (action_permutation[p] == seq[j]) {
				seq[j] = (Action) p;
				break;
			}
		}
	}
	return seqToInt(seq);
}
