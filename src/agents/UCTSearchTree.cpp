/* *****************************************************************************
 * a.L.E (Arcade Learning Environment)
 * Copyright (c) 2009-2012 by Yavar Naddaf, Joel Veness, Marc G. Bellemare
 * Released under GNU General Public License www.gnu.org/licenses/gpl-3.0.txt
 *
 * Based on: Stella  --  "An Atari 2600 VCS Emulator"
 * Copyright (c) 1995-2007 by Bradford W. Mott and the Stella team
 *
 * *****************************************************************************
 *  UCTSearchTree.cpp 
 *
 *  A subclass of SearchTree that implements UCT. 
 **************************************************************************** */
#include "UCTSearchTree.hpp"
#include "UCTTreeNode.hpp"

#include "random_tools.h"
#include "misc_tools.h"

UCTSearchTree::UCTSearchTree(RomSettings * rom_settings, Settings &settings,
			     ActionVect &actions, StellaEnvironment* _env):
	SearchTree(rom_settings, settings, actions, _env) {
 
	uct_max_simulations = settings.getInt("uct_monte_carlo_steps",true); 
	uct_search_depth = settings.getInt("uct_search_depth", true);
	uct_exploration_constant = 
		settings.getFloat("uct_exploration_constant", true);
}

/* *********************************************************************
   Destructor
   ******************************************************************* */
UCTSearchTree::~UCTSearchTree() {
}

/* *********************************************************************
   Builds a new tree
   ******************************************************************* */
void UCTSearchTree::build(ALEState & state) {
	assert(p_root == NULL);

	p_root = new UCTTreeNode(NULL, state, 0, UNDEFINED, this);
  
	update_tree();

	is_built = true;
}

/* *********************************************************************
   Re-Expands the tree until max_sim_steps_per_frame is reached, if required;
   or until we have the required number of UCT simulations. 
   ******************************************************************* */
void UCTSearchTree::update_tree(void) {
	m_generated_nodes=0;
	m_expanded_nodes=0;
	m_max_depth=0;
	int simulation_steps = ((UCTTreeNode*)p_root)->num_steps();
	
	std::cout << "starting with " << simulation_steps << " simulation steps" <<std::endl;
	
	int num_iterations = 0;
	while (true) {
		int new_sim_steps = single_uct_iteration();
		simulation_steps += new_sim_steps; 
		num_iterations++;

		// If max_sim_steps_per_frame is set, stop once we reach a certain number
		//  of frames
		if (max_sim_steps_per_frame != -1 && 
		    simulation_steps >= max_sim_steps_per_frame)
			break;
		//else if (num_simulations_per_frame != -1 &&
		//	 ((UCTTreeNode*)p_root)->visit_count >= num_simulations_per_frame)
		//	break;
		else if ( uct_max_simulations != -1 && 
			((UCTTreeNode*)p_root)->visit_count >= uct_max_simulations)
			break;	
		// Handle the case where we cannot simulate further but have not reached the
		//  maximum number of simulation steps per frame (thanks to Erik Talvitie
		//  for this one)
		else if (num_simulations_per_frame == -1 && new_sim_steps == 0)
			break;
	}
	std::cout << "Simulation steps: " << simulation_steps << std::endl;
	std::cout << "Visits to root: " << ((UCTTreeNode*)p_root)->visit_count << std::endl;
	total_simulation_steps += simulation_steps;
}

void UCTSearchTree::print_path(TreeNode * node, int a) {
	cerr << "Path, return " << node->v_children[a]->branch_return << endl;

	while (!node->is_leaf()) {
		TreeNode * child = node->v_children[a];

		cerr << "\tAction " << a << " Reward " << child->node_reward << " " << child->is_terminal << endl;
    
		node = child;
		if (!node->is_leaf())
			a = get_best_branch((UCTTreeNode*)node, false);
	}
}

/* *********************************************************************
   Returns the best action based on the expanded search tree
   ******************************************************************* */
