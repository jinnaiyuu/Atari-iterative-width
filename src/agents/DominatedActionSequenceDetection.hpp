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
#include "../ale_interface.hpp"
//#include "SearchTree.hpp"

class SearchTree;
class TreeNode;

class DominatedActionSequenceDetection {
public:
	DominatedActionSequenceDetection(Settings & settings,
			StellaEnvironment* _env);
	virtual ~DominatedActionSequenceDetection();

	virtual void learnDominatedActionSequences(SearchTree* tree,
			int seqLength) = 0;

	virtual std::vector<bool> getEffectiveActions(
			std::vector<Action> previousActions, int current_frame) = 0;
	virtual int getDetectedUsedActionsSize() = 0;

protected:

	virtual void getUsedSequenceList(TreeNode* node, int seqLength,
			std::vector<bool>& isSequenceUsed) = 0;
	void searchNode(TreeNode* tree, int seqLength,
			std::vector<bool>& isSequenceUsed);

	std::vector<Action> sortByNovelty(std::vector<bool> marked,
			std::vector<bool> minset);

	int seqToInt(std::vector<Action> sequence);
	std::vector<Action> intToSeq(int seqInt, int seqLength);

	std::vector<TreeNode*> sortNodeList(std::vector<TreeNode*> childList);
	TreeNode* getResultingNode(TreeNode* root, std::vector<Action> sequence);

	int num_sequences(int seqLength) const;

	std::vector<std::vector<bool> > isUsefulActionSequence;


	StellaEnvironment* m_env;
	int junk_decision_frame;

	int action_length;

//	std::vector<std::vector<int> > num_novel_node_by_action_sequence;
//	std::vector<std::vector<int> > num_duplicate_node_by_action_sequence;

	std::vector<VertexCover> dominance_graph;

	int permutateToOriginalAction(int input, int seqLength);
	bool permutate_action;
	std::vector<Action> action_permutation;

	// DA A
	std::vector<std::vector<double> > ratio_of_novelty; // length = 1
	std::vector<double> probabilty_by_action;

	double discount_factor;
	double epsilon;
	double junk_threshold;
	int maximum_steps_to_consider;
	std::vector<std::vector<std::vector<int> > > num_novel_node_by_action;
	std::vector<std::vector<std::vector<int> > > num_duplicate_node_by_action;

};


#endif /* SRC_AGENTS_DOMINATEDACTIONSEQUENCEDETECTION_HPP_ */
