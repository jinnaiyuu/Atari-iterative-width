#include "PIW1Search.hpp"
#include "SearchAgent.hpp"
#include <list>

#include "DominatedActionSequenceDetection.hpp"

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

	if (m_novelty_boolean_representation) {
		m_ram_reward_table_true.resize(8 * RAM_SIZE);
		m_ram_reward_table_false.resize(8 * RAM_SIZE);
		m_ram_reward_table_true.assign(8 * RAM_SIZE, minus_inf);
		m_ram_reward_table_false.assign(8 * RAM_SIZE, minus_inf);
	} else {
		m_ram_reward_table_byte.resize(256 * RAM_SIZE);
		m_ram_reward_table_byte.assign(256 * RAM_SIZE, minus_inf);

	}

//	for (unsigned int i = 0; i < 8 * RAM_SIZE; ++i) {
//		m_ram_reward_table_true[i] = minus_inf;
//		m_ram_reward_table_false[i] = minus_inf;
//	}

// TODO: parameterize
//	ignore_duplicates = true;

	m_expand_all_emulated_nodes = settings.getBool("expand_all_emulated_nodes",
			false);

	if (m_expand_all_emulated_nodes) {
		printf("Expands all previously emulated nodes\n");
	}

	m_priority_queue = settings.getString("priority_queue", false);
	if (m_priority_queue.empty() || m_priority_queue == "") {
		m_priority_queue = "reward";
	}
	if (m_priority_queue != "reward" && m_priority_queue != "novelty") {
		m_priority_queue = "reward";
	}
	printf("priority_queue %s\n", m_priority_queue.c_str());
}

PIW1Search::~PIW1Search() {
//	if (m_novelty_boolean_representation) {
//		delete m_ram_novelty_table_true;
//		delete m_ram_novelty_table_false;
//	} else
//		delete m_ram_novelty_table;

//	m_ram_reward_table_true.clear();
//	m_ram_reward_table_false.clear();
//	m_ram_reward_table_byte.clear();
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

	if (m_novelty_boolean_representation) {

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
						m_ram_reward_table_false[8 * i + j] =
								accumulated_reward;
						updated = true;
					}
				}
			}
		}

	} else {
		for (unsigned int i = 0; i < machine_state.size(); i++) {
			unsigned char mask = 1;
			byte_t byte = machine_state.get(i);

			int old_reward = m_ram_reward_table_byte[256 * i + byte];
			if (accumulated_reward > old_reward) {
				m_ram_reward_table_byte[256 * i + byte] = accumulated_reward;
				updated = true;
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

	if (m_novelty_boolean_representation) {
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

					if (accumulated_reward
							> m_ram_reward_table_true[8 * i + j]) {
//					printf("update: curr_reward= %d table_reward= %d\n",
//							accumulated_reward,
//							m_ram_reward_table_true[8 * i + j]);
						return true;
					}
				} else {
//				if (m_ram_reward_table_false[8 * i + j]
//						> numeric_limits<int>::min()) {
//					return true;
//				}

					if (accumulated_reward
							> m_ram_reward_table_false[8 * i + j]) {
//					printf("update: curr_reward= %d table_reward= %d\n",
//							accumulated_reward,
//							m_ram_reward_table_false[8 * i + j]);
						return true;
					}
				}
			}
		}
	} else {
		for (unsigned int i = 0; i < machine_state.size(); i++) {
			unsigned char mask = 1;
			byte_t byte = machine_state.get(i);
			if (accumulated_reward > m_ram_reward_table_byte[256 * i + byte]) {
				return true;
			}
		}
	}
	return false;

}

// TODO: This should be called BEFORE we update the reward table.
int PIW1Search::check_novelty(const ALERAM& machine_state,
		reward_t accumulated_reward) {
	int novelty = 0;

	if (m_novelty_boolean_representation) {
		for (unsigned int i = 0; i < machine_state.size(); i++) {
			unsigned char mask = 1;
			byte_t byte = machine_state.get(i);
			for (int j = 0; j < 8; j++) {
				bool bit_is_set = (byte & (mask << j)) != 0;
				if (bit_is_set) {
					if (accumulated_reward
							> m_ram_reward_table_true[8 * i + j]) {
						++novelty;
					}
				} else {
					if (accumulated_reward
							> m_ram_reward_table_false[8 * i + j]) {
						++novelty;
					}
				}
			}
		}
		return novelty;
	} else {
		for (unsigned int i = 0; i < machine_state.size(); i++) {
			unsigned char mask = 1;
			byte_t byte = machine_state.get(i);
			if (accumulated_reward > m_ram_reward_table_byte[256 * i + byte]) {
				++novelty;
			}
		}
	}
	return novelty;
}