Action UCTSearchTree::get_best_action(void) {
	assert(p_root != NULL);

	// This returns the best branch, but also sets the value of branch_return
	//  for all children of p_root
	int best_branch;
  
	// best_branch = get_best_branch((UCTTreeNode*)p_root, false);
	// Select based on most explored action
	best_branch = get_most_visited_branch((UCTTreeNode*)p_root);
  
	assert(best_branch != -1);

	// Replace the best branch by our actual choice 
	p_root->best_branch = best_branch;


	for (size_t c = 0; c < p_root->v_children.size(); c++) {
	    TreeNode* curr_child = p_root->v_children[ c ];
	    
	    std::cout << "Action: " << action_to_string(available_actions[c]) << " Depth: " << curr_child->branch_depth  <<" NumNodes: " << curr_child->num_nodes() << " Reward: "<< curr_child->branch_return   << std::endl;

	}
	
	std::cout << "Action " << best_branch << std::endl;
	return available_actions[best_branch];
}

/* *********************************************************************
   Performs a single UCT iteration, starting from the root
   ******************************************************************* */
int UCTSearchTree::single_uct_iteration(void) {
	// Traverse the tree down to a leaf node; traversal happens using UCT
	//  exploration
	TreeNode * node; 
	Action leaf_choice;

	bool done = false;
	// Traverse the tree down to a leaf node that results in a unique state
	while (!done) {
		bool zero_count_leaf = false; 
		node = p_root;

		while (!node->is_leaf()) {
			// ensure that not all children are duplicates 
			bool all_dups = true;
			for (size_t c = 0; c < node->v_children.size(); c++) {
				TreeNode * child = node->v_children[c];
				if (!child->is_duplicate()) {
					all_dups = false;
					break;
				}
			}

			assert(!all_dups);

			int unvisited_child = get_child_with_count_zero(node);
			if (unvisited_child != -1) {
				zero_count_leaf = true;
				node = node->v_children[unvisited_child];
				leaf_choice = available_actions[unvisited_child];
      
				assert(node->is_leaf());
				break;
			}
			else {
				// Select an action via UCT exploration
				int uct_choice = get_best_branch((UCTTreeNode*)node, true);
				node = node->v_children[uct_choice];
			}
		}

		// If this is not an unvisited child, then we should expand the node 
		//  so that it isn't a leaf anymore
		if (!zero_count_leaf) {
			expand_node(node);
    
			// Now visit (at random) one of the children of that node 
			int c = rand_range(0, available_actions.size()-1);
			node = node->v_children[c];

			leaf_choice = available_actions[c];
		}

		// We should be picking a node here that has never been simulated
		assert(!node->is_initialized());
		
		
		node->m_depth = node->p_parent->m_depth + 1;
		if (node->depth() > m_max_depth ) m_max_depth = node->depth();		

		node->init(this, leaf_choice, sim_steps_per_node); 

		// Before declaring ourselves done, ensure that this is not a duplicate
		//  of another action
		if (ignore_duplicates)
			done = !test_duplicate(node);
		else
			done = true;
	}

	int sim_steps = 0;

	// Now that we have a leaf - perform Monte Carlo sampling from it
	float mc_return;

	int node_depth = node->state.getFrameNumber() - 
		p_root->state.getFrameNumber();
  
	int mc_steps = uct_search_depth - node_depth; 
  
	sim_steps += do_monte_carlo((UCTTreeNode*)node, mc_steps, mc_return);
	
	
	// Propagate the return back up
	update_values((UCTTreeNode*)node, mc_return);

	return sim_steps;
}

/* *********************************************************************
   Returns the index of the first child with zero count
   Returns -1 if no such child is found
   ******************************************************************* */
int UCTSearchTree::get_child_with_count_zero(const TreeNode* node) const {
	// This method gets call infrequently enough that there is little gain in
	//  making unvisited_children a class member variable
	IntVect unvisited_children;

	for (size_t c = 0; c < node->v_children.size(); c++) {
		UCTTreeNode * child = (UCTTreeNode*)node->v_children[c];

		if (!child->is_duplicate() && child->visit_count == 0)
			unvisited_children.push_back(c);
	}

	if (unvisited_children.empty())
		return -1;
	else
		return choice(&unvisited_children);
}

/* *********************************************************************
   Returns the sub-branch with the highest value if add_uct_bias is true 
   we will add the UCT bonus to each branch value and then take the max
   ******************************************************************* */
