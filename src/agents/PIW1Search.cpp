#include "PIW1Search.hpp"
#include "SearchAgent.hpp"
#include <list>

PIW1Search::PIW1Search(RomSettings *rom_settings, Settings &settings,
		ActionVect &actions, StellaEnvironment* _env) :
		SearchTree(rom_settings, settings, actions, _env) {

	m_stop_on_first_reward = settings.getBool("iw1_stop_on_first_reward", true);

	int val = settings.getInt("iw1_reward_horizon", -1);

	m_novelty_boolean_representation = settings.getBool("novelty_boolean",
			false);

	m_reward_horizon = (val < 0 ? std::numeric_limits<unsigned>::max() : val);

	int minus_inf = numeric_limits<int>::min();
//	printf("minus_inf=%d\n", minus_inf);
	m_ram_reward_table_true.resize(8 * RAM_SIZE);
	m_ram_reward_table_false.resize(8 * RAM_SIZE);
//	for (unsigned int i = 0; i < 8 * RAM_SIZE; ++i) {
//		m_ram_reward_table_true[i] = minus_inf;
//		m_ram_reward_table_false[i] = minus_inf;
//	}
	m_ram_reward_table_true.assign(8 * RAM_SIZE, minus_inf);
	m_ram_reward_table_false.assign(8 * RAM_SIZE, minus_inf);

	// TODO: parameterize
//	ignore_duplicates = true;

	m_expand_all_emulated_nodes = settings.getBool("expand_all_emulated_nodes", false);

	if (m_expand_all_emulated_nodes) {
		printf("Expands all previously emulated nodes\n");
	}
}

PIW1Search::~PIW1Search() {
//	if (m_novelty_boolean_representation) {
//		delete m_ram_novelty_table_true;
//		delete m_ram_novelty_table_false;
//	} else
//		delete m_ram_novelty_table;

	m_ram_reward_table_true.clear();
	m_ram_reward_table_false.clear();
}

/* *********************************************************************
 Builds a new tree
 ******************************************************************* */
void PIW1Search::build(ALEState & state) {
	assert(p_root == NULL);
	p_root = new TreeNode(NULL, state, NULL, UNDEFINED, 0);
	update_tree();
	is_built = true;
}

