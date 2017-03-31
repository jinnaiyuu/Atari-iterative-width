/*
 * DominatedActionSequenceAvoidance.hpp
 *
 *  Created on: Mar 30, 2017
 *      Author: yuu
 */

#ifndef SRC_AGENTS_DOMINATEDACTIONSEQUENCEAVOIDANCE_HPP_
#define SRC_AGENTS_DOMINATEDACTIONSEQUENCEAVOIDANCE_HPP_

#include "DominatedActionSequenceDetection.hpp"

class DominatedActionSequenceAvoidance: public DominatedActionSequenceDetection {
public:
	DominatedActionSequenceAvoidance(Settings & settings,
			StellaEnvironment* _env);
	virtual ~DominatedActionSequenceAvoidance();
	void learnDominatedActionSequences(SearchTree* tree, int seqLength);

	std::vector<bool> getEffectiveActions(
			std::vector<Action> previousActions, int current_frame);
	int getDetectedUsedActionsSize();

protected:
	void getUsedSequenceList(TreeNode* node, int seqLength,
			std::vector<bool>& isSequenceUsed);
private:
	void learnDASA(SearchTree* tree, int seqLength);
	std::vector<double> novelty_to_probabilities(int seqLength);
	/**
	 * Dominated Action Sequence Avoidance
	 */
	std::vector<bool> getDASAActionSet(std::vector<Action> previousActions);
	std::vector<double> getQvaluesOfNextActions(
			std::vector<Action> previousActions);
	std::vector<double> getQvaluesOfAllSequences(int seqLength);
	double calcQvalue(int sequence, int seqLength);
	double calcNovelRatio(int n_steps_before, int sequence, int seqLength);

	double sigmoid(double x, double gain);


};

#endif /* SRC_AGENTS_DOMINATEDACTIONSEQUENCEAVOIDANCE_HPP_ */
