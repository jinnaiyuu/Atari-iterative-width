/*
 * DominatedActionSequencePruning.hpp
 *
 *  Created on: Mar 30, 2017
 *      Author: yuu
 */

#ifndef SRC_AGENTS_DOMINATEDACTIONSEQUENCEPRUNING_HPP_
#define SRC_AGENTS_DOMINATEDACTIONSEQUENCEPRUNING_HPP_

#include "DominatedActionSequenceDetection.hpp"

class DominatedActionSequencePruning: public DominatedActionSequenceDetection {
public:
	DominatedActionSequencePruning(Settings & settings,
			StellaEnvironment* _env);
	~DominatedActionSequencePruning();

	void learnDominatedActionSequences(SearchTree* tree, int seqLength);

	std::vector<bool> getEffectiveActions(
			std::vector<Action> previousActions, int current_frame);
	int getDetectedUsedActionsSize();

protected:
	void getUsedSequenceList(TreeNode* node, int seqLength,
			std::vector<bool>& isSequenceUsed);
private:


	void learnDASP(SearchTree* tree, int seqLength);

};

#endif /* SRC_AGENTS_DOMINATEDACTIONSEQUENCEPRUNING_HPP_ */
