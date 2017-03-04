/* *****************************************************************************
 * A.L.E (Arcade Learning Environment)
 * Copyright (c) 2009-2012 by Yavar Naddaf, Joel Veness, Marc G. Bellemare
 * Released under GNU General Public License www.gnu.org/licenses/gpl-3.0.txt
 *
 * Based on: Stella  --  "An Atari 2600 VCS Emulator"
 * Copyright (c) 1995-2007 by Bradford W. Mott and the Stella team
 *
 * *****************************************************************************
 *  SearchTree.cpp
 *
 *  Implementation of the SearchTree class, which represents the search tree for the
 *  Search-Agent
 **************************************************************************** */

#include "SearchTree.hpp"
#include "random_tools.h"

//#include <time.h>
#include <chrono>
#include "ActionSequenceDetection.hpp"

/* *********************************************************************
 Constructor
 ******************************************************************* */
SearchTree::SearchTree(RomSettings * rom_settings, Settings & settings,
		ActionVect &_actions, StellaEnvironment* _env) :
		is_built(false), p_root(NULL), ignore_duplicates(true), reward_magnitude(
				0), m_rom_settings(rom_settings), available_actions(_actions), total_simulation_steps(
				0) {

	sim_steps_per_node = settings.getInt("sim_steps_per_node", true);
	max_sim_steps_per_frame = settings.getInt("max_sim_steps_per_frame", false);
	num_simulations_per_frame = settings.getInt("num_simulations_per_frame",
			false);

//    std::cout << "max_sim_steps_per_frame: "<< max_sim_steps_per_frame << std::endl;
	assert(
			max_sim_steps_per_frame != -1
					|| settings.getInt("uct_monte_carlo_steps", false) != -1);

	discount_factor = settings.getFloat("discount_factor", true);
	depth_normalized_reward = settings.getBool("depth_normalized_reward",
			false);

	// Default: false
	normalize_rewards = settings.getBool("normalize_rewards", false);
	// Default: false
	ignore_duplicates = settings.getBool("ignore_duplicates_nodes", false);
	m_env = _env;
	m_randomize_successor = settings.getBool("randomize_successor_novelty",
			false);
	m_novelty_pruning = false;
	m_player_B = false;

	m_emulation_time = 0;
	m_context_time = 0;

	action_sequence_detection = settings.getBool("action_sequence_detection",
			false);

	if (action_sequence_detection) {
		printf("RUNNING ACTION SEQUENCE DETECTION\n");
		asd = new ActionSequenceDetection(settings);
		junk_decision_frame = settings.getInt("junk_decision_frame", false);
		junk_resurrection_frame = settings.getInt("junk_resurrection_frame",
				false);
		longest_junk_sequence = settings.getInt("longest_junk_sequence", false);

		decision_frame_function = settings.getInt("decision_frame_function",
				false);

		if (junk_decision_frame < 0) {
			junk_decision_frame = 12; // 5 seconds in game
		}

		if (junk_resurrection_frame < 0) {
			junk_resurrection_frame = 20;
		}

		if (decision_frame_function < 0) {
			decision_frame_function = 1;
		}

		if (longest_junk_sequence < 0) {
			longest_junk_sequence = 2;
		}

//		current_junk_length = 1;
	}

	image_based = settings.getBool("image_based", false);

	printf("MinimalActionSet= %d\n",
			m_rom_settings->getMinimalActionSet().size());
}

/* *********************************************************************
 Deletes the search-tree
 ******************************************************************* */
void SearchTree::clear(void) {
	if (p_root != NULL) {
		delete_branch(p_root);
		p_root = NULL;
	}
	is_built = false;
	m_max_depth = 0;

}

/* *********************************************************************
 Destructor
 ******************************************************************* */
SearchTree::~SearchTree(void) {
	clear();
	if (action_sequence_detection) {
		delete asd;
	}
}

/* *********************************************************************
 Returns the best action based on the expanded search tree
 ******************************************************************* */
