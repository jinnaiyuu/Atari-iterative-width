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

#ifndef __UCT_TREE_NODE_HPP__
#define __UCT_TREE_NODE_HPP__

#include "TreeNode.hpp"

class UCTTreeNode: public TreeNode  {
  public:
    UCTTreeNode(TreeNode *parent, ALEState &parentState); 
    
    UCTTreeNode(TreeNode *parent, ALEState &parentState, 
      int num_simulate_steps, Action a, SearchTree *tree);	

    int num_steps() {
	int num_steps = 0;
	
	for (size_t a = 0; a < v_children.size(); a++) {
	    if (v_children[a]->is_initialized())
		num_steps += ((UCTTreeNode*)v_children[a])->num_steps();
	}
	
	return num_steps + sim_steps;
    }
    
    /** Additional UCT-specific variables */
    int visit_count; // The number of visits to this node
    float sum_returns; // The sum of the returns received from this node
    int sim_steps; // The Simulation of the rollout steps
};

#endif // __UCT_TREE_NODE_HPP__

