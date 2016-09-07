#include "IW1Search.hpp"
#include "SearchAgent.hpp"
#include <list>
#include "ActionSequenceDetection.hpp"

IW1Search::IW1Search(RomSettings *rom_settings, Settings &settings,
		ActionVect &actions, StellaEnvironment* _env) :
		SearchTree(rom_settings, settings, actions, _env) {

	m_stop_on_first_reward = settings.getBool("iw1_stop_on_first_reward", true);

	int val = settings.getInt("iw1_reward_horizon", -1);

	m_novelty_boolean_representation = settings.getBool("novelty_boolean",
			false);

	m_reward_horizon = (val < 0 ? std::numeric_limits<unsigned>::max() : val);

	if (m_novelty_boolean_representation) {
		m_ram_novelty_table_true = new aptk::Bit_Matrix(RAM_SIZE, 8);
		m_ram_novelty_table_false = new aptk::Bit_Matrix(RAM_SIZE, 8);
	} else {
		m_ram_novelty_table = new aptk::Bit_Matrix(RAM_SIZE, 256);
	}

	m_pruned_nodes = 0;
}

IW1Search::~IW1Search() {
	if (m_novelty_boolean_representation) {
		delete m_ram_novelty_table_true;
		delete m_ram_novelty_table_false;
	} else
		delete m_ram_novelty_table;
}

/* *********************************************************************
 Builds a new tree
 ******************************************************************* */
void IW1Search::build(ALEState & state) {
	assert(p_root == NULL);
	p_root = new TreeNode(NULL, state, NULL, UNDEFINED, 0);
	update_tree();
	is_built = true;
}

void IW1Search::print_path(TreeNode * node, int a) {
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

// void IW1Search::initialize_tree(TreeNode* node){
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

void IW1Search::update_tree() {
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

void IW1Search::update_novelty_table(const ALERAM& machine_state) {
	for (size_t i = 0; i < machine_state.size(); i++)
		if (m_novelty_boolean_representation) {
			unsigned char mask = 1;
			byte_t byte = machine_state.get(i);
			for (int j = 0; j < 8; j++) {
				bool bit_is_set = (byte & (mask << j)) != 0;
				if (bit_is_set)
					m_ram_novelty_table_true->set(i, j);
				else
					m_ram_novelty_table_false->set(i, j);
			}
		} else
			m_ram_novelty_table->set(i, machine_state.get(i));
}

bool IW1Search::check_novelty_1(const ALERAM& machine_state) {
	for (size_t i = 0; i < machine_state.size(); i++)
		if (m_novelty_boolean_representation) {
			unsigned char mask = 1;
			byte_t byte = machine_state.get(i);
			for (int j = 0; j < 8; j++) {
				bool bit_is_set = (byte & (mask << j)) != 0;
				if (bit_is_set) {
					if (!m_ram_novelty_table_true->isset(i, j))
						return true;
				} else {
					if (!m_ram_novelty_table_false->isset(i, j))
						return true;

				}
			}
		} else if (!m_ram_novelty_table->isset(i, machine_state.get(i)))
			return true;
	return false;
}

int IW1Search::expand_node(TreeNode* curr_node, queue<TreeNode*>& q) {
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
					longest_junk_sequence - 1);
//			if (longest_junk_sequence > 0) {
//				p.push_back(curr_node->act);
//				p.assign(trajectory.end() - 1, trajectory.end());
			isUsefulAction = asd->getUsefulActions(p);
//				for (int i = 0; i < PLAYER_A_MAX; ++i) {
//					if (isUsefulAction[i]) {
//						printf("o");
//					} else {
//						printf("x");
//					}
//				}
//				printf("\n");
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
						//					printf("Pruned %d\n", (int) a,
						//							action_to_string((Action) a).c_str());
						// TODO: generate dummy node
						TreeNode * child;
						child = new TreeNode(curr_node, curr_node->state, this,
								act, 0);
						curr_node->v_children[a] = child;
						child->is_terminal = true;
						continue;
					}
				}
			}

			m_generated_nodes++;
			child = new TreeNode(curr_node, curr_node->state, this, act,
					sim_steps_per_node);

			if (check_novelty_1(child->state.getRAM())) {
				update_novelty_table(child->state.getRAM());

			} else {
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
			child = curr_node->v_children[a];

			// This recreates the novelty table (which gets resetted every time
			// we change the root of the search tree)
			if (m_novelty_pruning) {
				if (child->is_terminal) {
					if (check_novelty_1(child->state.getRAM())) {
						update_novelty_table(child->state.getRAM());
						child->is_terminal = false;
					} else {
						child->is_terminal = true;
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
			if (!(ignore_duplicates && test_duplicate(child)))
				if (child->num_nodes_reusable < max_nodes_per_frame)
					q.push(child);
		}
	}
	return num_simulated_steps;
}

/* *********************************************************************
 Expands the tree from the given node until i_max_sim_steps_per_frame
 is reached

 ******************************************************************* */
void IW1Search::expand_tree(TreeNode* start_node) {

	if (!start_node->v_children.empty()) {
		start_node->updateTreeNode();
		for (int a = 0; a < available_actions.size(); a++) {
			TreeNode* child = start_node->v_children[a];
			if (!child->is_terminal) {
				child->num_nodes_reusable = child->num_nodes();
			}
		}
	}

	queue<TreeNode*> q;
	std::list<TreeNode*> pivots;

	//q.push(start_node);
	pivots.push_back(start_node);

	update_novelty_table(start_node->state.getRAM());
	int num_simulated_steps = 0;

	m_expanded_nodes = 0;
	m_generated_nodes = 0;

	m_pruned_nodes = 0;
	m_jasd_pruned_nodes = 0;

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
			TreeNode* curr_node = q.front();
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
//		std::cout << "\tSimulated steps: " << num_simulated_steps

		if (q.empty())
			std::cout << "Search Space Exhausted!" << std::endl;
		// Stop once we have simulated a maximum number of steps
		if (num_simulated_steps >= max_sim_steps_per_frame) {
			break;
		}

	} while (!pivots.empty());

	update_branch_return(start_node);
}

void IW1Search::clear() {
	SearchTree::clear();

	if (m_novelty_boolean_representation) {
		m_ram_novelty_table_true->clear();
		m_ram_novelty_table_false->clear();
	} else
		m_ram_novelty_table->clear();
}

void IW1Search::move_to_best_sub_branch() {
	SearchTree::move_to_best_sub_branch();
	if (m_novelty_boolean_representation) {
		m_ram_novelty_table_true->clear();
		m_ram_novelty_table_false->clear();
	} else
		m_ram_novelty_table->clear();

}

/* *********************************************************************
 Updates the branch reward for the given node
 which equals to: node_reward + max(children.branch_return)
 ******************************************************************* */
void IW1Search::update_branch_return(TreeNode* node) {
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

	// Normalize
//	if (!depth_normalized_reward) {
//	node->branch_return = node->node_reward + best_return * discount_factor;
//	} else {
//		// a_n   := branch_return of node with depth n
//		// r_n   := node reward of node with depth n
//		// alpha := discount_factor
//
//		// We normalizes the reward with depth
//		// a_n = \frac{r_n + \alpha a_{n-1} t_{n-1}}{t_n}
//		// t_n = 1 + \alpha + \alpha^2 + ... + \alpha^n
//
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

	//node->branch_return = node->node_reward + avg * discount_factor;

	node->best_branch = best_branch;
}

void IW1Search::set_terminal_root(TreeNode * node) {
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

void IW1Search::print_frame_data(int frame_number, float elapsed,
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
	m_rom_settings->print(output);
	output << std::endl;
}
