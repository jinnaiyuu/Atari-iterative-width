/* *****************************************************************************
 * a.L.E (Arcade Learning Environment)
 * Copyright (c) 2009-2012 by Yavar Naddaf, Joel Veness, Marc G. Bellemare
 * Released under GNU General Public License www.gnu.org/licenses/gpl-3.0.txt
 *
 * Based on: Stella  --  "An Atari 2600 VCS Emulator"
 * Copyright (c) 1995-2007 by Bradford W. Mott and the Stella team
 *
 * *****************************************************************************
 *  Brute.cpp
 *
 *  A subclass of SearchTree that implements UCT. 
 **************************************************************************** */
#include "Brute.hpp"

#include "DominatedActionSequenceDetection.hpp"
#include "BruteTreeNode.hpp"

#include "random_tools.h"
#include "misc_tools.h"
#include <algorithm>

Brute::Brute(RomSettings * rom_settings, Settings &settings,
		ActionVect &actions, StellaEnvironment* _env) :
		SearchTree(rom_settings, settings, actions, _env) {
	epsilon = settings.getFloat("brute_epsilon", false);
	if (epsilon < 0.0) {
		epsilon = 0.005;
	}
}

/* *********************************************************************
 Destructor
 ******************************************************************* */
Brute::~Brute() {
}

/* *********************************************************************
 Builds a new tree
 ******************************************************************* */
void Brute::build(ALEState & state) {
	assert(p_root == NULL);

	p_root = new BruteTreeNode(NULL, state, 0, UNDEFINED, this);

	update_tree();

	is_built = true;
}

/* *********************************************************************
 Re-Expands the tree until max_sim_steps_per_frame is reached, if required;
 or until we have the required number of UCT simulations.
 ******************************************************************* */
void Brute::update_tree(void) {
	m_generated_nodes = 0;
	m_expanded_nodes = 0;
	m_max_depth = 0;
	int simulation_steps = ((BruteTreeNode*) p_root)->num_steps();

	std::cout << "starting with " << simulation_steps << " simulation steps"
			<< std::endl;

	int num_iterations = 0;
	while (true) {
		int new_sim_steps = single_brute_iteration();
		simulation_steps += new_sim_steps;
		num_iterations++;

		// If max_sim_steps_per_frame is set, stop once we reach a certain number
		//  of frames
		if (max_sim_steps_per_frame != -1
				&& simulation_steps >= max_sim_steps_per_frame) {
			printf(
					"break by max_sim_steps_per_frame: %d simulation_steps >= %d\n",
					simulation_steps, max_sim_steps_per_frame);
			break;
		}

		// Handle the case where we cannot simulate further but have not reached the
		//  maximum number of simulation steps per frame (thanks to Erik Talvitie
		//  for this one)
		else if (num_simulations_per_frame == -1 && new_sim_steps == 0) {
			printf("new_sim_steps == 0\n");
			break;
		}
	}
	std::cout << "Simulation steps: " << simulation_steps << std::endl;
	std::cout << "Visits to root: " << ((BruteTreeNode*) p_root)->visit_count
			<< std::endl;
	total_simulation_steps += simulation_steps;

//	print_tree();
}

void Brute::print_path(TreeNode * node, int a) {
	cerr << "Path, return " << node->v_children[a]->branch_return << endl;

	while (!node->is_leaf()) {
		TreeNode * child = node->v_children[a];

		cerr << "\tAction " << a << " Reward " << child->node_reward << " "
				<< child->is_terminal << endl;

		node = child;
		if (!node->is_leaf())
			a = get_best_branch((BruteTreeNode*) node, false);
	}
}

void Brute::move_to_branch(Action a, int duration) {
	SearchTree::move_to_branch(a, duration);
}

/* *********************************************************************
 Returns the best action based on the expanded search tree
 ******************************************************************* */
Action Brute::get_best_action(void) {
	assert(p_root != NULL);

	// This returns the best branch, but also sets the value of branch_return
	//  for all children of p_root
	int best_branch;

	best_branch = get_best_branch((BruteTreeNode*) p_root, false);

	assert(best_branch != -1);

	// Replace the best branch by our actual choice 
	p_root->best_branch = best_branch;

	for (size_t c = 0; c < p_root->v_children.size(); c++) {
		TreeNode* curr_child = p_root->v_children[c];

		std::cout << "Action: " << action_to_string(available_actions[c])
				<< " Depth: " << curr_child->branch_depth << " NumNodes: "
				<< curr_child->num_nodes() << " Reward: "
				<< curr_child->branch_return << std::endl;

	}

	std::cout << "Action " << best_branch << std::endl;
	return available_actions[best_branch];
}

