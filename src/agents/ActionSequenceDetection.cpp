/*
 * ActionSequenceDetection.cpp
 *
 *  Created on: Aug 11, 2016
 *      Author: yuu
 */

#include "ActionSequenceDetection.hpp"
#include "SearchTree.hpp"
#include <random>
#include <algorithm>

ActionSequenceDetection::ActionSequenceDetection(Settings & settings) {
	probablistic_action_selection = settings.getBool(
			"probablistic_action_selection", false);

	permutate_action = settings.getBool("permutate_action", false);

	discount_factor = settings.getFloat("asd_discount_factor", false);
	maximum_steps_to_consider = settings.getInt("asd_maximum_steps_to_consider",
			false);
	epsilon = settings.getFloat("asd_epsilon", false);

	junk_threshold = settings.getFloat("asd_junk_threshold", false);

	if (discount_factor < 0) {
		discount_factor = 0.95;
	}
	if (maximum_steps_to_consider < 0) {
		maximum_steps_to_consider = 30;
	}
	if (epsilon < 0.0) {
		epsilon = 0.1;
	}

	if (junk_threshold < 0.0) {
		junk_threshold = 0.4;
	}

	num_novel_node_by_action.resize(2);
	num_duplicate_node_by_action.resize(2);
	qvalues_by_action.resize(2);
}

ActionSequenceDetection::~ActionSequenceDetection() {
}

int ActionSequenceDetection::getDetectedUsedActionsSize() {
	int n_used_actions = 0;
	if (action_length == 1) {
		if (probablistic_action_selection) {
			std::vector<double> qs = getQvaluesOfNextActions(
					std::vector<Action>());
			for (int a = 0; a < PLAYER_A_MAX; ++a) {
				if (qs[a] > 0.05) {
					++n_used_actions;
				}
			}

			// TODO:
			double average_num_actions_per_state = 0.0;
			for (int a = 0; a < PLAYER_A_MAX; ++a) {
				double norm = (qvalues_by_action[0][a] - 0.5) * 5.0 + 1.5;
				double probabilty = sigmoid(norm, 1) * (1 - epsilon) + epsilon;
				average_num_actions_per_state += probabilty;
			}
			printf("AverageNumActionsPerState= %.2f\n",
					average_num_actions_per_state);

		} else {
			std::vector<bool> available = usedActionSeqs[0];
			for (int a = 0; a < PLAYER_A_MAX; ++a) {
				if (available[a]) {
					++n_used_actions;
				}
			}
		}
	}
	printf("DetectedUsedActions= %d\n", n_used_actions);
	return n_used_actions;
}