int PIW1Search::calc_fn(const ALERAM& machine_state,
		reward_t accumulated_reward) {
	int n_novelty = check_novelty(machine_state, accumulated_reward);

	double k = 1.0;
	return n_novelty + (int) (k * (double) accumulated_reward);
}

int PIW1Search::expand_node(TreeNode* curr_node) {
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
						TreeNode * child = new TreeNode(curr_node, curr_node->state,
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

			// Pruning is executed when the node is generated.
			if (check_novelty_1(child->state.getRAM(),
					child->accumulated_reward)) {

//				child->additive_novelty = check_novelty(child->state.getRAM(),
//						child->accumulated_reward);

//				child->fn = calc_fn(child->state.getRAM(),
//						child->accumulated_reward); // TODO: duplicated calculation

//				printf("state: novelty= %d reward= %d fn=%d\n",
//						child->additive_novelty, child->accumulated_reward,
//						child->fn);

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

//						child->additive_novelty = check_novelty(
//								child->state.getRAM(),
//								child->accumulated_reward);
//
//						child->fn = calc_fn(child->state.getRAM(),
//								child->accumulated_reward); // TODO: duplicated calculation

//						printf("state: novelty= %d reward= %d fn=%d\n",
//								child->additive_novelty,
//								child->accumulated_reward, child->fn);

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
					if (m_priority_queue == "reward") {
						m_q_reward.push(child);
					} else if (m_priority_queue == "novelty") {
						m_q_novelty.push(child);
					} else {

					}
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
//	std::priority_queue<TreeNode*, std::vector<TreeNode*>,
//			TreeNodeComparerReward> q;

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

		// TODO: REFACTOR multiple priority queues.
		if (m_priority_queue == "reward") {
			while (!m_q_reward.empty()) {
				// Pop a node to expand
				TreeNode* curr_node = m_q_reward.top();
				m_q_reward.pop();

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
			std::cout << "\tGenerated so far: " << m_generated_nodes
					<< std::endl;

			if (m_q_reward.empty())
				std::cout << "Search Space Exhausted!" << std::endl;
		} else if (m_priority_queue == "novelty") {
			while (!m_q_novelty.empty()) {
				// Pop a node to expand
				TreeNode* curr_node = m_q_novelty.top();
				m_q_novelty.pop();

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
			std::cout << "\tGenerated so far: " << m_generated_nodes
					<< std::endl;

			if (m_q_novelty.empty())
				std::cout << "Search Space Exhausted!" << std::endl;
		} else {
			printf("undefined priority queue error: %s\n",
					m_priority_queue.c_str());
		}

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

	if (m_novelty_boolean_representation) {

		m_ram_reward_table_true.assign(8 * RAM_SIZE, minus_inf);
		m_ram_reward_table_false.assign(8 * RAM_SIZE, minus_inf);
	} else {
		m_ram_reward_table_byte.assign(256 * RAM_SIZE, minus_inf);
	}

	std::priority_queue<TreeNode*, std::vector<TreeNode*>,
			TreeNodeComparerReward> emptyr;
	std::swap(m_q_reward, emptyr);

	std::priority_queue<TreeNode*, std::vector<TreeNode*>,
			TreeNodeComparerAdditiveNovelty> emptyn;
	std::swap(m_q_novelty, emptyn);

	//	if (m_novelty_boolean_representation) {
//		m_ram_novelty_table_true->clear();
//		m_ram_novelty_table_false->clear();
//	} else
//		m_ram_novelty_table->clear();
}

void PIW1Search::move_to_best_sub_branch() {
	SearchTree::move_to_best_sub_branch();
	int minus_inf = numeric_limits<int>::min();
	if (m_novelty_boolean_representation) {

		m_ram_reward_table_true.assign(8 * RAM_SIZE, minus_inf);
		m_ram_reward_table_false.assign(8 * RAM_SIZE, minus_inf);
	} else {
		m_ram_reward_table_byte.assign(256 * RAM_SIZE, minus_inf);
	}

	std::priority_queue<TreeNode*, std::vector<TreeNode*>,
			TreeNodeComparerReward> emptyr;
	std::swap(m_q_reward, emptyr);

	std::priority_queue<TreeNode*, std::vector<TreeNode*>,
			TreeNodeComparerAdditiveNovelty> emptyn;
	std::swap(m_q_novelty, emptyn);

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
