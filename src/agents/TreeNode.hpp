/* *****************************************************************************
 * A.L.E (Arcade Learning Environment)
 * Copyright (c) 2009-2012 by Yavar Naddaf, Joel Veness, Marc G. Bellemare
 * Released under GNU General Public License www.gnu.org/licenses/gpl-3.0.txt
 *
 * Based on: Stella  --  "An Atari 2600 VCS Emulator"
 * Copyright (c) 1995-2007 by Bradford W. Mott and the Stella team
 *
 * *****************************************************************************
 *  TreeNode.hpp 
 *
 *  Implementation of the TreeNode class, which represents a node in the search-
 *	tree for the Search-Agent
 **************************************************************************** */

#ifndef __TREE_NODE_HPP__
#define __TREE_NODE_HPP__

#include "Constants.h"
#include "../environment/ale_state.hpp"

class SearchAgent;
class SearchTree;
class TreeNode;
typedef vector<TreeNode*> NodeList;

class TreeNode {
    /* *************************************************************************
        Represents a node in the search-tree for the Search-Agent
    ************************************************************************* */

    public:
	/* *********************************************************************
            Constructor
	Generates a new uninitialized tree node.
	******************************************************************* */
	TreeNode(	TreeNode *parent, ALEState &parentState);	

    	/** Generates a tree node, then initializes it by simulating the result
      	*  from starting in parentState and taking action a for a given number
	*  of steps.
	*/
	TreeNode(	TreeNode *parent, ALEState &parentState, 
  	SearchTree *tree, Action a, int num_simulate_steps, float discount = 1.0);	

      	/** Properly generate this node by simulating it from the start state */
	void init(SearchTree * tree, Action a, int num_simulate_steps);


	/**
	 * Updates Tree Node info if it's reused from previous lookahead
	 */
	void updateTreeNode();
	/* *********************************************************************
	Returns true if this is a leaf node
	******************************************************************* */
	bool is_leaf(void) {
		return (v_children.empty());
	}
    
    	// Whether this node has been initialized, e.g. simulated forward
	bool is_initialized() { return initialized; }
	bool is_duplicate() { return duplicate; }

	int num_nodes();
    int depth(){ return m_depth;}

	ALEState state;

	// How many steps were simulated to obtain this node
	int num_simulated_steps; 

	reward_t node_reward;	// immediate reward recieved in this node
	reward_t discounted_node_reward;	// immediate reward recieved in this node * discount_factor
	return_t branch_return;	// estimated (max or average) reward for this subtree 
	return_t branch_depth;	// depth this subtree 
	bool is_terminal; // whether this is a terminal node 
	int best_branch;	// Best sub-branch that can be taken 
				// from the current node
	NodeList v_children;// vector of children nodes
	TreeNode* p_parent;	// pointer to our parent

    	// Whether this node has been simulated 
	bool initialized;

    	// Whether this node was flagged as a duplicate
    	bool duplicate;
        unsigned m_depth;

	unsigned long long	fn; // evaluation function
        unsigned                novelty;
	reward_t	        accumulated_reward; // evaluation function
	reward_t	        discounted_accumulated_reward; // evaluation function
	float                   discount;
	float                   original_discount;
	Action                  act;
	ActionVect              available_actions;
	bool                    already_expanded;
    
        unsigned                num_nodes_reusable;
};



#endif // __TREE_NODE_HPP__
