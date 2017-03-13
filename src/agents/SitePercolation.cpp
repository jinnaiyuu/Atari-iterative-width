#include "SitePercolation.hpp"

#include "SearchAgent.hpp"
#include <list>

#include "DominatedActionSequenceDetection.hpp"

SitePercolation::SitePercolation(RomSettings *rom_settings, Settings &settings,
		ActionVect &actions, StellaEnvironment* _env) :
		SearchTree(rom_settings, settings, actions, _env) {

	m_expand_all_emulated_nodes = settings.getBool("expand_all_emulated_nodes",
			false);

	if (m_expand_all_emulated_nodes) {
		printf("Expands all previously emulated nodes\n");
	}

	m_pruned_nodes = 0;
}

SitePercolation::~SitePercolation() {
}

/* *********************************************************************
 Builds a new tree
 ******************************************************************* */
void SitePercolation::build(ALEState & state) {
	assert(p_root == NULL);
	p_root = new TreeNode(NULL, state, NULL, UNDEFINED, 0);
	update_tree();
	is_built = true;
}

void SitePercolation::print_path(TreeNode * node, int a) {
	cerr << "Path, return " << node->v_children[a]->branch_return << endl;

	while (!node->is_leaf()) {
		TreeNode * child = node->v_children[a];

		cerr << "\tAction " << a << " Reward " << child->node_reward
				<< " Return " << child->branch_return << " Terminal "
				<< child->is_terminal << endl;

		node = child;
		if (!node->is_leaf())
			a = node->best_branch;
	}
}

void SitePercolation::update_tree() {
	expand_tree(p_root);

}

int SitePercolation::calc_fn(const ALERAM& machine_state,
		reward_t accumulated_reward) {
	return random();
}

int SitePercolation::expand_node(TreeNode* curr_node) {
	int num_simulated_steps = 0;
	int num_actions = available_actions.size();
	bool leaf_node = (curr_node->v_children.empty());
	static int max_nodes_per_frame = max_sim_steps_per_frame
			/ sim_steps_per_node;
	m_expanded_nodes++;
// Expand all of its children (simulates the result)
	if (leaf_node) {
		curr_node->v_children.resize(num_actions);
		curr_node->available_actions = available_actions;
		if (m_randomize_successor)
			std::random_shuffle(curr_node->available_actions.begin(),
					curr_node->available_actions.end());

	}

	vector<bool> isUsefulAction(PLAYER_A_MAX, true);
	if (action_sequence_detection) {
		if (!trajectory.empty()) {
			vector<Action> p = getPreviousActions(curr_node,
					dasd_sequence_length - 1);
//			if (longest_junk_sequence - 1 > 0) {
			isUsefulAction = dasd->getEffectiveActions(p);
//			}
		}
	}

	for (int a = 0; a < num_actions; a++) {
		Action act = curr_node->available_actions[a];

		TreeNode * child;

		// If re-expanding an internal node, don't creates new nodes
		if (leaf_node) {
			if (action_sequence_detection) {
				if (curr_node != p_root) {
					if (!isUsefulAction[act]) {
						m_jasd_pruned_nodes++;
						TreeNode * child = new TreeNode(curr_node,
								curr_node->state, this, act, 0);
						curr_node->v_children[a] = child;
						child->is_terminal = true;
						continue;
					}
				}
			}

			m_generated_nodes++;
			child = new TreeNode(curr_node, curr_node->state, this, act,
					sim_steps_per_node);
			child->fn = calc_fn(child->state.getRAM(),
					child->accumulated_reward);
//			printf("child generated %d\n", child->fn);
			curr_node->v_children[a] = child;
			child->is_terminal = false;
//			m_pruned_nodes++;

			if (child->depth() > m_max_depth)
				m_max_depth = child->depth();
			num_simulated_steps += child->num_simulated_steps;

		} else {
			m_reused_nodes++;

			child = curr_node->v_children[a];

			// TODO: Nodes which do not require emulator can be explored more freely
			// because they do not require any additional resources to expand.

			// This recreates the novelty table (which gets resetted every time
			// we change the root of the search tree)
			if (child->is_terminal) {

				// Expanding emulated nodes require minimal search effort.
				// May worth expanding without strict pruning.
				if (m_expand_all_emulated_nodes) {
					child->is_terminal = false;
				} else {
					child->is_terminal = true;
				}
				m_pruned_nodes++;

			}
			child->updateTreeNode();

			if (child->depth() > m_max_depth)
				m_max_depth = child->depth();

			// DONT COUNT REUSED NODES
			//if ( !child->is_terminal )
			//	num_simulated_steps += child->num_simulated_steps;

		}

		// Don't expand duplicate nodes, or terminal nodes
		if (!child->is_terminal) {
			if (!(ignore_duplicates && test_duplicate(child))) {
				m_q_percolation.push(child);
			}
		}

	}
//	printf("reused= %d\n", m_reused_nodes);
	return num_simulated_steps;
}

/* *********************************************************************
 Expands the tree from the given node until i_max_sim_steps_per_frame
 is reached

 ******************************************************************* */