// Return the list of actions which is available given the previous action sequences
//
std::vector<bool> ActionSequenceDetection::getUsefulActions(
		std::vector<Action> previousActions) {

	if (probablistic_action_selection) {
		return getWeightedProbableActions(previousActions);
	}

	std::vector<bool> available = usedActionSeqs[0];

	int length = min(previousActions.size() + 1, usedActionSeqs.size());

// i <- the length of junk to check.
	for (int i = 2; i <= length; ++i) {
		std::vector<Action> p;
		p.resize(i);
		p.assign(previousActions.end() - i, previousActions.end());

		for (int j = 0; j < PLAYER_A_MAX; ++j) {
			std::vector<Action> sequence = p;
			sequence.push_back((Action) j);
			int seqInt = seqToInt(sequence);

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
	action_length = seqLength;

	if (action_permutation.empty()) {
		for (int i = 0; i < PLAYER_A_MAX; ++i) {
			action_permutation.push_back((Action) i);
		}
		if (permutate_action) {
			std::random_shuffle(action_permutation.begin(),
					action_permutation.end());
		}
	}
//	action_permutation.clear();

	if (usedActionSeqs.size() < seqLength) {
//		assert(seqLength == usedActionSeqs.size() + 1);
		usedActionSeqs.resize(seqLength);
		int size = t_size(seqLength);
		usedActionSeqs[seqLength - 1].resize(size);
		usedActionSeqs[seqLength - 1].assign(size, false);
	}

	if (probablistic_action_selection) {
		num_novel_node_by_action[0].push_back(std::vector<int>(t_size(1), 0));
		num_duplicate_node_by_action[0].push_back(
				std::vector<int>(t_size(1), 0));
		num_novel_node_by_action[1].push_back(std::vector<int>(t_size(2), 0));
		num_duplicate_node_by_action[1].push_back(
				std::vector<int>(t_size(2), 0));
	}

	// TODO: length=2; or should we keep this to l=1?
	VertexCover* dgraph = new VertexCover(t_size(1));
	dominance_graph.push_back(dgraph);

	TreeNode* root = tree->get_root();
	searchNode(root, seqLength, usedActionSeqs[seqLength - 1]);

//	return isSequenceUsed;

	if (!probablistic_action_selection) {
		std::vector<bool> minset = dominance_graph.back()->minimalActionSet();
		if (std::count(minset.begin(), minset.end(), true)
				< std::count(usedActionSeqs[0].begin(), usedActionSeqs[0].end(),
						true)) {
//			printf("usedActionSeqs:");
//			for (int i = 0; i < PLAYER_A_MAX; i++)
//				if (usedActionSeqs[0][i])
//					printf("%s ", action_to_string((Action) i).c_str());
//			printf("\n");
//			printf("min coverage:");
//			for (int i = 0; i < PLAYER_A_MAX; i++)
//				if (minset[i])
//					printf("%s ", action_to_string((Action) i).c_str());
//			printf("\n");

			if (permutate_action) {

				printf("minset %d < %d\n",
						std::count(minset.begin(), minset.end(), true),
						std::count(usedActionSeqs[0].begin(),
								usedActionSeqs[0].end(), true));
				usedActionSeqs[0] = minset;

				// REFACTOR: hand-scripted stable_sort: Not sure how to implement using lambda.

				std::vector<Action> in;
				std::vector<Action> notin;
				for (int i = 0; i < action_permutation.size(); ++i) {
					if (usedActionSeqs[0][action_permutation[i]]) {
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
	} else {
		std::vector<bool> minset = dominance_graph.back()->minimalActionSet();
		qvalues_by_action[0] = getQvaluesOfAllSequences(1);
		int numPruned = std::count_if(qvalues_by_action[0].begin(),
				qvalues_by_action[0].end(), [](double i) {return i > 0.1;});

		if (std::count(minset.begin(), minset.end(), true) < numPruned) {
//			printf("qs: ");
//			for (int i = 0; i < PLAYER_A_MAX; ++i) {
//				if (qvalues_by_action[0][i] > 0.1)
//					printf("%s ", action_to_string((Action) i).c_str());
//			}
//			printf("\n");

//			printf("min coverage:");
//			for (int i = 0; i < PLAYER_A_MAX; i++)
//				if (minset[i])
//					printf("%s ", action_to_string((Action) i).c_str());
//			printf("\n");

			printf("minset %d < %d\n",
					std::count(minset.begin(), minset.end(), true), numPruned);
//			usedActionSeqs[0] = minset;
			std::vector<bool> marked =
					dominance_graph.back()->uniqueActionSet();
			// REFACTOR: hand-scripted stable_sort: Not sure how to implement using lambda.

//			printf("pre qs: ");
//			for (int i = 0; i < PLAYER_A_MAX; ++i) {
//				int a = action_permutation[i];
//				printf("%.2f ", qvalues_by_action[0][a]);
//			}
//			printf("\n");

			if (permutate_action) {
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
				action_permutation = unique;
			}
//			printf("ordered qs: ");
//			for (int i = 0; i < PLAYER_A_MAX; ++i) {
//				int a = action_permutation[i];
//				printf("%.2f ", qvalues_by_action[0][a]);
//			}
//			printf("\n");
		}

		int size = t_size(seqLength);
//		std::vector<Action> previousActions(seqLength - 1, PLAYER_A_NOOP);
		qvalues_by_action[0] = getQvaluesOfAllSequences(1);
		if (seqLength >= 2) {
			qvalues_by_action[1] = getQvaluesOfAllSequences(2);
		}

		probabilty_by_action = std::vector<double>(size, 0.0);
		for (int a = 0; a < size; ++a) {
			double norm1 = (qvalues_by_action[0][a % PLAYER_A_MAX] - 0.5) * 5.0;
			double p1 = sigmoid(norm1, 1);
			double p2 = 1.0;
			if (seqLength >= 2) {
				double norm2 = (qvalues_by_action[1][a] - 0.5) * 5.0;
				p2 = sigmoid(norm2, 1);
			}

			probabilty_by_action[a] = p1 * p2 * (1 - epsilon) + epsilon;

			if (probabilty_by_action[a] > 1.0) {
				probabilty_by_action[a] = 1.0;
			}
//			if (qvalues_by_action[a] > junk_threshold) {
//				probabilty_by_action[a] = 1.0;
//			} else {
//				probabilty_by_action[a] = sqrt(qvalues_by_action[a] * 2.0)
//						* (1.0 - epsilon) + epsilon;
//			}

		}
		printf("qs: ");
		for (int a = 0; a < PLAYER_A_MAX; ++a) {
			printf("%.2f ", qvalues_by_action[0][a]);
		}
		printf("\n");

//		printf("ordered qs: ");
//		for (int i = 0; i < PLAYER_A_MAX; ++i) {
//			int a = action_permutation[i];
//			printf("%.2f ", qvalues_by_action[0][a]);
//		}
//		printf("\n");

		if (seqLength >= 2) {
			printf("qs2: ");
			for (int a = 0; a < size; ++a) {
				printf("%.2f ", qvalues_by_action[1][a]);
			}
			printf("\n");
		}
//		qvalues_by_action.push_back(qvalues);

//		printf("used: \n");
//		for (int a = 0; a < size; ++a) {
//			printf(" %-2d:  %d + %d\n", a,
//					num_novel_node_by_action[num_novel_node_by_action.size() - 1][a],
//					num_duplicate_node_by_action[num_duplicate_node_by_action.size()
//							- 1][a]);
//		}
//		printf("\n");

	}
}

// Apply recursive call
void ActionSequenceDetection::searchNode(TreeNode* node, int seqLength,
		std::vector<bool>& isSequenceUsed) {
// 1. get actionList for the node
	getUsedSequenceList(node, seqLength, isSequenceUsed);

	if (seqLength == 2) {
		getUsedSequenceList(node, 1, isSequenceUsed);
	}

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
		vector<Action> seq = intToSeq(i, seqLength);
		for (int j = 0; j < seq.size(); ++j) {
			seq[j] = action_permutation[seq[j]];
		}
		TreeNode* child = getResultingNode(node, seq);
		nodeList.push_back(child);

	}

	if (probablistic_action_selection) {

//		if (try_permutation)
//		vector<Action> action_permutation;
//		for (int i = 0; i < PLAYER_A_MAX; ++i) {
//			action_permutation.push_back((Action) i);
//		}
//		std::random_shuffle(action_permutation.begin(),
//				action_permutation.end());

// TODO: t_size should be seqLength

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
			assert(num_duplicate_node_by_action[0].size() > 0);
			assert(num_novel_node_by_action[0].size() > 0);

			int sInt = permutateToOriginalAction(i, seqLength);
			if (isDuplicate) {
				num_duplicate_node_by_action[seqLength - 1][num_duplicate_node_by_action[seqLength
						- 1].size() - 1][sInt]++;
			} else {
//				printf("num %d %d %d\n", num_novel_node_by_action.size(), i,
//						num_novel_node_by_action[num_novel_node_by_action.size()
//								- 1].size());
				num_novel_node_by_action[seqLength - 1][num_novel_node_by_action[seqLength
						- 1].size() - 1][sInt]++;
			}
		}
	} else {

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
				int sInt = permutateToOriginalAction(i, seqLength);
				isSequenceUsed[sInt] = true;
			}
		}
	}

// TODO: for now we only detect single action dominance.
	int tmp_seqLength = 1;
	size = t_size(tmp_seqLength);
	for (int i = 0; i < size; ++i) {
		if (nodeList[i] == nullptr || nodeList[i]->is_terminal) {
			continue;
		}
		int iInt = permutateToOriginalAction(i, tmp_seqLength);

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
				int jInt = permutateToOriginalAction(j, tmp_seqLength);
				dominance_graph.back()->addEdge(iInt, jInt);
				break;
			}
		}
		if (!isDuplicate) {
			dominance_graph.back()->addNode(iInt);
		}
	}
}

