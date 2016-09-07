/*
 * ActionSequenceDetection.hpp
 *
 *  Created on: Aug 11, 2016
 *      Author: yuu
 */

#ifndef SRC_AGENTS_ACTIONSEQUENCEDETECTION_HPP_
#define SRC_AGENTS_ACTIONSEQUENCEDETECTION_HPP_

#include "Settings.hxx"
#include "Constants.h"
//#include "SearchTree.hpp"

class SearchTree;
class TreeNode;

class ActionSequenceDetection {
public:
	ActionSequenceDetection(Settings & settings);
	virtual ~ActionSequenceDetection();

	void getJunkActionSequence(SearchTree* tree, int seqLength);

	std::vector<bool> getUsefulActions(std::vector<Action> previousActions);
	int getDetectedUsedActionsSize();

private:
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

	int t_size(int seqLength);

	/**
	 * Probablistic action selection 2016/09/01
	 *
	 */
	std::vector<bool> getWeightedProbableActions(
			std::vector<Action> previousActions);
	std::vector<double> getQvaluesOfNextActions(
			std::vector<Action> previousActions);
	std::vector<double> getQvaluesOfAllSequences(int seqLength);
	double calcQvalue(int sequence, int seqLength);
	double calcNovelRatio(int n_steps_before, int sequence, int seqLength);

	double sigmoid(double x, double gain);

	int probablistic_action_selection;
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

//	double previous_qvalue;
};

#endif /* SRC_AGENTS_ACTIONSEQUENCEDETECTION_HPP_ */
