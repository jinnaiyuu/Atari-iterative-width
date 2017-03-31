/*
 * DominatedActionSequenceAvoidance.cpp
 *
 *  Created on: Mar 30, 2017
 *      Author: yuu
 */

#include "DominatedActionSequenceAvoidance.hpp"

#include "SearchTree.hpp"
#include <random>
#include <algorithm>

DominatedActionSequenceAvoidance::DominatedActionSequenceAvoidance(
		Settings & settings, StellaEnvironment* _env) :
		DominatedActionSequenceDetection(settings, _env) {
	discount_factor = settings.getFloat("asd_discount_factor", false);
	maximum_steps_to_consider = settings.getInt("asd_maximum_steps_to_consider",
			false);
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

	// ODO
	num_novel_node_by_action.resize(2);
	num_duplicate_node_by_action.resize(2);
	ratio_of_novelty.resize(2);
}

DominatedActionSequenceAvoidance::~DominatedActionSequenceAvoidance() {
	// TODO Auto-generated destructor stub
}

void DominatedActionSequenceAvoidance::learnDominatedActionSequences(
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

	// Store the number of novel/duplicate nodes each action generated.
	for (int i = 1; i <= seqLength; ++i) {
		num_novel_node_by_action[i - 1].push_back(
				std::vector<int>(num_sequences(i), 0));
		num_duplicate_node_by_action[i - 1].push_back(
				std::vector<int>(num_sequences(i), 0));
	}

	TreeNode* root = tree->get_root();
	for (int i = 1; i <= seqLength; ++i) {
		VertexCover dgraph(num_sequences(i)); //TODO
		dominance_graph.push_back(dgraph);
		searchNode(root, i, isUsefulActionSequence[i - 1]);
	}

	learnDASA(tree, seqLength);
}

std::vector<bool> DominatedActionSequenceAvoidance::getEffectiveActions(
		std::vector<Action> previousActions, int current_frame) {
	if (junk_decision_frame > current_frame) {
		return vector<bool>(PLAYER_A_MAX, true);
	}

	return getDASAActionSet(previousActions);
}

int DominatedActionSequenceAvoidance::getDetectedUsedActionsSize() {
	int n_used_actions = 0;
	if (action_length == 1) {
		std::vector<double> qs = getQvaluesOfNextActions(std::vector<Action>());
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

	}
	printf("DetectedUsedActions= %d\n", n_used_actions);
	return n_used_actions;
}

void DominatedActionSequenceAvoidance::learnDASA(SearchTree* tree,
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

std::vector<double> DominatedActionSequenceAvoidance::novelty_to_probabilities(
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

std::vector<bool> DominatedActionSequenceAvoidance::getDASAActionSet(
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
	return ret;
}

std::vector<double> DominatedActionSequenceAvoidance::getQvaluesOfNextActions(
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

std::vector<double> DominatedActionSequenceAvoidance::getQvaluesOfAllSequences(
		int seqLength) {
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

double DominatedActionSequenceAvoidance::calcQvalue(int sequence,
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

double DominatedActionSequenceAvoidance::calcNovelRatio(int n_steps_before,
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

double DominatedActionSequenceAvoidance::sigmoid(double x, double gain) {
	return 1.0 / (1.0 + exp(-gain * x));
}

void DominatedActionSequenceAvoidance::getUsedSequenceList(TreeNode* node,
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

	for (int i = 0; i < size; ++i) {
		if (nodeList[i] == nullptr || nodeList[i]->is_terminal) {
			continue;
		}

		bool isDuplicate = false;
		for (int j = 0; j < i; ++j) {
			if (nodeList[j] == nullptr || nodeList[i]->is_terminal) {
				continue;
			}

			// ODO
			if (nodeList[i]->state.equals(nodeList[j]->state)) {
				isDuplicate = true;
				break;
			}
		}
		assert(num_duplicate_node_by_action[0].size() > 0);
		assert(num_novel_node_by_action[0].size() > 0);

		int sInt = permutateToOriginalAction(i, seqLength);
		if (isDuplicate) {
//			printf("acion dupped; %d\n", sInt);
			num_duplicate_node_by_action[seqLength - 1][num_duplicate_node_by_action[seqLength
					- 1].size() - 1][sInt]++;
		} else {
//			printf("acion novel; %d\n", sInt);
//			printf("num %d %d %d\n", num_novel_node_by_action.size(), i,
//					-num_novel_node_by_action[num_novel_node_by_action.size()
//							- 1].size());
			num_novel_node_by_action[seqLength - 1][num_novel_node_by_action[seqLength
					- 1].size() - 1][sInt]++;
		}
	}

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
