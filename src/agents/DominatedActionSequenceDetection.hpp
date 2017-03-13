/*
 * ActionSequenceDetection.hpp
 *
 *  Created on: Aug 11, 2016
 *      Author: yuu
 */

#ifndef SRC_AGENTS_DOMINATEDACTIONSEQUENCEDETECTION_HPP_
#define SRC_AGENTS_DOMINATEDACTIONSEQUENCEDETECTION_HPP_

#include "Settings.hxx"
#include "Constants.h"
#include "VertexCover.hpp"
//#include "SearchTree.hpp"

class SearchTree;
class TreeNode;

class DominatedActionSequenceDetection {
public:
	DominatedActionSequenceDetection(Settings & settings);
	virtual ~DominatedActionSequenceDetection();

	void learnDominatedActionSequences(SearchTree* tree, int seqLength);

	std::vector<bool> getEffectiveActions(std::vector<Action> previousActions);
	int getDetectedUsedActionsSize();

private:
	void learnDASA(SearchTree* tree, int seqLength);
	void learnDASP(SearchTree* tree, int seqLength);

	void searchNode(TreeNode* tree, int seqLength,
			std::vector<bool>& isSequenceUsed);
	void getUsedSequenceList(TreeNode* node, int seqLength,
			std::vector<bool>& isSequenceUsed);
//	void getUsedSequenceRec(TreeNode* node, int seqLength,
//			std::vector<bool>& isSequenceUsed, std::vector<Action>& currSeq,
//			int currDepth);
	int seqToInt(std::vector<Action> sequence);
	std::vector<Action> intToSeq(int seqInt, int seqLength);

	std::vector<TreeNode*> sortNodeList(std::vector<TreeNode*> childList);
	TreeNode* getResultingNode(TreeNode* root, std::vector<Action> sequence);

	std::vector<std::vector<bool> > usedActionSeqs;

//	bool nodeActionSort(const TreeNode* l, const TreeNode* r);

	int seq_size(int seqLength);

	/**
	 * Dominated Action Sequence Avoidance
	 */
	std::vector<bool> getWeightedProbableActions(
			std::vector<Action> previousActions);
	std::vector<double> getQvaluesOfNextActions(
			std::vector<Action> previousActions);
	std::vector<double> getQvaluesOfAllSequences(int seqLength);
	double calcQvalue(int sequence, int seqLength);
	double calcNovelRatio(int n_steps_before, int sequence, int seqLength);

	double sigmoid(double x, double gain);

	bool probablistic_action_selection; // True if DASA, False if DASP
	int action_length;
	double discount_factor;
	double epsilon;
	double junk_threshold;
	int maximum_steps_to_consider;
	std::vector<std::vector<std::vector<int> > > num_novel_node_by_action;
	std::vector<std::vector<std::vector<int> > > num_duplicate_node_by_action;

//	std::vector<std::vector<int> > num_novel_node_by_action_sequence;
//	std::vector<std::vector<int> > num_duplicate_node_by_action_sequence;

	std::vector<std::vector<double> > qvalues_by_action; // length = 1
//	std::vector<double> qvalues_by_action_sequence; // length = 2
	std::vector<double> probabilty_by_action;

	std::vector<VertexCover*> dominance_graph;


	bool permutate_action;
	std::vector<Action> action_permutation;
	int permutateToOriginalAction(int input, int seqLength);

//	bool wayToSort(const Action &a, const Action &b) const {
//		if (usedActionSeqs[0][a] && usedActionSeqs[0][b]) {
//			return true;
//		} else if (usedActionSeqs[0][a] && !usedActionSeqs[0][b]){
//			return false;
//		} else if (!usedActionSeqs[0][a] && usedActionSeqs[0][b]){
//			return false;
//		} else {
//			return false;
//		}
//	}
};

#endif /* SRC_AGENTS_DOMINATEDACTIONSEQUENCEDETECTION_HPP_ */
