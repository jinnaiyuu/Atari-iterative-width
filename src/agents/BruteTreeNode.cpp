/* *****************************************************************************
 * A.L.E (Arcade Learning Environment)
 * Copyright (c) 2009-2012 by Yavar Naddaf, Joel Veness, Marc G. Bellemare
 * Released under GNU General Public License www.gnu.org/licenses/gpl-3.0.txt
 *
 * Based on: Stella  --  "An Atari 2600 VCS Emulator"
 * Copyright (c) 1995-2007 by Bradford W. Mott and the Stella team
 *
 * *****************************************************************************
 *  UCTTreeNode.cpp 
 *
 *  A subclass of TreeNode which adds UCT-relevant variables. 
 * *****************************************************************************
 */
#include "BruteTreeNode.hpp"

BruteTreeNode::BruteTreeNode(TreeNode *parent, ALEState &parentState) :
		TreeNode(parent, parentState), visit_count(0), max_return(0) {
}

BruteTreeNode::BruteTreeNode(TreeNode *parent, ALEState &parentState,
		int num_simulate_steps, Action a, SearchTree *tree) :
		TreeNode(parent, parentState, tree, a, num_simulate_steps), visit_count(
				0), max_return(0) {
}
