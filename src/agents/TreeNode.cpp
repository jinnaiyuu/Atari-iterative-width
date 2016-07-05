/* *****************************************************************************
 * A.L.E (Arcade Learning Environment)
 * Copyright (c) 2009-2012 by Yavar Naddaf, Joel Veness, Marc G. Bellemare
 * Released under GNU General Public License www.gnu.org/licenses/gpl-3.0.txt
 *
 * Based on: Stella  --  "An Atari 2600 VCS Emulator"
 * Copyright (c) 1995-2007 by Bradford W. Mott and the Stella team
 *
 * *****************************************************************************
 *  TreeNode.cpp
 *
 *  Implementation of the TreeNode class, which represents a node in the search-
 *	tree for the Search-Agent
 **************************************************************************** */

#include "TreeNode.hpp"
#include "SearchTree.hpp"

/* *********************************************************************
	Constructor
	Generates a new tree node by starting from start_state and 
	simulating the game for num_simulate_steps steps.
 ******************************************************************* */
TreeNode::TreeNode(	TreeNode* parent, ALEState &parentState):
  state(parentState), // Copy constructor of the parent state
  node_reward(0), 
  discounted_node_reward(0),
  branch_return(0),
  branch_depth(0),
  is_terminal(false),
  best_branch(-1),
  p_parent(parent),
  initialized(false),
  duplicate(false),
  m_depth(0),
  fn(0),
  novelty(0),
  accumulated_reward(0),
  discounted_accumulated_reward(0),
  discount(1.0),
  act(Action::PLAYER_A_NOOP),
  already_expanded(false),
  num_nodes_reusable(0)

{
}

TreeNode::TreeNode(	TreeNode* parent, ALEState &parentState, 
			SearchTree * tree, Action a, int num_simulate_steps, float disc):
	state(parentState), // Copy constructor of the parent state
	node_reward(0), 
	discounted_node_reward(0),
	branch_return(0),
	branch_depth(0),
	is_terminal(false),
	best_branch(-1),
	p_parent(parent), 
	initialized(false),
	duplicate(false) ,
	fn(0),
	novelty(0),
	accumulated_reward(0),
	discounted_accumulated_reward(0),
	discount(disc),
	original_discount(disc),
	act(a),
	already_expanded(false),
	num_nodes_reusable(0)
{
	if(parent == NULL){
		m_depth = 0;
	}
	else{
		m_depth = parent->m_depth + 1;
		discount = parent->discount * discount;
	}

	if(tree){
		init(tree, a, num_simulate_steps);
		discounted_node_reward = node_reward * discount;
		if(parent == NULL){
			accumulated_reward = node_reward;
			discounted_accumulated_reward = discounted_node_reward;
		}
		else{
			accumulated_reward = parent->accumulated_reward + node_reward;
			discounted_accumulated_reward = parent->discounted_accumulated_reward + discounted_node_reward;
		}


	}
}

void TreeNode::updateTreeNode(){
	if(p_parent == NULL){
		m_depth = 0;
	}
	else{
		m_depth = p_parent->m_depth + 1;
		discount = p_parent->discount * original_discount;
	}

	discounted_node_reward = node_reward * discount;
	if(p_parent == NULL){
		accumulated_reward = node_reward;
		discounted_accumulated_reward = discounted_node_reward;
	}
	else{
		accumulated_reward = p_parent->accumulated_reward + node_reward;
		discounted_accumulated_reward = p_parent->discounted_accumulated_reward + discounted_node_reward;
	}



}

void TreeNode::init(SearchTree * tree, Action a, int num_simulate_steps) { 
  // Simulate from this state
    return_t step_return;
    num_simulated_steps = tree->simulate_game(state, a, num_simulate_steps, 
					      step_return, is_terminal, false);
    node_reward = (reward_t)step_return;
  

    // Initialize the branch reward to the received node reward
    branch_return = node_reward;

    initialized = true;
}

int TreeNode::num_nodes() {
  int numNodes = 0;

  for (size_t a = 0; a < v_children.size(); a++) {
    if (v_children[a]->is_initialized())
      numNodes += v_children[a]->num_nodes();
  }

  return numNodes + 1;
}
