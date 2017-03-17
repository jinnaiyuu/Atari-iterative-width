#include "BondPercolation.hpp"

#include "SearchAgent.hpp"
#include <list>

#include "DominatedActionSequenceDetection.hpp"

BondPercolation::BondPercolation(RomSettings *rom_settings, Settings &settings,
		ActionVect &actions, StellaEnvironment* _env) :
		SearchTree(rom_settings, settings, actions, _env) {

	m_randomize_successor = false;
// TODO: parameterize
//	ignore_duplicates = true;

	m_expand_all_emulated_nodes = settings.getBool("expand_all_emulated_nodes",
			false);

	if (m_expand_all_emulated_nodes) {
		printf("Expands all previously emulated nodes\n");
	}

	m_pruned_nodes = 0;

	string tiebreaking = settings.getString("tiebreaking", false);
	string delimiter = ",";

	size_t pos = 0;
	string token;
	while ((pos = tiebreaking.find(delimiter)) != string::npos) {
		token = tiebreaking.substr(0, pos);
		tiebreaking.erase(0, pos + delimiter.length());
		ties.push_back(token);
	}
	ties.push_back(tiebreaking);
	ListComparator comp;

	for (int i = 0; i < ties.size(); ++i) {
		printf("ties = %s\n", ties[i].c_str());
		if (ties[i] == "depth") {
			DepthPriority* depth = new DepthPriority();
			comp.comps.push_back(depth);
		} else if (ties[i] == "bip") {
			BondIPPrioirty* bip = new BondIPPrioirty();
			comp.comps.push_back(bip);
		} else if (ties[i] == "novelty") {
			if (!image_based) {
				if (m_novelty_boolean_representation) {
					m_ram_novelty_table_true = new aptk::Bit_Matrix(RAM_SIZE,
							8);
					m_ram_novelty_table_false = new aptk::Bit_Matrix(RAM_SIZE,
							8);
				} else {
					m_ram_novelty_table = new aptk::Bit_Matrix(RAM_SIZE, 256);
				}
			} else {
				int image_size = _env->getScreen().width()
						* _env->getScreen().height();
				//		printf("image_size = %d\n", image_size);
				m_image_novelty_table = new aptk::Bit_Matrix(image_size,
						256 * sizeof(unsigned char));
			}
			NoveltyPriority* novelty = new NoveltyPriority();
			comp.comps.push_back(novelty);
		} else {
			printf("error ties %s\n", ties[i].c_str());
			assert(false);
		}
	}

	m_q_percolation = new priority_queue<TreeNodeExp*, vector<TreeNodeExp*>,
			ListComparator>(comp);
}

BondPercolation::~BondPercolation() {
}

/* *********************************************************************
 Builds a new tree
 ******************************************************************* */
void BondPercolation::build(ALEState & state) {
	assert(p_root == NULL);
	p_root = new TreeNode(NULL, state, NULL, UNDEFINED, 0);
	update_tree();
	is_built = true;

}