/* *********************************************************************
 Performs a single UCT iteration, starting from the root
 ******************************************************************* */
int Brute::single_brute_iteration(void) {
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

			int choice = choose_child((BruteTreeNode*) node);
			node = node->v_children[choice];
			leaf_choice = node->act;
		}

// If this is not an unvisited child, then we should expand the node
//  so that it isn't a leaf anymore
		if (!zero_count_leaf) {
			expand_node(node);

			vector<Action> usefulActions = getEffectiveActionsVector(node);

//			printf("usefulActions = ");
//			for (unsigned int a = 0; a < usefulActions.size(); ++a) {
//				printf("%d ", (int) usefulActions[a]);
//			}
//			printf("\n");

			//			printf("223: %d\n", usefulActions.size());
			if (usefulActions.size() == 0) {
				// TODO: recheck this
				usefulActions = available_actions;
			}
//			int c = choice(&usefulActions);
			int c = rand() % available_actions.size(); // TODO:
			node = node->v_children[c];

			leaf_choice = available_actions[c];
		}

// We should be picking a node here that has never been simulated
//		assert(!node->is_initialized());
		assert(node != nullptr);
		assert(node->p_parent != nullptr);

		node->m_depth = node->p_parent->m_depth + 1;
		if (node->depth() > m_max_depth)
			m_max_depth = node->depth();

		if (!node->initialized) {
			printf("uninitialized\n");
			node->init(this, leaf_choice, sim_steps_per_node);
		}

// Before declaring ourselves done, ensure that this is not a duplicate
//  of another action
		if (ignore_duplicates) {
			done = !test_duplicate(node);
		} else {
			done = true;
		}
	}
//	printf("leaf_choice=%d\n", leaf_choice);

//	int sim_steps = node->num_simulated_steps;
//	assert(
//			sim_steps != sim_steps_per_node
//					&& [&sim_steps] {printf("simsteps=%d\n", sim_steps); return true;});
	int sim_steps = 5;
//	printf("sim_steps=%d\n", node->num_simulated_steps);

// Now that we have a leaf - perform Monte Carlo sampling from it
	float mc_return;

	int node_depth = node->state.getFrameNumber()
			- p_root->state.getFrameNumber();

	// TODO: Why doesn't it go further than the maximal depth?
//	int mc_steps = uct_search_depth - node_depth;

	float average_return = node->node_reward;

	// Propagate the return back up
	update_values((BruteTreeNode*) node, average_return);

	return sim_steps;
}

// Brute Action Selection Policy.
int Brute::choose_child(const BruteTreeNode* node) const {
	double explr_chance = (double) epsilon / log(node->visit_count + 1);
	if (rand() < explr_chance * RAND_MAX) {
//		printf("explore by %.2f\n", explr_chance);
		return rand() % node->v_children.size();
	} else {
		float max_ret = std::numeric_limits<float>::min();
		vector<int> max_childs;
		for (size_t c = 0; c < node->v_children.size(); ++c) {
			BruteTreeNode* child = (BruteTreeNode*) node->v_children[c];
			if (max_ret < child->max_return) {
				max_ret = child->max_return;
				max_childs.clear();
				max_childs.push_back(c);
			} else if (max_ret == max_ret) {
				max_childs.push_back(c);
			}
		}
		assert(max_childs.size() > 0);
		return choice(&max_childs);
	}

}

/**
 * Let's not delete uct_bias here as we may be able to make use of it later.
 */