//void ActionSequenceDetection::getUsedSequenceRec(TreeNode* node, int seqLength,
//		std::vector<bool>& isSequenceUsed, std::vector<Action>& currSeq,
//		int currDepth) {
//	if (currDepth == seqLength) {
//		NodeList childList = sortNodeList(node->v_children);
//		for (int i = 0; i < childList.size(); ++i) {
//			TreeNode* child = childList[i];
//
//			// TODO: find a way to check if the child is in fact redundant in a sequential fashion.
//			// TODO: use "test_duplicate function?"
//			// TODO: if (sibling->state.equals(node->state))
//			if (!child->is_leaf()) {
//				std::vector<Action> list = currSeq;
//				list.push_back(child->act);
//				isSequenceUsed[secToInt(list)] = true;
//			}
//		}
//	} else {
//		NodeList childList = sortNodeList(node->v_children);
//		for (int i = 0; i < childList.size(); ++i) {
//			TreeNode* child = childList[i];
//			if (!child->is_leaf()) {
//				std::vector<Action> list = currSeq;
//				list.push_back(child->act);
//				getUsedSequenceRec(child, seqLength, isSequenceUsed, list,
//						currDepth + 1);
//			}
//		}
//	}
//
//}

int ActionSequenceDetection::seqToInt(std::vector<Action> sequence) {
	int ret = 0;
	for (int i = 0; i < sequence.size(); ++i) {
		ret = ret * PLAYER_A_MAX + (int) sequence[i];
	}
	return ret;
}