void BondPercolation::print_path(TreeNode * node, int a) {
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

void BondPercolation::update_tree() {
	expand_tree(p_root);

}

int BondPercolation::expand_node(TreeNode* curr_node) {
	int steps = 0;
	for (int a = 0; a < available_actions.size(); ++a) {
		steps += expand_node(curr_node, a);
	}
	return steps;
}

int BondPercolation::expand_node(TreeNode* curr_node, int action) {
	int num_simulated_steps = 0;
	int num_actions = available_actions.size();
	bool leaf_node = (curr_node->v_children.empty());
	static int max_nodes_per_frame = max_sim_steps_per_frame
			/ sim_steps_per_node;
// Expand all of its children (simulates the result)
	if (leaf_node) {
		curr_node->v_children.resize(num_actions);
		curr_node->available_actions = available_actions;
		m_expanded_nodes++;
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

	//	int a = (int) action;
	// TODO: just wanna make it looks similar to the other methods.
	for (int a = action; a < action + 1; a++) {
		Action act = curr_node->available_actions[a];
//		printf("apply action %d\n", (int)act);

		TreeNode * child;

		// If nullptr, then generate a new node by simulation.
		if (curr_node->v_children[a] == nullptr) {

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
//				printf("pruned state\n");
			curr_node->v_children[a] = child;
			child->is_terminal = false;

			if (std::find(ties.begin(), ties.end(), string("novelty"))
					!= ties.end()) {
				if (check_novelty_1(child->state)) {
					curr_node->novelty = 1;
					update_novelty_table(child->state);
				} else {
					curr_node->novelty = 256;
				}
			}

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
				for (int i = 0; i < num_actions; i++) {
					TreeNodeExp* c = new TreeNodeExp(child, rand(), i);
					m_q_percolation->push(c);
				}
			} else {
				m_pruned_nodes++;
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
void BondPercolation::expand_tree(TreeNode* start_node) {

	if (!start_node->v_children.empty()) {
		start_node->updateTreeNode();
		for (int a = 0; a < available_actions.size(); a++) {
			if (start_node->v_children[a] == nullptr) {
				continue;
			}
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
		TreeNode* piv = pivots.front();
		int steps = 0;
		for (int a = 0; a < available_actions.size(); ++a) {
			steps += expand_node(piv, a);
		}
//		int steps = expand_node(pivots.front());
		num_simulated_steps += steps;

		if (num_simulated_steps >= max_sim_steps_per_frame) {
			break;
		}

		pivots.pop_front();

		while (!m_q_percolation->empty()) {
			// Pop a node to expand
			TreeNodeExp* node_action = m_q_percolation->top();
//			printf("top() node_action\n");
			m_q_percolation->pop();
			TreeNode* curr_node = node_action->node;
			int action = node_action->action;
			delete node_action;

			if (curr_node->depth() > m_reward_horizon - 1)
				continue;
			if (m_stop_on_first_reward && curr_node->node_reward != 0) {
				pivots.push_back(curr_node);
				continue;
			}
			steps = expand_node(curr_node, action);
			num_simulated_steps += steps;
			// Stop once we have simulated a maximum number of steps
			if (num_simulated_steps >= max_sim_steps_per_frame) {
				break;
			}

		}
		std::cout << "\tExpanded so far: " << m_expanded_nodes << std::endl;
		std::cout << "\tPruned so far: " << m_pruned_nodes << std::endl;
		std::cout << "\tGenerated so far: " << m_generated_nodes << std::endl;

		if (m_q_percolation->empty())
			std::cout << "Search Space Exhausted!" << std::endl;

		// Stop once we have simulated a maximum number of steps
		if (num_simulated_steps >= max_sim_steps_per_frame) {
			break;
		}

	} while (!pivots.empty());

	update_branch_return(start_node);
}

void BondPercolation::update_novelty_table(ALEState& machine_state) {
	if (!image_based) {
		const ALERAM ram_state = machine_state.getRAM();
		for (size_t i = 0; i < ram_state.size(); i++)
			if (m_novelty_boolean_representation) {
				unsigned char mask = 1;
				byte_t byte = ram_state.get(i);
				for (int j = 0; j < 8; j++) {
					bool bit_is_set = (byte & (mask << j)) != 0;
					if (bit_is_set)
						m_ram_novelty_table_true->set(i, j);
					else
						m_ram_novelty_table_false->set(i, j);
				}
			} else
				m_ram_novelty_table->set(i, ram_state.get(i));
	} else {
//		assert(machine_state.);

		assert(get_screen(machine_state).getArray());
		assert(get_screen(machine_state).arraySize() != 0);
		const ALEScreen image = get_screen(machine_state);
		int m_column = image.width();
//		printf("image_size=%d\n", image.arraySize());
		for (size_t i = 0; i < image.arraySize(); ++i) {
			m_image_novelty_table->set(i,
					image.get(i / m_column, i % m_column));
		}
	}
}

bool BondPercolation::check_novelty_1(ALEState& machine_state) {
	if (!image_based) {
		ALERAM ram_state = machine_state.getRAM();
		for (size_t i = 0; i < ram_state.size(); i++)
			if (m_novelty_boolean_representation) {
				unsigned char mask = 1;
				byte_t byte = ram_state.get(i);
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
			} else if (!m_ram_novelty_table->isset(i, ram_state.get(i)))
				return true;
		return false;
	} else {
		const ALEScreen image = get_screen(machine_state);
		int m_column = image.width();
//		printf("image_size=%d\n", image.arraySize());
		for (size_t i = 0; i < image.arraySize(); ++i) {
			if (m_image_novelty_table->isset(i,
					image.get(i / m_column, i % m_column))) {
				return true;
			}
		}
		return false;
	}
}

const ALEScreen BondPercolation::get_screen(ALEState &machine_state) {
	ALEState buffer = m_env->cloneState();
	m_env->restoreState(machine_state);
	const ALEScreen screen = m_env->buildAndGetScreen();
	m_env->restoreState(buffer);
	return screen;
}

void BondPercolation::clear() {
	SearchTree::clear();
	while (!m_q_percolation->empty()) {
		m_q_percolation->pop();
	}
//	std::priority_queue<TreeNodeExp*, std::vector<TreeNodeExp*>, BondIPPrioirty> emptyn;
//	std::swap(m_q_percolation, emptyn);
}

void BondPercolation::move_to_best_sub_branch() {
	SearchTree::move_to_best_sub_branch();
	while (!m_q_percolation->empty()) {
		m_q_percolation->pop();
	}
}

/* *********************************************************************
 Updates the branch reward for the given node
 which equals to: node_reward + max(children.branch_return)
 ******************************************************************* */
void BondPercolation::update_branch_return(TreeNode* node) {
// Base case (leaf node): the return is the immediate reward
	if (node->v_children.empty()) {
		node->branch_return = node->node_reward;
		node->best_branch = -1;
		node->branch_depth = node->m_depth;
		return;
	}

// First, we have to make sure that all the children are updated
	for (unsigned int c = 0; c < node->v_children.size(); c++) {
		if (node->v_children[c] == nullptr) {
			continue;
		}
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
			if (node->v_children[a] == nullptr) {
				continue;
			}
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

void BondPercolation::set_terminal_root(TreeNode * node) {
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

void BondPercolation::print_frame_data(int frame_number, float elapsed,
		Action curr_action, std::ostream& output) {
	output << "frame=" << frame_number;
	output << ",expanded=" << expanded_nodes();
	output << ",generated=" << generated_nodes();
	output << ",pruned=" << pruned();
	output << ",jasd_pruned=" << jasd_pruned();
	output << ",reused=" << m_reused_nodes;
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


/**
 * If node b should be prioritized, then return true.
 *
 */
bool BondPercolation::BondIPPrioirty::operator ()(TreeNodeExp* a,
		TreeNodeExp* b) const {
	if (a->priority > b->priority)
		return true;
	else
		return false;
}


bool BondPercolation::DepthPriority::operator()(TreeNodeExp* a,
		TreeNodeExp* b) const {
	if (a->node->depth() > b->node->depth()) {
		return true;
	} else {
		return false;
	}
}

bool BondPercolation::NoveltyPriority::operator()(TreeNodeExp* a,
		TreeNodeExp* b) const {
	if (a->node->novelty > b->node->novelty) {
		return true;
	} else {
		return false;
	}
}