void PIW1Search::print_path(TreeNode * node, int a) {
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

// void PIW1Search::initialize_tree(TreeNode* node){
// 	do {
// 		Action act =  Action::PLAYER_A_NOOP
// 		m_generated_nodes++;

// 		TreeNode* child = new TreeNode(	curr_node,	
// 					curr_node->state,
// 					this,
// 					act,
// 					sim_steps_per_node); 

// 		if ( check_novelty_1( child->state.getRAM() ) ) {
// 			update_novelty_table( child->state.getRAM() );

// 		}
// 		else{
// 			curr_node->v_children[a] = child;
// 			child->is_terminal = true;
// 			m_pruned_nodes++;
// 			break;
// 		}

// 		num_simulated_steps += child->num_simulated_steps;					
// 		node->v_children[a] = child;

// 		node = child;
// 	}while( node->depth() < m_max_depth)
// }

void PIW1Search::update_tree() {
	expand_tree(p_root);
	// for(unsigned byte = 0; byte < RAM_SIZE; byte++){
	//     std::cout << "Byte: " << byte << std::endl;
	//     int count = 0;
	//     for( unsigned i = 0; i < 255; i++){
	// 	if ( m_ram_novelty_table->isset( byte,i ) ){
	// 	    count++;			
	// 	    std::cout << "\t element: "<< i << std::endl;
	// 	}

	//     }
	//     std::cout << "\t num_elements " << count << std::endl;
	// }
	// std::exit(0);
}

void PIW1Search::update_novelty_table(const ALERAM& machine_state,
		reward_t accumulated_reward) {
//	for (size_t i = 0; i < machine_state.size(); i++) {
//		if (m_novelty_boolean_representation) {
//			unsigned char mask = 1;
//			byte_t byte = machine_state.get(i);
//			for (int j = 0; j < 8; j++) {
//				bool bit_is_set = (byte & (mask << j)) != 0;
//				if (bit_is_set)
//					m_ram_novelty_table_true->set(i, j);
//				else
//					m_ram_novelty_table_false->set(i, j);
//			}
//		} else {
//			m_ram_novelty_table->set(i, machine_state.get(i));
//		}
//	}
	bool updated = false;

	for (unsigned int i = 0; i < machine_state.size(); i++) {
		unsigned char mask = 1;
		byte_t byte = machine_state.get(i);
		for (int j = 0; j < 8; j++) {
			bool bit_is_set = (byte & (mask << j)) != 0;
			if (bit_is_set) {
				int old_reward = m_ram_reward_table_true[8 * i + j];
				if (accumulated_reward > old_reward) {
					m_ram_reward_table_true[8 * i + j] = accumulated_reward;
					updated = true;
				}
			} else {
				int old_reward = m_ram_reward_table_false[8 * i + j];
				if (accumulated_reward > old_reward) {
					m_ram_reward_table_false[8 * i + j] = accumulated_reward;
					updated = true;
				}
			}
		}
	}
//	if (updated) {
//		printf("Updated rewards\n");
//	}
}

bool PIW1Search::check_novelty_1(const ALERAM& machine_state,
		reward_t accumulated_reward) {
//	for (size_t i = 0; i < machine_state.size(); i++)
//		if (m_novelty_boolean_representation) {
//			unsigned char mask = 1;
//			byte_t byte = machine_state.get(i);
//			for (int j = 0; j < 8; j++) {
//				bool bit_is_set = (byte & (mask << j)) != 0;
//				if (bit_is_set) {
//					if (!m_ram_novelty_table_true->isset(i, j))
//						return true;
//				} else {
//					if (!m_ram_novelty_table_false->isset(i, j))
//						return true;
//
//				}
//			}
//		} else if (!m_ram_novelty_table->isset(i, machine_state.get(i)))
//			return true;
//	return false;

	for (unsigned int i = 0; i < machine_state.size(); i++) {
		unsigned char mask = 1;
		byte_t byte = machine_state.get(i);
		for (int j = 0; j < 8; j++) {
			bool bit_is_set = (byte & (mask << j)) != 0;
			if (bit_is_set) {
//				if (m_ram_reward_table_true[8 * i + j]
//						> numeric_limits<int>::min()) {
//					return true;
//				}

				if (accumulated_reward > m_ram_reward_table_true[8 * i + j]) {
					printf("update: curr_reward= %d table_reward= %d\n",
							accumulated_reward,
							m_ram_reward_table_true[8 * i + j]);
					return true;
				}
			} else {
//				if (m_ram_reward_table_false[8 * i + j]
//						> numeric_limits<int>::min()) {
//					return true;
//				}

				if (accumulated_reward > m_ram_reward_table_false[8 * i + j]) {
					printf("update: curr_reward= %d table_reward= %d\n",
							accumulated_reward,
							m_ram_reward_table_false[8 * i + j]);
					return true;
				}
			}
		}
	}
	return false;
}

int PIW1Search::check_novelty(const ALERAM& machine_state,
		reward_t accumulated_reward) {
	int novelty = 0;

	for (unsigned int i = 0; i < machine_state.size(); i++) {
		unsigned char mask = 1;
		byte_t byte = machine_state.get(i);
		for (int j = 0; j < 8; j++) {
			bool bit_is_set = (byte & (mask << j)) != 0;
			if (bit_is_set) {
				if (accumulated_reward > m_ram_reward_table_true[8 * i + j]) {
					++novelty;
				}
			} else {
				if (accumulated_reward > m_ram_reward_table_false[8 * i + j]) {
					++novelty;
				}
			}
		}
	}
	return novelty;
}

int PIW1Search::calc_fn(const ALERAM& machine_state,
		reward_t accumulated_reward) {
	int n_novelty = check_novelty(machine_state, accumulated_reward);

	double k = 1.0;
	return n_novelty + k * (double) accumulated_reward;
}

int PIW1Search::expand_node(TreeNode* curr_node,
		std::priority_queue<TreeNode*, std::vector<TreeNode*>,
				TreeNodeComparerReward>& q) {
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
	for (int a = 0; a < num_actions; a++) {
		Action act = curr_node->available_actions[a];

		TreeNode * child;

		// If re-expanding an internal node, don't creates new nodes
		if (leaf_node) {
			m_generated_nodes++;
			child = new TreeNode(curr_node, curr_node->state, this, act,
					sim_steps_per_node);

			// Pruning is executed when the node is generated.
			if (check_novelty_1(child->state.getRAM(),
					child->accumulated_reward)) {
//				printf("found novel state\n");
				update_novelty_table(child->state.getRAM(),
						child->accumulated_reward);

			} else {
//				printf("pruned state\n");
				curr_node->v_children[a] = child;
				child->is_terminal = true;
				m_pruned_nodes++;
				//continue;
			}
			if (child->depth() > m_max_depth)
				m_max_depth = child->depth();
			num_simulated_steps += child->num_simulated_steps;

			curr_node->v_children[a] = child;
		} else {
			m_reused_nodes++;

			child = curr_node->v_children[a];

			// TODO: Nodes which do not require emulator can be explored more freely
			// because they do not require any additional resources to expand.

			// This recreates the novelty table (which gets resetted every time
			// we change the root of the search tree)
			if (m_novelty_pruning) {
				if (child->is_terminal) {
					if (check_novelty_1(child->state.getRAM(),
							child->accumulated_reward)) {
						update_novelty_table(child->state.getRAM(),
								child->accumulated_reward);

						child->is_terminal = false;
					} else {
						// Expanding emulated nodes require minimal search effort.
						// May worth expanding without strict pruning.
						if (m_expand_all_emulated_nodes) {
							child->is_terminal = false;
						} else {
							child->is_terminal = true;
						}
						m_pruned_nodes++;
					}
				}
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
			if (!(ignore_duplicates && test_duplicate_reward(child)))
				if (child->num_nodes_reusable < max_nodes_per_frame) {
					q.push(child);
//					printf("node gend\n");
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
void PIW1Search::expand_tree(TreeNode* start_node) {

	if (!start_node->v_children.empty()) {
		start_node->updateTreeNode();
		for (int a = 0; a < available_actions.size(); a++) {
			TreeNode* child = start_node->v_children[a];
			if (!child->is_terminal) {
				child->num_nodes_reusable = child->num_nodes();
			}
		}
	}

//	queue<TreeNode*> q;
	std::priority_queue<TreeNode*, std::vector<TreeNode*>,
			TreeNodeComparerReward> q;
//	q = new priority_queue<TreeNode*, std::vector<TreeNode*>,
//			TreeNodeComparerReward>();
	std::list<TreeNode*> pivots;

//q.push(start_node);
	pivots.push_back(start_node);

	update_novelty_table(start_node->state.getRAM(), 0);

	int num_simulated_steps = 0;

	m_expanded_nodes = 0;
	m_generated_nodes = 0;

	m_reused_nodes = 0;

	m_pruned_nodes = 0;

	do {

		std::cout << "# Pivots: " << pivots.size() << std::endl;
		std::cout << "First pivot reward: " << pivots.front()->node_reward
				<< std::endl;
		pivots.front()->m_depth = 0;
		int steps = expand_node(pivots.front(), q);
		num_simulated_steps += steps;

		if (num_simulated_steps >= max_sim_steps_per_frame) {
			break;
		}

		pivots.pop_front();

		while (!q.empty()) {
			// Pop a node to expand
			TreeNode* curr_node = q.top();
			q.pop();

			if (curr_node->depth() > m_reward_horizon - 1)
				continue;
			if (m_stop_on_first_reward && curr_node->node_reward != 0) {
				pivots.push_back(curr_node);
				continue;
			}
			steps = expand_node(curr_node, q);
			num_simulated_steps += steps;
			// Stop once we have simulated a maximum number of steps
			if (num_simulated_steps >= max_sim_steps_per_frame) {
				break;
			}

		}
		std::cout << "\tExpanded so far: " << m_expanded_nodes << std::endl;
		std::cout << "\tPruned so far: " << m_pruned_nodes << std::endl;
		std::cout << "\tGenerated so far: " << m_generated_nodes << std::endl;

		if (q.empty())
			std::cout << "Search Space Exhausted!" << std::endl;
		// Stop once we have simulated a maximum number of steps
		if (num_simulated_steps >= max_sim_steps_per_frame) {
			break;
		}

	} while (!pivots.empty());

	update_branch_return(start_node);
}

void PIW1Search::clear() {
	SearchTree::clear();
	int minus_inf = numeric_limits<int>::min();
	m_ram_reward_table_true.assign(8 * RAM_SIZE, minus_inf);
	m_ram_reward_table_false.assign(8 * RAM_SIZE, minus_inf);

//	if (m_novelty_boolean_representation) {
//		m_ram_novelty_table_true->clear();
//		m_ram_novelty_table_false->clear();
//	} else
//		m_ram_novelty_table->clear();
}

void PIW1Search::move_to_best_sub_branch() {
	SearchTree::move_to_best_sub_branch();
	int minus_inf = numeric_limits<int>::min();
	m_ram_reward_table_true.assign(8 * RAM_SIZE, minus_inf);
	m_ram_reward_table_false.assign(8 * RAM_SIZE, minus_inf);

//	if (m_novelty_boolean_representation) {
//		m_ram_novelty_table_true->clear();
//		m_ram_novelty_table_false->clear();
//	} else
//		m_ram_novelty_table->clear();

}

/* *********************************************************************
 Updates the branch reward for the given node
 which equals to: node_reward + max(children.branch_return)
 ******************************************************************* */
void PIW1Search::update_branch_return(TreeNode* node) {
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

	node->branch_return = node->node_reward + best_return * discount_factor;
//node->branch_return = node->node_reward + avg * discount_factor;

	node->best_branch = best_branch;
}

void PIW1Search::set_terminal_root(TreeNode * node) {
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

bool PIW1Search::test_duplicate_reward(TreeNode * node) {
	if (node->p_parent == NULL)
		return false;
	else {
		TreeNode * parent = node->p_parent;

		// TODO: Inefficient duplicate detection: Currently implemented by Full mesh.
		// Compare each valid sibling with this one
		for (size_t c = 0; c < parent->v_children.size(); c++) {
			TreeNode * sibling = parent->v_children[c];
			// Ignore duplicates, this node and uninitialized nodes
			if (sibling->is_duplicate() || sibling == node
					|| !sibling->is_initialized())
				continue;

			if (sibling->state.equals(node->state)
					&& sibling->accumulated_reward > node->accumulated_reward) {

				node->duplicate = true;
				return true;
			}
		}

		// None of the siblings match, unique node
		node->duplicate = false;
		return false;
	}
}

void PIW1Search::print_frame_data(int frame_number, float elapsed,
		Action curr_action, std::ostream& output) {
	output << "frame=" << frame_number;
	output << ",expanded=" << expanded_nodes();
	output << ",generated=" << generated_nodes();
	output << ",pruned=" << pruned();
	output << ",depth_tree=" << max_depth();
	output << ",tree_size=" << num_nodes();
	output << ",best_action=" << action_to_string(curr_action);
	output << ",branch_reward=" << get_root_value();
	output << ",elapsed=" << elapsed;
	m_rom_settings->print(output);
	output << std::endl;
}
