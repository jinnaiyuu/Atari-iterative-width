/*
 * ActionSequenceDetection.hpp
 *
 *  Created on: Aug 11, 2016
 *      Author: yuu
 */

#ifndef SRC_AGENTS_ACTIONSEQUENCEDETECTION_HPP_
#define SRC_AGENTS_ACTIONSEQUENCEDETECTION_HPP_

#include "Constants.h"
//#include "SearchTree.hpp"

class SearchTree;
class TreeNode;

class ActionSequenceDetection {
public:
	ActionSequenceDetection();
	virtual ~ActionSequenceDetection();

	void getJunkActionSequence(SearchTree* tree, int seqLength);

	std::vector<bool> getUsefulActions(std::vector<Action> previousActions);

private:
	void searchNode(TreeNode* tree, int seqLength,
			std::vector<bool>& isSequenceUsed);
	void getUsedSequenceList(TreeNode* node, int seqLength,
			std::vector<bool>& isSequenceUsed);
	void getUsedSequenceRec(TreeNode* node, int seqLength,
			std::vector<bool>& isSequenceUsed, std::vector<Action>& currSeq,
			int currDepth);
	int secToInt(std::vector<Action> sequence);
	std::vector<Action> intToSec(int seqInt, int seqLength);

	std::vector<TreeNode*> sortNodeList(std::vector<TreeNode*> childList);
	TreeNode* getResultingNode(TreeNode* root, std::vector<Action> sequence);

	std::vector<std::vector<bool> > usedActionSeqs;

//	bool nodeActionSort(const TreeNode* l, const TreeNode* r);

	int t_size(int seqLength);
};

#endif /* SRC_AGENTS_ACTIONSEQUENCEDETECTION_HPP_ */
