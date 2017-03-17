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
		junk_decision_frame = 12; // 5 seconds in game
	}

	isDASA = settings.getBool("probablistic_action_selection", false)
			|| settings.getBool("probabilistic_action_selection", false)
			|| settings.getBool("dasa", false);

	permutate_action = settings.getBool("permutate_action", false);

	if (isDASA) {
		discount_factor = settings.getFloat("asd_discount_factor", false);
		maximum_steps_to_consider = settings.getInt(
				"asd_maximum_steps_to_consider", false);
		epsilon = settings.getFloat("asd_epsilon", false);
		if (discount_factor < 0) {
			discount_factor = 0.95;
		}
		if (maximum_steps_to_consider < 0) {
			maximum_steps_to_consider = 30;
		}
		if (epsilon < 0.0) {
			epsilon = 0.1;
		}
	}

//	junk_threshold = settings.getFloat("asd_junk_threshold", false);

//	if (junk_threshold < 0.0) {
//		junk_threshold = 0.4;
//	}

	num_novel_node_by_action.resize(2);
	num_duplicate_node_by_action.resize(2);
	ratio_of_novelty.resize(2);
}

DominatedActionSequenceDetection::~DominatedActionSequenceDetection() {
}

int DominatedActionSequenceDetection::getDetectedUsedActionsSize() {
	int n_used_actions = 0;
	if (action_length == 1) {
		if (isDASA) {
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
				double norm = (ratio_of_novelty[0][a] - 0.5) * 5.0 + 1.5;
				double probabilty = sigmoid(norm, 1) * (1 - epsilon) + epsilon;
				average_num_actions_per_state += probabilty;
			}
			printf("AverageNumActionsPerState= %.2f\n",
					average_num_actions_per_state);

		} else {
			std::vector<bool> available = isUsefulActionSequence[0];
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
std::vector<bool> DominatedActionSequenceDetection::getEffectiveActions(
		std::vector<Action> previousActions) {
	if (junk_decision_frame > m_env->getFrameNumber()) {
		return vector<bool>(PLAYER_A_MAX, true);
	}

	if (isDASA) {
		return getDASAActionSet(previousActions);
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

	return available;
}

void DominatedActionSequenceDetection::learnDominatedActionSequences(
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

	if (isDASA) {
		// Store the number of novel/duplicate nodes each action generated.
		for (int i = 1; i <= seqLength; ++i) {
			num_novel_node_by_action[i - 1].push_back(
					std::vector<int>(num_sequences(i), 0));
			num_duplicate_node_by_action[i - 1].push_back(
					std::vector<int>(num_sequences(i), 0));
		}
	}

	TreeNode* root = tree->get_root();
	for (int i = 1; i <= seqLength; ++i) {
		VertexCover dgraph(num_sequences(i)); //TODO
		dominance_graph.push_back(dgraph);
		searchNode(root, i, isUsefulActionSequence[i - 1]);
	}

	if (isDASA) {
		learnDASA(tree, seqLength);
	} else {
		learnDASP(tree, seqLength);
	}

}

void DominatedActionSequenceDetection::learnDASA(SearchTree* tree,
		int seqLength) {

	if (permutate_action) {
		std::vector<bool> minset = dominance_graph[0].minimalActionSet();
		ratio_of_novelty[0] = getQvaluesOfAllSequences(1);
		int numPruned = std::count_if(ratio_of_novelty[0].begin(),
				ratio_of_novelty[0].end(), [](double i) {return i < 0.1;});
		if (std::count(minset.begin(), minset.end(), true) < numPruned) {
			printf("minset %d < %d\n",
					std::count(minset.begin(), minset.end(), true), numPruned);
			std::vector<bool> marked = dominance_graph[0].uniqueActionSet();
			action_permutation = sortByNovelty(marked, minset);
		}
	}

	for (int i = 1; i <= seqLength; ++i) {
		ratio_of_novelty[i - 1] = getQvaluesOfAllSequences(i);
		printf("novelty of action sequence of length %d: ", i);
		for (int a = 0; a < num_sequences(i); ++a) {
			printf("%.2f ", ratio_of_novelty[i - 1][a]);
		}
		printf("\n");
	}

	probabilty_by_action = novelty_to_probabilities(seqLength);
}

/**
 * TODO: not needed here?
 * Returns the probabilities of applying actions.
 */
std::vector<double> DominatedActionSequenceDetection::novelty_to_probabilities(
		int seqLength) {
	int size = num_sequences(seqLength);
	std::vector<double> probabilities = std::vector<double>(size, 0.0);
	for (int a = 0; a < size; ++a) {
		std::vector<Action> seq = intToSeq(a, seqLength);
		double p1 = 1;
		for (int i = 1; i <= seqLength; ++i) {
			double norm = (ratio_of_novelty[i - 1][seq[i - 1]] - 0.5) * 5.0;
			p1 = p1 * sigmoid(norm, 1);
		}

		probabilities[a] = p1 * (1 - epsilon) + epsilon;

		if (probabilities[a] > 1.0) {
			// printf("\n");
			probabilities[a] = 1.0;
		}
	}
	return probabilities;
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

void DominatedActionSequenceDetection::learnDASP(SearchTree* tree,
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

// Get the UsedSequeneList for a single node
void DominatedActionSequenceDetection::getUsedSequenceList(TreeNode* node,
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

	if (isDASA) {

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

std::vector<bool> DominatedActionSequenceDetection::getDASAActionSet(
		std::vector<Action> previousActions) {
	int size = num_sequences(previousActions.size() + 1);
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

std::vector<double> DominatedActionSequenceDetection::getQvaluesOfNextActions(
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

std::vector<double> DominatedActionSequenceDetection::getQvaluesOfAllSequences(
		int seqLength) {
////	int size = t_size(previousActions.size() + 1);
//	int size = t_size(action_length);
//	std::vector<double> ret(size, 0.0);
//// TODO: currently only length = 1;
//	for (int a = 0; a < size; ++a) {
//		ret[a] = calcQvalue(a);
//	}
//	return ret;

	int size = num_sequences(seqLength);

	std::vector<double> ret(size, 0.0);
	for (int a = 0; a < size; ++a) {
		double ratio = calcNovelRatio(1, a, seqLength);
		if (ratio_of_novelty[seqLength - 1].size() == 0) {
			// First planning
			ret[a] = 1.0;
		} else if (ratio < 0.0) {
			// If no data for t-1 step
			ret[a] = ratio_of_novelty[seqLength - 1][a];
		} else {
			ret[a] = (ratio_of_novelty[seqLength - 1][a] * discount_factor
					+ ratio) / (1.0 + discount_factor);
		}
//		assert(0.0 <= ret[a] && ret[a] <= 1.0);
	}
	return ret;

}

double DominatedActionSequenceDetection::calcQvalue(int sequence,
		int seqLength) {
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

double DominatedActionSequenceDetection::calcNovelRatio(int n_steps_before,
		int sequence, int seqLength) {
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

double DominatedActionSequenceDetection::sigmoid(double x, double gain) {
	return 1.0 / (1.0 + exp(-gain * x));
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