void SitePercolation::expand_tree(TreeNode* start_node) {

	if (!start_node->v_children.empty()) {
		start_node->updateTreeNode();
		for (int a = 0; a < available_actions.size(); a++) {
			TreeNode* child = start_node->v_children[a];
			if (!child->is_terminal) {
				child->num_nodes_reusable = child->num_nodes();
			}
		}
	}

	std::list<TreeNode*> pivots;

//q.push(start_node);
	pivots.push_back(start_node);

	int num_simulated_steps = 0;

	m_expanded_nodes = 0;
	m_generated_nodes = 0;

	m_reused_nodes = 0;

	m_pruned_nodes = 0;
	m_jasd_pruned_nodes = 0;

	do {

		std::cout << "# Pivots: " << pivots.size() << std::endl;
		std::cout << "First pivot reward: " << pivots.front()->node_reward
				<< std::endl;
		pivots.front()->m_depth = 0;
		int steps = expand_node(pivots.front());
		num_simulated_steps += steps;

		if (num_simulated_steps >= max_sim_steps_per_frame) {
			break;
		}

		pivots.pop_front();

		while (!m_q_percolation.empty()) {
			// Pop a node to expand
			TreeNode* curr_node = m_q_percolation.top();
			m_q_percolation.pop();

			if (curr_node->depth() > m_reward_horizon - 1)
				continue;
			if (m_stop_on_first_reward && curr_node->node_reward != 0) {
				pivots.push_back(curr_node);
				continue;
			}
			steps = expand_node(curr_node);
			num_simulated_steps += steps;
			// Stop once we have simulated a maximum number of steps
			if (num_simulated_steps >= max_sim_steps_per_frame) {
				break;
			}

		}
		std::cout << "\tExpanded so far: " << m_expanded_nodes << std::endl;
		std::cout << "\tPruned so far: " << m_pruned_nodes << std::endl;
		std::cout << "\tGenerated so far: " << m_generated_nodes << std::endl;

		if (m_q_percolation.empty())
			std::cout << "Search Space Exhausted!" << std::endl;

		// Stop once we have simulated a maximum number of steps
		if (num_simulated_steps >= max_sim_steps_per_frame) {
			break;
		}

	} while (!pivots.empty());

	update_branch_return(start_node);
}

void SitePercolation::clear() {
	SearchTree::clear();
	std::priority_queue<TreeNode*, std::vector<TreeNode*>, SiteIPPriority> emptyn;
	std::swap(m_q_percolation, emptyn);

}

void SitePercolation::move_to_best_sub_branch() {
	SearchTree::move_to_best_sub_branch();
	std::priority_queue<TreeNode*, std::vector<TreeNode*>, SiteIPPriority> emptyn;
	std::swap(m_q_percolation, emptyn);

}

/* *********************************************************************
 Updates the branch reward for the given node
 which equals to: node_reward + max(children.branch_return)
 ******************************************************************* */
void SitePercolation::update_branch_return(TreeNode* node) {
// Base case (leaf node): the return is the immediate reward
	if (node->v_children.empty()) {
		node->branch_return = node->node_reward;
		node->best_branch = -1;
		node->branch_depth = node->m_depth;
		return;
	}

// First, we have to make sure that all the children are updated
	for (unsigned int c = 0; c < node->v_children.size(); c++) {
		TreeNode* curr_child = node->v_children[c];

		if (ignore_duplicates && curr_child->is_duplicate())
			continue;

		update_branch_return(curr_child);
	}

// Now that all the children are updated, we can update the branch-reward
	float best_return = -1;
	int best_branch = -1;
	return_t avg = 0;

// Terminal nodes encur no reward beyond immediate
	if (node->is_terminal) {
		node->branch_depth = node->m_depth;
		best_return = node->node_reward;
		best_branch = 0;
	} else {

		for (size_t a = 0; a < node->v_children.size(); a++) {
			return_t child_return = node->v_children[a]->branch_return;
			if (best_branch == -1 || child_return > best_return) {
				best_return = child_return;
				best_branch = a;
				avg += child_return;
			}
			if (node->v_children[a]->branch_depth > node->branch_depth)
				node->branch_depth = node->v_children[a]->branch_depth;

			if (node->v_children.size())
				avg /= node->v_children.size();
		}
	}

//node->branch_return = node->node_reward + avg * discount_factor;
	// Normalize
//	if (!depth_normalized_reward) {
	node->branch_return = node->node_reward + best_return * discount_factor;
//	} else {
//		int node_height = node->branch_depth - node->m_depth;
//		double norm_times = (1.0 - pow(discount_factor, node_height))
//				/ (1.0 - discount_factor);
//		double norm_divides = (1.0 - pow(discount_factor, node_height + 1))
//				/ (1.0 - discount_factor);
//
//		node->branch_return = ((double) node->node_reward
//				+ norm_times * (double) discount_factor * (double) best_return)
//				/ norm_divides;
//	}

	node->best_branch = best_branch;
}

void SitePercolation::set_terminal_root(TreeNode * node) {
	node->branch_return = node->node_reward;

	if (node->v_children.empty()) {
		// Expand one child; add it to the node's children
		TreeNode* new_child = new TreeNode(node, node->state, this,
				PLAYER_A_NOOP, sim_steps_per_node);

		node->v_children.push_back(new_child);
	}

// Now we have at least one child, set the 'best branch' to the first action
	node->best_branch = 0;
}

void SitePercolation::print_frame_data(int frame_number, float elapsed,
		Action curr_action, std::ostream& output) {
	output << "frame=" << frame_number;
	output << ",expanded=" << expanded_nodes();
	output << ",generated=" << generated_nodes();
	output << ",pruned=" << pruned();
	output << ",jasd_pruned=" << jasd_pruned();
	output << ",depth_tree=" << max_depth();
	output << ",tree_size=" << num_nodes();
	output << ",best_action=" << action_to_string(curr_action);
	output << ",branch_reward=" << get_root_value();
	output << ",elapsed=" << elapsed;
	output << ",total_simulation_steps=" << total_simulation_steps;
	output << ",emulation_time=" << m_emulation_time;
	m_rom_settings->print(output);
	output << std::endl;
}