int Brute::get_best_branch(BruteTreeNode* node, bool add_uct_bias) {
	int best_branch = -1;
	float highest_value = 0;

	assert(!node->v_children.empty());

	vector<Action> usefulAction = getEffectiveActionsVector(node);

// We want to tie-break actions with the same value
	vector<int> ties;

	for (size_t c = 0; c < node->v_children.size(); c++) {
		BruteTreeNode * child = (BruteTreeNode*) node->v_children[c];
		if (find(usefulAction.begin(), usefulAction.end(), child->act)
				== usefulAction.end()) {
//			printf("PRUNED action %s\n", action_to_string(child->act).c_str());
			continue;
		}

// Skip unvisited children (they should be selected explicitly)
		if (!child->is_initialized() || child->visit_count == 0)
			continue;

// Compute this child's value, adding the UCT bonus if required
//		float v = child->sum_returns / child->visit_count;
//
//		if (add_uct_bias) {
//			// The UCT bias (see Kocsis & Szepesvari (2006))
//			float bias = sqrt(
//					log(node->visit_count + 0.0) / child->visit_count);
//			bias *= uct_exploration_constant;
//
//			v = v + bias;
//		}

// Set the return to the computed value -- note! This is only used
//  for computation purposes; branch_return is not guaranteed to be
//  valid
		child->branch_return = child->max_return;
		float v = child->max_return;
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
	if (ties.size() > 1) {
		best_branch = choice(&ties);
//		assert(best_branch != -1 && "ties has -1 branch");
	}

	if (best_branch == -1) {
		best_branch = rand() % node->v_children.size();
	}
	assert(best_branch != -1);

	return best_branch;
}

int Brute::get_most_visited_branch(BruteTreeNode * node) {
	int maxVisits = -1;
	int bestBranch = -1;

	IntVect ties;

	for (size_t c = 0; c < node->v_children.size(); c++) {
// Ignore unitialized children (though they also should have 0 visits)
		BruteTreeNode * child = ((BruteTreeNode*) node->v_children[c]);

		if (child->is_duplicate() || !child->is_initialized())
			continue;

		child->branch_return = child->max_return; // / child->visit_count;

// Find the maximum visit child
		if (bestBranch == -1 || child->visit_count > maxVisits) {
			bestBranch = c;
			maxVisits = child->visit_count;
			ties.clear();
			ties.push_back(bestBranch);
		} else if (child->visit_count == maxVisits) {
			ties.push_back(c);
		}
	}

	assert(!ties.empty() && "get_most_visited_branch");

// Return at random one of the max. visit children
	return choice(&ties);
}

/* *********************************************************************
 Expands the given node, by generating all its children
 ******************************************************************* */
void Brute::expand_node(TreeNode* node) {
	m_expanded_nodes++;

// TODO:;
//	if (action_sequence_detection) {
//		if (!trajectory.empty()) {
//			vector<Action> p = getPreviousActions(node,
//					longest_junk_sequence - 1);
//			isUsefulAction = asd->getUsefulActions(p);
//		}
//	}

// YJ: This function does not actually "expand" the node.
//     It does not run emulation to generate the successor state.
//     Rather it places TreeNode to put into v_children.
	for (size_t i = 0; i < available_actions.size(); i++) {
		// Should we simulate after or before evaluation?
//		BruteTreeNode *child = new BruteTreeNode(node, node->state);
//		child->init(this, available_actions[i], sim_steps_per_node);

		BruteTreeNode * child = new BruteTreeNode(node, node->state,
				sim_steps_per_node, available_actions[i], this);

//		printf("child-act = %d\n", (int) child->act);
		node->v_children.push_back(child);
	}
}

/* *********************************************************************
 Update the node values and counters from the current node, all the
 way up to the root
 ******************************************************************* */
void Brute::update_values(BruteTreeNode* node, return_t mc_return) {
// Update the visit count
	node->visit_count++;

	return_t total_return = discount_factor * mc_return + node->node_reward;

	if (total_return > node->max_return) {
		node->max_return = total_return;
	}

	if (node->p_parent != NULL)
		update_values((BruteTreeNode*) node->p_parent,
				total_return * discount_factor);
}

vector<Action> Brute::getEffectiveActionsVector(const TreeNode* node) const {
	vector<Action> usefulActions;
	if (action_sequence_detection && trajectory.size() > dasd_sequence_length) {
		vector<Action> p = getPreviousActions(node, dasd_sequence_length);
		vector<bool> isUsefulAction = dasd->getEffectiveActions(p,
				this->trajectory.size() * sim_steps_per_node);
		for (unsigned int a = 0; a < isUsefulAction.size(); ++a) {
			if (isUsefulAction[a]) {
				usefulActions.push_back((Action) a);
			}
		}
	} else {
		usefulActions = available_actions;
	}
	return usefulActions;
}

void Brute::print_tree() {
	printf("*** TREE ***\n");
	print_node((BruteTreeNode*) p_root);

}

void Brute::print_node(BruteTreeNode* node) {
	if (node->visit_count == 0) {
		return;
	}

	int depth = node->depth();
	for (int i = 0; i < depth; ++i) {
		if (i == depth - 1) {
			printf("|-");
		} else {
			printf("  ");
		}
	}
	printf("%d\n", node->visit_count);
	if (node->v_children.size() > 0) {
		for (int i = 0; i < node->v_children.size(); ++i) {
			print_node((BruteTreeNode*) node->v_children[i]);
		}
	}
}
