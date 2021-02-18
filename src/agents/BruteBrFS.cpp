#include "BruteBrFS.hpp"

#include "SearchAgent.hpp"
#include <queue>
#include <>

#include "DominatedActionSequenceDetection.hpp"

/* *********************************************************************
 Constructor
 Generates a whole search tree from the current state, until
 max_frame_num is reached.
 ******************************************************************* */
BruteBrFS::BruteBrFS(RomSettings *rom_settings, Settings &settings, ActionVect &actions,
		StellaEnvironment* _env) :
		SearchTree(rom_settings, settings, actions, _env) {
}

/* *********************************************************************
 Destructor
 ******************************************************************* */
BruteBrFS::~BruteBrFS() {
}

/* *********************************************************************
 Builds a new tree
 ******************************************************************* */
void BruteBrFS::build(ALEState & state) {
	assert(p_root == NULL);
	p_root = new TreeNode(NULL, state, NULL, UNDEFINED, 0);
	update_tree();
	is_built = true;
}

void BruteBrFS::print_path(TreeNode * node, int a) {
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

void BruteBrFS::update_tree() {
	expand_tree(p_root);
}

/* *********************************************************************
 Expands the tree from the given node until i_max_sim_steps_per_frame
 is reached

 ******************************************************************* */
void BruteBrFS::expand_tree(TreeNode* start_node) {
	// If the root is terminal, we will not expand any of its children; deal with this
	//  appropriately
	assert(start_node != nullptr && "start_node is null");
	if (start_node->is_terminal) {
		set_terminal_root(start_node);
		return;
	}

	priority_queue<BruteTreeNode*, vector<BruteTreeNode*>, BruteComparator> q;

//	queue<TreeNode*> q;
	q.push((BruteTreeNode*) start_node);

	int num_simulated_steps = 0;
	int num_actions = available_actions.size();

	m_expanded_nodes = 0;
	m_generated_nodes = 0;

	m_pruned_nodes = 0;
	m_jasd_pruned_nodes = 0;

	while (!q.empty()) {
		// Pop a node to expand
		TreeNode* curr_node = selectNode(q);

		bool leaf_node = (curr_node->v_children.empty());
		m_expanded_nodes++;
		// Expand all of its children (simulates the result)

		if (m_randomize_successor)
			std::random_shuffle(available_actions.begin(),
					available_actions.end());
		if (leaf_node) {
			curr_node->v_children.resize(num_actions);
			curr_node->available_actions = available_actions;
		}

		vector<bool> isUsefulAction(PLAYER_A_MAX, true);
		if (action_sequence_detection) {
			if (!trajectory.empty()) {
				vector<Action> p = getPreviousActions(curr_node,
						dasd_sequence_length - 1);
				isUsefulAction = dasd->getEffectiveActions(p,
						trajectory.size() * sim_steps_per_node);

			}
		}

		for (int a = 0; a < num_actions; a++) {
			Action act = available_actions[a];

			TreeNode * child;

			// If re-expanding an internal node, don't creates new nodes
			if (leaf_node) {

				if (action_sequence_detection) {
					if (curr_node != p_root) {
						if (!isUsefulAction[act]) {
							m_jasd_pruned_nodes++;
							//					printf("Pruned %d\n", (int) a,
							//							action_to_string((Action) a).c_str());
							// TODO: generate dummy node
							TreeNode * child;
							child = new TreeNode(curr_node, curr_node->state,
									this, act, 0);
							curr_node->v_children[a] = child;
							child->is_terminal = true;
							continue;
						}
					}
				}

				m_generated_nodes++;
				child = new TreeNode(curr_node, curr_node->state, this, act,
						sim_steps_per_node);

				// TODO: BreadthFirstSearch needs to be split into two classes
				// the new one encapsulating the novelty-based search algorithm
				if (child->depth() > m_max_depth)
					m_max_depth = child->depth();
				num_simulated_steps += child->num_simulated_steps;

				curr_node->v_children[a] = child;
			} else {
				child = curr_node->v_children[a];
				if (!child->is_terminal)
					num_simulated_steps += child->num_simulated_steps;

			}

			// Don't expand duplicate nodes, or terminal nodes
			if (!child->is_terminal) {
				if (!(ignore_duplicates && test_duplicate(child))) {
					q.push((BruteTreeNode*) child);
				} else {
					m_pruned_nodes++;
				}
			}
		}

		// Stop once we have simulated a maximum number of steps
		if (num_simulated_steps >= max_sim_steps_per_frame) {
			break;
		}

	}

	if (q.empty())
		std::cout << "Search Space Exhausted!" << std::endl;

	update_branch_return(start_node);
}

TreeNode* BruteBrFS::selectNode(
		priority_queue<BruteTreeNode*, vector<BruteTreeNode*>,
				BruteComparator>& queue) {

}

void BruteBrFS::clear() {
	SearchTree::clear();
}

void BruteBrFS::move_to_best_sub_branch() {
	SearchTree::move_to_best_sub_branch();
}

void BruteBrFS::move_to_branch(Action a, int duration) {
	SearchTree::move_to_branch(a, duration);
}

/* *********************************************************************
 Updates the branch reward for the given node
 which equals to: node_reward + max(children.branch_return)
 ******************************************************************* */
void BruteBrFS::update_branch_return(TreeNode* node) {
	// Base case (leaf node): the return is the immediate reward
	if (node->v_children.empty()) {
		node->branch_return = node->node_reward;
		node->best_branch = -1;
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
			if (node->v_children.size())
				avg /= node->v_children.size();
		}
	}

	node->branch_return = node->node_reward + best_return * discount_factor;
	//node->branch_return = node->node_reward + avg * discount_factor; 

	node->best_branch = best_branch;
}

void BruteBrFS::set_terminal_root(TreeNode * node) {
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

void BruteBrFS::print_frame_data(int frame_number, float elapsed,
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
	output << ",total_simulation_steps=" << m_total_simulation_steps;
	output << ",emulation_time=" << m_emulation_time;
	output << ",context_switching_time=" << m_context_time;
	output << std::endl;
}