Action SearchTree::get_best_action(void) {
	assert(p_root != NULL);
	int best_branch = p_root->best_branch;
	TreeNode* best_child = p_root->v_children[best_branch];
	vector<int> best_branches;
	assert(best_branch != -1);

	if (depth_normalized_reward) {
//		best_branch = -1;
		reward_t best_reward = -1.0;
		for (size_t c = 0; c < p_root->v_children.size(); c++) {
			TreeNode* child = p_root->v_children[c];
			double norm_divides = (1.0
					- pow(discount_factor, child->branch_depth))
					/ (1.0 - discount_factor);
			reward_t r = child->branch_return / norm_divides;
			if (r > best_reward) {
				best_reward = r;
				best_branch = c;
			}
		}
		best_child = p_root->v_children[best_branch];
		best_branches.push_back(best_branch);
	} else {
		best_branches.push_back(best_branch);
	}

	for (size_t c = 0; c < p_root->v_children.size(); c++) {
		TreeNode* curr_child = p_root->v_children[c];
		// Ignore duplicates if the flag is set
		if (ignore_duplicates && curr_child->is_duplicate())
			continue;

		if (c != (size_t) best_branch
				&& curr_child->branch_return == best_child->branch_return
				&& curr_child->is_terminal == best_child->is_terminal) {
			best_branches.push_back(c);

		}
	}

	if (best_branches.size() > 1) {
		// best_branch = 0;
		// unsigned best_depth = p_root->v_children[ best_branches[0] ]->branch_depth;

		// for( unsigned i = 0; i < best_branches.size(); i++){
		// 	TreeNode* curr_child = p_root->v_children[ best_branches[i] ];

		// 	std::cout << "Action: " << action_to_string(curr_child->act) << "/" << action_to_string( p_root->available_actions[ best_branches[i] ] ) << " Depth: " << curr_child->branch_depth << " NumNodes: " << curr_child->num_nodes() << " Reward: "<< curr_child->branch_return   << std::endl;
		// 	if(best_depth <  curr_child->branch_depth ){
		// 		best_depth = curr_child->branch_depth;
		// 		best_branch = best_branches[i];
		// 	}

		// }

		// 	// when we have more than one best-branch, pick one randomly
		best_branch = choice(&best_branches);
	}
	for (size_t c = 0; c < p_root->v_children.size(); c++) {
		TreeNode* curr_child = p_root->v_children[c];

		std::cout << "Action: " << action_to_string(curr_child->act)
				<< " Depth: " << curr_child->branch_depth << " NumNodes: "
				<< curr_child->num_nodes() << " Reward: "
				<< curr_child->branch_return << std::endl;

	}

	p_root->best_branch = best_branch;
	printf("best_branch=%s\n",
			action_to_string(p_root->available_actions[best_branch]).c_str());
	return p_root->available_actions[best_branch];
}

/* *********************************************************************
 Moves the best sub-branch of the root to be the new root of the tree
 ******************************************************************* */
void SearchTree::move_to_best_sub_branch(void) {
	assert(p_root->v_children.size() > 0);
	assert(p_root->best_branch != -1);

	// Delete all the other branches
	for (size_t del = 0; del < p_root->v_children.size(); del++) {
		if (del != (size_t) p_root->best_branch) {
			delete_branch(p_root->v_children[del]);
		}
	}

	TreeNode* old_root = p_root;
	p_root = p_root->v_children[p_root->best_branch];
	// make sure the child I want to become root doesn't get deleted:
	old_root->v_children[old_root->best_branch] = NULL;
	delete old_root;
	p_root->p_parent = NULL;
	m_max_depth = 0;
}

/* *********************************************************************
 Deletes a node and all its children, all the way down the branch
 ******************************************************************* */
void SearchTree::delete_branch(TreeNode* node) {
	if (!node->v_children.empty()) {
		for (size_t c = 0; c < node->v_children.size(); c++) {
			delete_branch(node->v_children[c]);
		}
	}
	delete node;

}

bool SearchTree::test_duplicate(TreeNode *node) {
	if (node->p_parent == NULL)
		return false;
	else {
		TreeNode * parent = node->p_parent;
		ALEScreen nodeImg = node->state.getScreen();
		// Compare each valid sibling with this one
		for (size_t c = 0; c < parent->v_children.size(); c++) {
			TreeNode * sibling = parent->v_children[c];
			// Ignore duplicates, this node and uninitialized nodes
			if (sibling->is_duplicate() || sibling == node
					|| !sibling->is_initialized())
				continue;

			if (image_based) {
				// YJ: Image-based duplicate detection.
				//     It is more natural to use image for realistic situation.
				if (sibling->state.getScreen().equals(nodeImg)) {
					node->duplicate = true;
					return true;
				}
			} else {

				if (sibling->state.equals(node->state)) {

					node->duplicate = true;
					return true;
				}
			}
		}

		// None of the siblings match, unique node
		node->duplicate = false;
		return false;
	}
}

