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

#include "DominatedActionSequenceDetection.hpp"
#include "DominatedActionSequencePruning.hpp"
#include "DominatedActionSequenceAvoidance.hpp"

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
		bool isDASA = settings.getBool("dasa", false)
				|| settings.getBool("probablistic_action_selection", false);
		if (isDASA) {
			dasd = new DominatedActionSequenceAvoidance(settings, _env);
		} else {
			dasd = new DominatedActionSequencePruning(settings, _env);
		}

		dasd_sequence_length = max(
				settings.getInt("longest_junk_sequence", false),
				settings.getInt("dasd_sequence_length", false));

//		if (junk_resurrection_frame < 0) {
//			junk_resurrection_frame = 20;
//		}

//		if (decision_frame_function < 0) {
//			decision_frame_function = 1;
//		}

		if (dasd_sequence_length < 0) {
			dasd_sequence_length = 2;
		}

//		current_junk_length = 1;
	}

	image_based = settings.getBool("image_based", false);

	printf("MinimalActionSet= %d\n",
			m_rom_settings->getMinimalActionSet().size());

	erroneous_prediction = settings.getBool("erroneous_prediction", false);
	if (erroneous_prediction) {
		prediction_error_rate = settings.getFloat("prediction_error_rate",
				false);
		if (prediction_error_rate < 0) {
			prediction_error_rate = 0.03;
		}
		printf("Prediction Error Rate = %f\n", prediction_error_rate);
	}

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
		delete dasd;
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
		if (p_root->v_children[del] == nullptr) {
			continue;
		}
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

void SearchTree::move_to_branch(Action a, int duration) {
	assert(p_root->v_children.size() > 0);
	int best_branch = -1;
	if (duration == sim_steps_per_node) {
		bool prediction_error = false;
		TreeNode* newChild = new TreeNode(p_root, p_root->state, this, a,
				duration, discount_factor);
		// Delete all the other branches
		for (size_t del = 0; del < p_root->v_children.size(); del++) {
			if (p_root->v_children[del] == nullptr) {
				continue;
			}
			if (p_root->v_children[del]->act != a) {
				delete_branch(p_root->v_children[del]);
			} else {
				if (newChild->state.equals(p_root->v_children[del]->state)) {
					best_branch = del;
				} else {
					printf("Prediction error\n");
					prediction_error = true;
				}
			}
		}
		if (prediction_error) {
			TreeNode* old_root = p_root;
			p_root = newChild;
			delete old_root;
			p_root->p_parent = NULL;
			m_max_depth = 0;
		} else {
			TreeNode* old_root = p_root;
			p_root = p_root->v_children[best_branch];
			// make sure the child I want to become root doesn't get deleted:
			old_root->v_children[old_root->best_branch] = NULL;
			delete old_root;
			p_root->p_parent = NULL;
			m_max_depth = 0;
		}
	} else {
		ALEState buffer = m_env->cloneState();
		ALEState s = p_root->state;
		return_t r = 0;
		bool terminated = false;
		TreeNode* newChild = new TreeNode(p_root, s, this, a, duration,
				discount_factor);

		int best_branch = -1;
		for (int i = 0; i < p_root->v_children.size(); ++i) {
			if (newChild->state.equals(p_root->v_children[i]->state)) {
				best_branch = i;
				break;
			} else {
				delete_branch(p_root->v_children[i]);
			}
		}
		if (best_branch != -1) {
			delete newChild;
			TreeNode* old_root = p_root;
			p_root = p_root->v_children[best_branch];
			// make sure the child I want to become root doesn't get deleted:
			old_root->v_children[old_root->best_branch] = NULL;
			delete old_root;
			p_root->p_parent = NULL;
			m_max_depth = 0;
		} else {
			TreeNode* old_root = p_root;
			p_root = newChild;
			// make sure the child I want to become root doesn't get deleted:
//			old_root->v_children[old_root->best_branch] = NULL;
			delete old_root;
			p_root->p_parent = NULL;
			m_max_depth = 0;
		}

		m_env->restoreState(buffer);
	}

}

/* *********************************************************************
 Deletes a node and all its children, all the way down the branch
 ******************************************************************* */
void SearchTree::delete_branch(TreeNode* node) {
	if (!node->v_children.empty()) {
		for (size_t c = 0; c < node->v_children.size(); c++) {
			if (node->v_children[c] == nullptr) {
				continue;
			}
			delete_branch(node->v_children[c]);
		}
	}
	delete node;

}

