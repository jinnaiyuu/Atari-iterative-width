/*
 * ActionSequenceDetection.cpp
 *
 *  Created on: Aug 11, 2016
 *      Author: yuu
 */

#include "ActionSequenceDetection.hpp"
#include "SearchTree.hpp"

ActionSequenceDetection::ActionSequenceDetection() {
}

ActionSequenceDetection::~ActionSequenceDetection() {
	// TODO Auto-generated destructor stub
}

// Return the list of actions which is available given the previous action sequences
std::vector<bool> ActionSequenceDetection::getUsefulActions(
		std::vector<Action> previousActions) {
	std::vector<bool> available = usedActionSeqs[0];

	int length = min(previousActions.size(), usedActionSeqs.size());

	for (int i = 1; i <= length; ++i) {
		std::vector<Action> p;
		p.resize(i);
		p.assign(previousActions.end() - i, previousActions.end());

		for (int j = 0; j < PLAYER_A_MAX; ++j) {
			std::vector<Action> sequence = p;
			sequence.push_back((Action) j);
			int seqInt = secToInt(sequence);

			if (!usedActionSeqs[i - 1][seqInt]) {
				available[j] = false;
			}
		}

	}

	return available;
}

// TODO: Implement seqLength 1 first
/**
 * 1.
 *
 *
 */
void ActionSequenceDetection::getJunkActionSequence(SearchTree* tree,
		int seqLength) {
	if (usedActionSeqs.size() < seqLength) {
		assert(seqLength == usedActionSeqs.size() + 1);
		usedActionSeqs.resize(seqLength);
		int size = t_size(seqLength);
		usedActionSeqs[seqLength - 1].resize(size);
		usedActionSeqs[seqLength - 1].assign(size, false);
	}

	TreeNode* root = tree->get_root();
	searchNode(root, seqLength, usedActionSeqs[seqLength - 1]);

//	return isSequenceUsed;

	for (int i = 1; i < seqLength; ++i) {
		printf("fixed   %d: ", i);
		for (int j = 0; j < usedActionSeqs[i - 1].size(); ++j) {
			if (usedActionSeqs[i - 1][j]) {
				printf("o");
			} else {
				printf("x");
			}
		}
		printf("\n");
	}

	printf("working %d: ", seqLength);
	for (int j = 0; j < usedActionSeqs[seqLength - 1].size(); ++j) {
		if (usedActionSeqs[seqLength - 1][j]) {
			printf("o");
		} else {
			printf("x");
		}
	}
	printf("\n");
}

// Apply recursive call
void ActionSequenceDetection::searchNode(TreeNode* node, int seqLength,
		std::vector<bool>& isSequenceUsed) {
	// 1. get actionList for the node
	getUsedSequenceList(node, seqLength, isSequenceUsed);

	// 2. recursively call this for child nodes
	NodeList childList = sortNodeList(node->v_children);
	for (int i = 0; i < childList.size(); ++i) {
		searchNode(childList[i], seqLength, isSequenceUsed);
	}
}

// Get the UsedSequeneList for a single node
// TODO: Action sequence is not supported yet
void ActionSequenceDetection::getUsedSequenceList(TreeNode* node, int seqLength,
		std::vector<bool>& isSequenceUsed) {

//	NodeList childList = node->v_children;
//	std::vector<bool> usedAction;
//	usedAction.resize(PLAYER_A_MAX);
//	usedAction.assign(PLAYER_A_MAX, false);

//	std::vector<Action> currSeq;
//	getUsedSequenceRec(node, seqLength, isSequenceUsed, currSeq, 1);

	int size = t_size(seqLength);
	std::vector<TreeNode*> nodeList;

	for (int i = 0; i < size; ++i) {
		TreeNode* child = getResultingNode(node, intToSec(i, seqLength));
		nodeList.push_back(child);
//		if (child != nullptr) {
//			printf("%d ", child->novelty);
//		} else {
//			printf("X ");
//		}
	}
//	printf("\n");

	for (int i = 0; i < size; ++i) {
		if (nodeList[i] == nullptr) {
			continue;
		}

		bool isDuplicate = false;
		for (int j = 0; j < i; ++j) {
			if (nodeList[j] == nullptr) {
				continue;
			}

			if (nodeList[i]->state.equals(nodeList[j]->state)) {
				isDuplicate = true;
				break;
			}
		}
		if (!isDuplicate) {
			isSequenceUsed[i] = true;
		}
	}

}

void ActionSequenceDetection::getUsedSequenceRec(TreeNode* node, int seqLength,
		std::vector<bool>& isSequenceUsed, std::vector<Action>& currSeq,
		int currDepth) {
	if (currDepth == seqLength) {
		NodeList childList = sortNodeList(node->v_children);
		for (int i = 0; i < childList.size(); ++i) {
			TreeNode* child = childList[i];

			// TODO: find a way to check if the child is in fact redundant in a sequential fashion.
			// TODO: use "test_duplicate function?"
			// TODO: if (sibling->state.equals(node->state))
			if (!child->is_leaf()) {
				std::vector<Action> list = currSeq;
				list.push_back(child->act);
				isSequenceUsed[secToInt(list)] = true;
			}
		}
	} else {
		NodeList childList = sortNodeList(node->v_children);
		for (int i = 0; i < childList.size(); ++i) {
			TreeNode* child = childList[i];
			if (!child->is_leaf()) {
				std::vector<Action> list = currSeq;
				list.push_back(child->act);
				getUsedSequenceRec(child, seqLength, isSequenceUsed, list,
						currDepth + 1);
			}
		}
	}

}

int ActionSequenceDetection::secToInt(std::vector<Action> sequence) {
	int ret = 0;
	for (int i = 0; i < sequence.size(); ++i) {
		ret = ret * PLAYER_A_MAX + (int) sequence[i];
	}
	return ret;
}

std::vector<Action> ActionSequenceDetection::intToSec(int seqInt,
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

NodeList ActionSequenceDetection::sortNodeList(NodeList childList) {
	NodeList sortedList = childList;
	std::sort(sortedList.begin(), sortedList.end(), nodeActionSort);
	return sortedList;
}

TreeNode* ActionSequenceDetection::getResultingNode(TreeNode* root,
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

int ActionSequenceDetection::t_size(int seqLength) {
	if (seqLength == 0) {
		return 0;
	}
	int size = 1;
	for (int i = 0; i < seqLength; ++i) {
		size *= PLAYER_A_MAX;
	}
	return size;
}