int UCTSearchTree::get_best_branch(UCTTreeNode* node, bool add_uct_bias) {
	int best_branch = -1;
	float highest_value = 0;

	assert(!node->v_children.empty());

	// We want to tie-break actions with the same value 
	vector<int> ties;

	for (size_t c = 0; c < node->v_children.size(); c++) {
		UCTTreeNode * child = (UCTTreeNode*)node->v_children[c];

		// Skip unvisited children (they should be selected explicitly)
		if (!child->is_initialized() || child->visit_count == 0) 
			continue;

		// Compute this child's value, adding the UCT bonus if required
		float v = child->sum_returns / child->visit_count;

		if (add_uct_bias) {
			// The UCT bias (see Kocsis & Szepesvari (2006))
			float bias = sqrt(log(node->visit_count + 0.0) / child->visit_count);
			bias *= uct_exploration_constant;

			v = v + bias; 
		}

		// Set the return to the computed value -- note! This is only used
		//  for computation purposes; branch_return is not guaranteed to be
		//  valid
		child->branch_return = v;

		// Update the maximum 
		if (best_branch == -1 || v > highest_value) {
			best_branch = c;
			highest_value = v;
      
			ties.clear();
			ties.push_back(best_branch);
		}
		// If tied, add to the list of ties
		else if (v == highest_value) 
			ties.push_back(c);
	}

	// If we have ties, pick one at random
	if (ties.size() > 1)
		best_branch = choice(&ties);

	assert(best_branch != -1);

	return best_branch;
}

int UCTSearchTree::get_most_visited_branch(UCTTreeNode * node) {
	int maxVisits = -1;
	int bestBranch = -1;
  
	IntVect ties;

	for (size_t c = 0; c < node->v_children.size(); c++) {
		// Ignore unitialized children (though they also should have 0 visits)
		UCTTreeNode * child = ((UCTTreeNode*)node->v_children[c]);

		if (child->is_duplicate() || !child->is_initialized()) continue;

		child->branch_return = child->sum_returns / child->visit_count;

		// Find the maximum visit child
		if (bestBranch == -1 || child->visit_count > maxVisits) {
			bestBranch = c;
			maxVisits = child->visit_count;
			ties.clear();
			ties.push_back(bestBranch);
		}
		else if (child->visit_count == maxVisits) {
			ties.push_back(c);
		}
	}

	// Return at random one of the max. visit children
	return choice(&ties);
}

/* *********************************************************************
   Expands the given node, by generating all its children
   ******************************************************************* */
void UCTSearchTree::expand_node(TreeNode* node) {

	m_expanded_nodes++;
	for (size_t i = 0; i < available_actions.size(); i++) {
		UCTTreeNode *child = new UCTTreeNode(node, node->state);
		node->v_children.push_back(child);		
	}
}

/* *********************************************************************
   Performs a Monte Carlo simulation from the given node, for
   'num_steps' steps 
   ******************************************************************* */
int UCTSearchTree::do_monte_carlo(UCTTreeNode* start_node, 
				  int num_steps, return_t& mc_return) {
  
	m_generated_nodes++;

	m_env->restoreState(start_node->state);
	bool is_terminal;

	// Return the number of simulated steps (less than num_steps 
	//  when we reach a terminal state)
	int steps = simulate_game(start_node->state, RANDOM, num_steps, 
				  mc_return, is_terminal, true, false);
	
	start_node->sim_steps += steps;
	// Reset its frame number for consistency
	m_env->restoreState(start_node->state);

	// Note - we do not care here if we reach a terminal node (indicated
	//  by is_terminal being set to true)
	return steps;
}

/* *********************************************************************
   Update the node values and counters from the current node, all the
   way up to the root
   ******************************************************************* */
void UCTSearchTree::update_values(UCTTreeNode* node, return_t mc_return) {
	// Update the visit count
	node->visit_count++;

	// Update our estimate of the return at this node as the sum of the 
	//  Monte-Carlo return (including expanded child nodes below) and this
	//  node's immediate reward
	return_t total_return = discount_factor * mc_return + node->node_reward;

	node->sum_returns += total_return; 

	// Recursively update the parent (this can also be done in a loop at the 
	//  cost of legibility)
	if (node->p_parent != NULL)
		update_values((UCTTreeNode*)node->p_parent, total_return * discount_factor);
}