bool SearchTree::test_duplicate(TreeNode *node) {
	// TODO: Image based is problematic when the action does not give immediate difference.
	if (image_based && node->p_parent == this->p_root) {
		return false;
	}
	if (node->p_parent == NULL)
		return false;
	else {
		TreeNode * parent = node->p_parent;
		ALEScreen nodeImg = node->state.getScreen();
		// Compare each valid sibling with this one
		for (size_t c = 0; c < parent->v_children.size(); c++) {
			TreeNode * sibling = parent->v_children[c];
			if (sibling == nullptr) {
				continue;
			}
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
	long long context_microseconds = std::chrono::duration_cast<
			std::chrono::microseconds>(context_elapsed).count();
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
	long long microseconds = std::chrono::duration_cast<
			std::chrono::microseconds>(elapsed).count();
//	printf("t=%.2f, %lld, microseconds);

	m_emulation_time += microseconds;
	m_total_simulation_steps += i;

	// Save the result
	if (save_state)
		state = m_env->cloneState();

	return i;
}

// Simulate game randomly using a particular action_set.
int SearchTree::simulate_game_random(ALEState & state, ActionVect&action_set,
		int num_steps, return_t &traj_return, bool &game_ended,
		bool discount_return, bool save_state) {
	ActionVect buffer = available_actions;
	available_actions = action_set;
	int steps = simulate_game(state, RANDOM, num_steps, traj_return, game_ended,
			discount_return, save_state);
	available_actions = buffer;

	return steps;
}

int SearchTree::simulate_game_err(ALEState & state, Action act, int num_steps,
		return_t &traj_return, bool &game_ended, bool discount_return,
		bool save_state) {
	if (erroneous_prediction) {
		pair<Action, int> rnd = randomizeAction(act, num_steps);
//		if (rnd.first != act) {
//			printf("Randomized prediction from %d to %d\n", (int) act,
//					(int) rnd.first);
//		}
//		if (rnd.second != num_steps) {
//			printf("duration = %d\n", rnd.second);
//
//		}
		return simulate_game(state, rnd.first, rnd.second, traj_return,
				game_ended, discount_return, save_state);
	} else {
		return simulate_game(state, act, num_steps, traj_return, game_ended,
				discount_return, save_state);
	}
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
//		int agent_frame = frame_number / 5;
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
		dasd->learnDominatedActionSequences(this, dasd_sequence_length);
//		}

	}
}

//std::vector<bool> SearchTree::getUsefulActions(vector<Action> previousActions) {
//	if (m_env->getFrameNumber() < junk_decision_frame) {
//		return vector<bool>(PLAYER_A_MAX, true);
//	} else {
//		return dasd->getEffectiveActions(previousActions);
//	}
//}

std::vector<Action> SearchTree::getPreviousActions(const TreeNode* node,
		int seqLength) const {
	std::vector<Action> previousActions;
	previousActions.resize(seqLength);

	TreeNode* curr = (TreeNode*) node;
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
		return dasd->getDetectedUsedActionsSize();
	} else {
		return 0;
	}
}

pair<Action, int> SearchTree::randomizeAction(Action a, int duration) {
	int x_axis;
	int y_axis;
	bool fire;
	if (a == PLAYER_A_NOOP || a == PLAYER_A_UP || a == PLAYER_A_DOWN
			|| a == PLAYER_A_FIRE || a == PLAYER_A_UPFIRE
			|| a == PLAYER_A_DOWNFIRE) {
		x_axis = 0;
	} else if (a == PLAYER_A_LEFT || a == PLAYER_A_UPLEFT
			|| a == PLAYER_A_DOWNLEFT || a == PLAYER_A_LEFTFIRE
			|| a == PLAYER_A_UPLEFTFIRE || a == PLAYER_A_DOWNLEFTFIRE) {
		x_axis = -1;
	} else {
		x_axis = 1;
	}

	if (a == PLAYER_A_NOOP || a == PLAYER_A_LEFT || a == PLAYER_A_RIGHT
			|| a == PLAYER_A_FIRE || a == PLAYER_A_LEFTFIRE
			|| a == PLAYER_A_RIGHTFIRE) {
		y_axis = 0;
	} else if (a == PLAYER_A_DOWN || a == PLAYER_A_DOWNLEFT
			|| a == PLAYER_A_DOWNRIGHT || a == PLAYER_A_DOWNFIRE
			|| a == PLAYER_A_DOWNLEFTFIRE || a == PLAYER_A_DOWNRIGHTFIRE) {
		y_axis = -1;
	} else {
		y_axis = 1;
	}

	if (a == PLAYER_A_NOOP || a == PLAYER_A_UP || a == PLAYER_A_RIGHT
			|| a == PLAYER_A_DOWN || a == PLAYER_A_LEFT || a == PLAYER_A_UPRIGHT
			|| a == PLAYER_A_DOWNRIGHT || a == PLAYER_A_DOWNLEFT
			|| a == PLAYER_A_UPLEFT) {
		fire = false;
	} else {
		fire = true;
	}

	if (rand() < prediction_error_rate * (double) RAND_MAX) {
		vector<pair<int, int>> errors;
		if (x_axis <= 0) {
			errors.push_back(pair<int, int>(1, 0));
		}
		if (x_axis >= 0) {
			errors.push_back(pair<int, int>(-1, 0));
		}
		if (y_axis <= 0) {
			errors.push_back(pair<int, int>(0, 1));
		}
		if (y_axis >= 0) {
			errors.push_back(pair<int, int>(0, -1));
		}
		pair<int, int> err = choice(&errors);
		x_axis += err.first;
		y_axis += err.second;
	}

	if (rand() < prediction_error_rate * (double) RAND_MAX) {
		fire = !fire;
	}

	Action ret;
	int dur = duration;

	if (x_axis == 0 && y_axis == 0) {
		ret = (Action) fire;
	} else {
		int bias = 2;
		if (fire) {
			bias = 10;
		}
		if (x_axis == 0 && y_axis == 1) {
			ret = (Action) bias;
		} else if (x_axis == 1 && y_axis == 0) {
			ret = (Action) (bias + 1);
		} else if (x_axis == -1 && y_axis == 0) {
			ret = (Action) (bias + 2);
		} else if (x_axis == 0 && y_axis == -1) {
			ret = (Action) (bias + 3);
		} else if (x_axis == 1 && y_axis == 1) {
			ret = (Action) (bias + 4);
		} else if (x_axis == -1 && y_axis == 1) {
			ret = (Action) (bias + 5);
		} else if (x_axis == 1 && y_axis == -1) {
			ret = (Action) (bias + 6);
		} else if (x_axis == -1 && y_axis == -1) {
			ret = (Action) (bias + 7);
		} else {
			assert(false && "randomizeAction error");
			ret = (Action) 0;
		}
	}

	if (rand() < prediction_error_rate * RAND_MAX) {
		if (rand() % 2) {
			dur += 1;
		} else {
			dur -= 1;
		}
//		printf("duration = %d\n", dur);
	}
	return pair<Action, int>(ret, dur);
}

