/*
 * Priorities.hpp
 *
 *  Created on: Jul 11, 2016
 *      Author: yuu
 */

#ifndef SRC_AGENTS_PRIORITIES_HPP_
#define SRC_AGENTS_PRIORITIES_HPP_

class TreeNodeComparerReward {
public:
	// TODO: IMPORTANT: Note that std::priority_queue puts the LEAST
	// significant node to the top. Therefore, THE SMALLER THE BETTER, thus accumulated reward should be flipped.
	bool operator()(TreeNode* a, TreeNode* b) const {
		if (b->accumulated_reward > a->accumulated_reward)
			return true;
		else if (b->accumulated_reward == a->accumulated_reward
				&& b->novelty < a->novelty)
			return true;
		return false;
	}
};

class TreeNodeComparerAdditiveNovelty {
public:
	// TODO: IMPORTANT: Note that std::priority_queue puts the LEAST
	// significant node to the top. Therefore, THE SMALLER THE BETTER, thus accumulated reward should be flipped.
	bool operator()(TreeNode* a, TreeNode* b) const {
		if (b->additive_novelty > a->additive_novelty)
			return true;
		else if (b->additive_novelty == a->additive_novelty
				&& b->accumulated_reward > a->accumulated_reward)
			return true;
		return false;
	}
};

// Fn is defined in PIW1Search.cpp. as a function of reward and (additive)novelty.
class TreeNodeComparerFunction {
public:
	bool operator()(TreeNode* a, TreeNode* b) const {
		if (b->fn < a->fn)
			return true;
		else
			return false;
	}
};

#endif /* SRC_AGENTS_PRIORITIES_HPP_ */