int SearchTree::simulate_game(ALEState & state, Action act, int num_steps,
		return_t &traj_return, bool &game_ended, bool discount_return,
		bool save_state) {

	// Load the state into the emulator - a copy of the parent state
	auto context_start = std::chrono::high_resolution_clock::now();

	m_env->restoreState(state);

	auto context_elapsed = std::chrono::high_resolution_clock::now()
			- context_start;
	long long context_microseconds = std::chrono::duration_cast
			< std::chrono::microseconds > (context_elapsed).count();
//	printf("t=%.2f, %lld, microseconds);

	m_context_time += context_microseconds;

	// For discounting purposes
	float g = 1.0;
	traj_return = 0.0;
	game_ended = false;
	Action a;

	// So that the compiler doesn't complain
	if (act == RANDOM)
		a = PLAYER_A_NOOP;
	else
		a = act;

	int i;

//	time_t start, end;
//	time(&start);
//	std::chrono::steady_clock::time_point start =
//			std::chrono::steady_clock::now();

	auto start = std::chrono::high_resolution_clock::now();

	for (i = 0; i < num_steps; i++) {
		if (act == RANDOM && i % sim_steps_per_node == 0)
			a = choice(&available_actions);

		// Move state forward using action a
		m_env->set_player_B(m_player_B);
		reward_t curr_reward;
		if (m_player_B)
			curr_reward = m_env->oneStepAct(PLAYER_A_NOOP, a);
		else
			curr_reward = m_env->oneStepAct(a, PLAYER_B_NOOP);

		game_ended = m_env->isTerminal();

		return_t r = normalize_rewards ? normalize(curr_reward) : curr_reward;

		// Add curr_reward to the trajectory return
		if (discount_return) {
			traj_return += r * g;
			// Update the discount factor every sim_steps_per_ndoe
			if ((i + 1) % sim_steps_per_node == 0)
				g *= discount_factor;
		} else
			traj_return += r;

		// Early exit if we reach termination
		if (game_ended) {
			i++;
			break;
		}
	}

//	time(&end);
//	std::chrono::steady_clock::time_point end =
//			std::chrono::steady_clock::now();
//
//	typedef std::chrono::duration<int, std::milli> millisecs_t;
//	millisecs_t duration(std::chrono::duration_cast<millisecs_t>(end - start));
//	printf("%.2f millisecnds.\n", duration.count());

	auto elapsed = std::chrono::high_resolution_clock::now() - start;
	long long microseconds = std::chrono::duration_cast
			< std::chrono::microseconds > (elapsed).count();
//	printf("t=%.2f, %lld, microseconds);

	m_emulation_time += microseconds;
	m_total_simulation_steps += i;

	// Save the result
	if (save_state)
		state = m_env->cloneState();

	return i;
}

return_t SearchTree::normalize(reward_t reward) {
	if (reward == 0)
		return 0;
	else {
		// Set reward magnitude to first observed non-zero reward
		if (reward_magnitude == 0)
			reward_magnitude = abs(reward);
		return (reward + 0.0) / reward_magnitude;
	}

}

int SearchTree::num_nodes() {
	if (p_root == NULL)
		return 0;
	else
		return p_root->num_nodes();
}

void SearchTree::print_best_path() {
	if (p_root == NULL)
		cerr << "no path" << endl;
	else {
		TreeNode * node = p_root;

		cerr << "Path ";
		while (!node->v_children.empty()) {
			// Print action taken at node
			cerr << node->best_branch << " ";
			node = node->v_children[node->best_branch];
		}

		cerr << endl;
	}
}

int SearchTree::get_root_frame_number() {
	return p_root->state.getFrameNumber();
}

long SearchTree::num_simulation_steps() {
	long s = total_simulation_steps;
	total_simulation_steps = 0;

	return s;
}

void SearchTree::print_frame_data(int frame_number, float elapsed,
		Action curr_action, std::ostream& output) {
	output << "frame=" << frame_number;
	output << ",expanded=" << expanded_nodes();
	output << ",generated=" << generated_nodes();
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

void SearchTree::getJunkActionSequence(int frame_number) {
	if (action_sequence_detection) {
		int agent_frame = frame_number / 5;
//		int decision_frame = junk_decision_frame
//				* pow(current_junk_length, decision_frame_function);

//		assert(agent_frame <= decision_frame);
		// TODO: The incrementing algorithm for junk length is not obvious.
		// For now it's n*n*n.
//		if (agent_frame == decision_frame) {
//			++current_junk_length;
//		}

//		if (frame_number > 0 && (frame_number / 5) % junk_decision_frame == 0) {
//			if (longest_junk_sequence >= current_junk_length) {
//				++current_junk_length;
//			}
//		}

//		if (longest_junk_sequence >= current_junk_length) {
		asd->getJunkActionSequence(this, longest_junk_sequence);
//		}

	}
}

std::vector<bool> SearchTree::getUsefulActions(vector<Action> previousActions) {
	return asd->getUsefulActions(previousActions);
}

std::vector<Action> SearchTree::getPreviousActions(TreeNode* node,
		int seqLength) {
	std::vector<Action> previousActions;
	previousActions.resize(seqLength);

	TreeNode* curr = node;
	int currLength = seqLength;

	while (currLength > 0) {
		if (curr == p_root) {
			previousActions.assign(trajectory.end() - currLength,
					trajectory.end());
			break;
		}
		previousActions[currLength - 1] = curr->act;
		curr = node->p_parent;
		currLength--;
	}
	return previousActions;
}

void SearchTree::saveUsedAction(int frame_number, Action action) {
	if (action_sequence_detection) {
		trajectory.push_back(action);
	}
}

int SearchTree::getDetectedUsedActionsSize() {
	if (action_sequence_detection) {
		return asd->getDetectedUsedActionsSize();
	} else {
		return 0;
	}
}
