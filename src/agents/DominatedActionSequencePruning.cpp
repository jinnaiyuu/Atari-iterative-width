/*
 * DominatedActionSequencePruning.cpp
 *
 *  Created on: Mar 30, 2017
 *      Author: yuu
 */

#include "DominatedActionSequencePruning.hpp"

#include "SearchTree.hpp"
#include <random>
#include <algorithm>

DominatedActionSequencePruning::DominatedActionSequencePruning(
		Settings & settings, StellaEnvironment* _env) :
		DominatedActionSequenceDetection(settings, _env) {

}

DominatedActionSequencePruning::~DominatedActionSequencePruning() {
}

void DominatedActionSequencePruning::learnDominatedActionSequences(
		SearchTree* tree, int seqLength) {
	action_length = seqLength;

	// Initializations
	if (action_permutation.empty()) {
		for (int i = 0; i < PLAYER_A_MAX; ++i) {
			action_permutation.push_back((Action) i);
		}
		if (permutate_action) {
			std::random_shuffle(action_permutation.begin(),
					action_permutation.end());
		}
	}

	if (isUsefulActionSequence.empty()) {
		isUsefulActionSequence.resize(seqLength);
		for (int i = 1; i <= seqLength; ++i) {
			isUsefulActionSequence[i - 1].resize(num_sequences(i));
			isUsefulActionSequence[i - 1].assign(num_sequences(i), false);
		}
	}

	TreeNode* root = tree->get_root();
	for (int i = 1; i <= seqLength; ++i) {
		VertexCover dgraph(num_sequences(i)); //TODO
		dominance_graph.push_back(dgraph);
		searchNode(root, i, isUsefulActionSequence[i - 1]);
	}

	learnDASP(tree, seqLength);
}

std::vector<bool> DominatedActionSequencePruning::getEffectiveActions(
		std::vector<Action> previousActions, int current_frame) {
	if (junk_decision_frame > current_frame) {
		return vector<bool>(PLAYER_A_MAX, true);
	}

	std::vector<bool> available = isUsefulActionSequence[0];

	int length = min(previousActions.size() + 1, isUsefulActionSequence.size());

	// i <- the length of junk to check.
	for (int i = 2; i <= length; ++i) {
		std::vector<Action> p(previousActions.end() - i, previousActions.end());

		for (int j = 0; j < PLAYER_A_MAX; ++j) {
			std::vector<Action> sequence = p;
			sequence.push_back((Action) j);
			int seqInt = seqToInt(sequence);

			if (!isUsefulActionSequence[i - 1][seqInt]) {
				available[j] = false;
			}
		}

	}
	//	for (int j = 0; j < available.size(); ++j) {
	//		if (available[j]) {
	//			printf("o");
	//		} else {
	//			printf("x");
	//		}
	//	}
	//	printf("\n");
	return available;
}

int DominatedActionSequencePruning::getDetectedUsedActionsSize() {
	int n_used_actions = 0;
	if (action_length == 1) {
		std::vector<bool> available = isUsefulActionSequence[0];
		for (int a = 0; a < PLAYER_A_MAX; ++a) {
			if (available[a]) {
				++n_used_actions;
			}
		}
	}
	printf("DetectedUsedActions= %d\n", n_used_actions);
	return n_used_actions;
}

void DominatedActionSequencePruning::getUsedSequenceList(TreeNode* node,
		int seqLength, std::vector<bool>& isSequenceUsed) {

	int size = num_sequences(seqLength);
	std::vector<TreeNode*> nodeList;

	for (int i = 0; i < size; ++i) {
		vector<Action> seq = intToSeq(i, seqLength);
		for (int j = 0; j < seq.size(); ++j) {
			seq[j] = action_permutation[seq[j]];
		}
		TreeNode* child = getResultingNode(node, seq);
		nodeList.push_back(child);

	}

//	printf("\n");
	// TODO: this is wrong: we are adding nulls!
	for (int i = 0; i < size; ++i) {
		if (nodeList[i] == nullptr || nodeList[i]->is_terminal) {
			continue;
		}

		bool isDuplicate = false;
		for (int j = 0; j < i; ++j) {
			if (nodeList[j] == nullptr || nodeList[i]->is_terminal) {
				continue;
			}

			if (nodeList[i]->state.equals(nodeList[j]->state)) {
				isDuplicate = true;
				break;
			}
		}
		if (!isDuplicate) {
			int sInt = permutateToOriginalAction(i, seqLength);
			isSequenceUsed[sInt] = true;
		}
	}

// TODO: for now we only detect single action dominance.
//	if (seqLength == 1) {
	for (int i = 0; i < size; ++i) {
		if (nodeList[i] == nullptr || nodeList[i]->is_terminal) {
			continue;
		}
		int iInt = permutateToOriginalAction(i, seqLength);

		bool isDuplicate = false;
		for (int j = 0; j < size; ++j) {
			if (j == i) {
				continue;
			}
			if (nodeList[j] == nullptr || nodeList[i]->is_terminal) {
				continue;
			}

			if (nodeList[i]->state.equals(nodeList[j]->state)) {
				isDuplicate = true;
				int jInt = permutateToOriginalAction(j, seqLength);
				dominance_graph[seqLength - 1].addEdge(iInt, jInt);
				break;
			}
		}
		if (!isDuplicate) {
			dominance_graph[seqLength - 1].addNode(iInt);
		}
//		}
	}
}

void DominatedActionSequencePruning::learnDASP(SearchTree* tree,
		int seqLength) {

	if (permutate_action) {
		std::vector<bool> minset = dominance_graph[0].minimalActionSet();
		if (std::count(minset.begin(), minset.end(), true)
				< std::count(isUsefulActionSequence[0].begin(),
						isUsefulActionSequence[0].end(), true)) {
			printf("minset %d < %d\n",
					std::count(minset.begin(), minset.end(), true),
					std::count(isUsefulActionSequence[0].begin(),
							isUsefulActionSequence[0].end(), true));
			isUsefulActionSequence[0] = minset;

			// REFACTOR: hand-scripted stable_sort: Not sure how to implement using lambda.

			std::vector<Action> in;
			std::vector<Action> notin;
			for (int i = 0; i < action_permutation.size(); ++i) {
				if (isUsefulActionSequence[0][action_permutation[i]]) {
					in.push_back(action_permutation[i]);
				} else {
					notin.push_back(action_permutation[i]);
				}
			}
			in.insert(in.end(), notin.begin(), notin.end());
			action_permutation = in;
		}
		//		});
	}

	for (int i = 1; i <= seqLength; ++i) {
		printf("Non-dominated action sequence of length %d: ", i);
		for (int j = 0; j < isUsefulActionSequence[i - 1].size(); ++j) {
			if (isUsefulActionSequence[i - 1][j]) {
				printf("o");
			} else {
				printf("x");
			}
		}
		printf("\n");
	}
}