std::vector<Action> ActionSequenceDetection::intToSeq(int seqInt,
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

TreeNode * ActionSequenceDetection::getResultingNode(TreeNode * root,
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

std::vector<bool> ActionSequenceDetection::getWeightedProbableActions(
		std::vector<Action> previousActions) {
	int size = t_size(previousActions.size() + 1);
	std::vector<bool> ret(PLAYER_A_MAX, false);
//	std::vector<double> qvalues = qvalues_by_action;

// TODO: currently only length = 1;
	for (int a = 0; a < PLAYER_A_MAX; ++a) {
		std::vector<Action> seq = previousActions;
		seq.push_back((Action) a);
		double probability = probabilty_by_action[seqToInt(seq)];
		double random = (double) rand() / (double) RAND_MAX;
//		printf("p=%.1f, r=%.1f\n", probability, random);
		if (random < probabilty_by_action[a]) {
			ret[a] = true;
		} else {
//			printf("%d pruned\n", a);
		}

	}

//	for (int a = 0; a < PLAYER_A_MAX; ++a) {
//		if (ret[a]) {
//			printf("o");
//
//		} else {
//			printf("x");
//		}
//	}
//	printf("\n");
	return ret;
}

std::vector<double> ActionSequenceDetection::getQvaluesOfNextActions(
		std::vector<Action> previousActions) {
//	int size = t_size(previousActions.size() + 1);

	std::vector<double> ret(PLAYER_A_MAX, 0.0);
// TODO: currently only length = 1;
	for (int a = 0; a < PLAYER_A_MAX; ++a) {
		std::vector<Action> seq = previousActions;
		seq.push_back((Action) a);
		int sInt = seqToInt(seq);
		ret[a] = calcQvalue(sInt, previousActions.size() + 1);
	}
	return ret;
}

std::vector<double> ActionSequenceDetection::getQvaluesOfAllSequences(
		int seqLength) {
////	int size = t_size(previousActions.size() + 1);
//	int size = t_size(action_length);
//	std::vector<double> ret(size, 0.0);
//// TODO: currently only length = 1;
//	for (int a = 0; a < size; ++a) {
//		ret[a] = calcQvalue(a);
//	}
//	return ret;

	int size = t_size(seqLength);

	std::vector<double> ret(size, 0.0);
	for (int a = 0; a < size; ++a) {
		double ratio = calcNovelRatio(1, a, seqLength);
		if (qvalues_by_action[seqLength - 1].size() == 0) {
			// First planning
			ret[a] = 1.0;
		} else if (ratio < 0.0) {
			// If no data for t-1 step
			ret[a] = qvalues_by_action[seqLength - 1][a];
		} else {
			ret[a] = (qvalues_by_action[seqLength - 1][a] * discount_factor
					+ ratio) / (1.0 + discount_factor);
		}
//		assert(0.0 <= ret[a] && ret[a] <= 1.0);
	}
	return ret;

}

double ActionSequenceDetection::calcQvalue(int sequence, int seqLength) {
	double q = 0.0;
	double normalizer = (1.0 - pow(discount_factor, maximum_steps_to_consider))
			/ (1.0 - discount_factor);

	for (int i = 0; i < maximum_steps_to_consider; ++i) {
		double ratio = calcNovelRatio(i + 1, sequence, seqLength);
		if (ratio >= 0.0) {
			q += pow(discount_factor, i)
					* calcNovelRatio(i + 1, sequence, seqLength);

		} else {
			// means no data. So we have to exclude \alpha^i factor from normalizer.
			normalizer -= pow(discount_factor, i);
		}
	}

	if (normalizer < 0.0001) {
// means no data. Let's go for a exploration.
// Extremele unlikely though.
		return 1.0;
	}

	return q / normalizer;
}

double ActionSequenceDetection::calcNovelRatio(int n_steps_before, int sequence,
		int seqLength) {
	if (n_steps_before > (int) num_novel_node_by_action[seqLength - 1].size()) {
		return 2.0;
	}
	if (n_steps_before
			> (int) num_duplicate_node_by_action[seqLength - 1].size()) {
		return 2.0;
	}
	int n =
			num_novel_node_by_action[seqLength - 1][num_novel_node_by_action[seqLength
					- 1].size() - n_steps_before][sequence];
	int d =
			num_duplicate_node_by_action[seqLength - 1][num_duplicate_node_by_action[seqLength
					- 1].size() - n_steps_before][sequence];

	if (n + d <= 0) {
		return -1.0;
	}
	return (double) n / ((double) (n + d));
}

double ActionSequenceDetection::sigmoid(double x, double gain) {
	return 1.0 / (1.0 + exp(-gain * x));
}

int ActionSequenceDetection::permutateToOriginalAction(int input,
		int seqLength) {
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
