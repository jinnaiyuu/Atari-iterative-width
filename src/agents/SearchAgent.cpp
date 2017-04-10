/* *****************************************************************************
 * A.L.E (Arcade Learning Environment)
 * Copyright (c) 2009-2012 by Yavar Naddaf, Joel Veness, Marc G. Bellemare
 * Released under GNU General Public License www.gnu.org/licenses/gpl-3.0.txt
 *
 * Based on: Stella  --  "An Atari 2600 VCS Emulator"
 * Copyright (c) 1995-2007 by Bradford W. Mott and the Stella team
 *
 * *****************************************************************************
 *  SearchAgent.cpp
 *
 * The implementation of the SearchAgent class, which uses Search Algorithms
 * to act in the game
 **************************************************************************** */

#include "SearchAgent.hpp"
//#include "game_controller.h"
#include "random_tools.h"
#include "Serializer.hxx"
#include "Deserializer.hxx"
#include "System.hxx"
#include <sstream>

#include "BreadthFirstSearch.hpp"
#include "IW1Search.hpp"
#include "PIW1Search.hpp"

#include "UniformCostSearch.hpp"
#include "BestFirstSearch.hpp"
#include "UCTSearchTree.hpp"
#include "time.hxx"

#include "Priorities.hpp"

// YJ: IP searches
#include "SitePercolation.hpp"
#include "BondPercolation.hpp"

SearchAgent::SearchAgent(OSystem* _osystem, RomSettings* _settings,
		StellaEnvironment* _env, bool player_B) :
		PlayerAgent(_osystem, _settings), m_curr_action(UNDEFINED), m_current_episode(
				0) {
	search_method = p_osystem->settings().getString("search_method", true);

	if (player_B) {
		available_actions = _settings->getAllActions_B();
		search_method = p_osystem->settings().getString("search_method_B",
				false);
	}

	// Depending on the configuration, create a SearchTree of the requested type
	if (search_method == "brfs") {
		search_tree = new BreadthFirstSearch(_settings, _osystem->settings(),
				available_actions, _env);
		m_trace.open("brfs.search-agent.trace");
	} else if (search_method == "ucs") {
		search_tree = new UniformCostSearch(_settings, _osystem->settings(),
				available_actions, _env);

		m_trace.open("ucs.search-agent.trace");

	} else if (search_method == "iw1") {
		search_tree = new IW1Search(_settings, _osystem->settings(),
				available_actions, _env);

		search_tree->set_novelty_pruning();
		m_trace.open("iw1.search-agent.trace");

	} else if (search_method == "piw1") {
		search_tree = new PIW1Search(_settings, _osystem->settings(),
				available_actions, _env);

		search_tree->set_novelty_pruning();
		m_trace.open("piw1.search-agent.trace");

	} else if (search_method == "bfs") {
		search_tree = new BestFirstSearch(_settings, _osystem->settings(),
				available_actions, _env);

		search_tree->set_novelty_pruning();
		m_trace.open("bfs.search-agent.trace");
	} else if (search_method == "sips") {
		search_tree = new SitePercolation(_settings, _osystem->settings(),
				available_actions, _env);

		m_trace.open("sips.search-agent.trace");
	} else if (search_method == "bips") {
		search_tree = new BondPercolation(_settings, _osystem->settings(),
				available_actions, _env);

		m_trace.open("bips.search-agent.trace");

	} else if (search_method == "uct") {
		search_tree = new UCTSearchTree(_settings, _osystem->settings(),
				available_actions, _env);
		m_trace.open("uct.search-agent.trace");
	} else {
		cerr << "Unknown search Method: " << search_method << endl;
		exit(-1);
	}
	m_rom_settings = _settings;
	m_env = _env;

	search_tree->set_player_B(player_B);

	Settings &settings = _osystem->settings();
	sim_steps_per_node = settings.getInt("sim_steps_per_node", true);

	erroneous_action = settings.getBool("erroneous_action", false);
	if (erroneous_action) {
		action_error_rate = settings.getFloat("action_error_rate", false);
		if (action_error_rate < 0) {
			action_error_rate = 0.03;
		}
		printf("Action Error Rate = %f\n", action_error_rate);
	}

	m_curr_action_duration = 0;
	m_curr_action_duration_left = 0;
}

SearchAgent::~SearchAgent() {

	m_trace.close();
}

int SearchAgent::num_available_actions() {
	return available_actions.size();
}

ActionVect& SearchAgent::get_available_actions() {
	return available_actions;
}

Action SearchAgent::agent_step() {
	Action act = PlayerAgent::agent_step();

	state.incrementFrame();

	return act;
}

//Action SearchAgent::act() {
//	return act_dur().first;
//}
/* *********************************************************************
 Returns a random action from the set of possible actions
 ******************************************************************** */
Action SearchAgent::act() {
	// Generate a new action every sim_steps_per node; otherwise return the
	//  current selected action 

	// should be NO_OP, otherwise it sends best action every frame for sim_steps_frames!!!!
	if (m_curr_action_duration_left > 0) {
		--m_curr_action_duration_left;
		return m_curr_action;
	}
	std::cout << "Search Agent action selection: frame=" << frame_number
			<< std::endl;
	std::cout << "Is Terminal Before Lookahead? "
			<< m_rom_settings->isTerminal() << std::endl;
	std::cout << "Evaluating actions: " << std::endl;

	float t0 = aptk::time_used();

	m_env->getScreen();
	state = m_env->cloneState();

	if (search_tree->is_built) {
		// Re-use the old tree
		search_tree->move_to_branch(m_curr_action, m_curr_action_duration);
//		search_tree->move_to_best_sub_branch();
		//assert(search_tree->get_root()->state.equals(state));
		if (search_tree->get_root()->state.equals(state)) {
			//assert(search_tree->get_root()->state.equals(state));
			//assert (search_tree->get_root_frame_number() == state.getFrameNumber());
			search_tree->update_tree();

		} else {
			//std::cout << "\n\n\tDIFFERENT STATE!\n" << std::endl;
			search_tree->clear();
			search_tree->build(state);
		}
	} else {
		// Build a new Search-Tree
		search_tree->clear();
		search_tree->build(state);
	}

	m_curr_action = search_tree->get_best_action();

	// TOREFACTOR: these commands should be emplaced within SearchTree.
	search_tree->getJunkActionSequence(frame_number); // TODO: messy
	search_tree->saveUsedAction(frame_number, m_curr_action);

	m_env->restoreState(state);

	std::cout << "Is Terminal After Lookahead? " << m_rom_settings->isTerminal()
			<< std::endl;

	float tf = aptk::time_used();

	float elapsed = tf - t0;

	search_tree->print_frame_data(frame_number, elapsed, m_curr_action,
			m_trace);
	search_tree->print_frame_data(frame_number, elapsed, m_curr_action,
			std::cout);

	int duration = sim_steps_per_node;
	if (erroneous_action) {
		Action m = randomizeAction(m_curr_action);
		if (m != m_curr_action) {
			printf("Randomized from %d to %d\n", (int) m_curr_action, (int) m);
			m_curr_action = m;
			search_tree->set_best_action(m_curr_action);
		}
		if (rand() % 100 < action_error_rate * 2 * 100) {
			if (rand() % 2) {
				duration += 1;
			} else {
				duration -= 1;
			}
			printf("duration = %d\n", duration);
		}
	}
	m_curr_action_duration = duration;
	m_curr_action_duration_left = duration - 1;

	return m_curr_action;
}

/* *********************************************************************
 This method is called when the game ends.
 ******************************************************************** */
void SearchAgent::episode_end(void) {
	PlayerAgent::episode_end();
	// Our search-tree is useless now. Clear it
	search_tree->clear();

	search_tree->getDetectedUsedActionsSize();
}

Action SearchAgent::episode_start(void) {
	Action a = PlayerAgent::episode_start();

	state.incrementFrame();
	m_current_episode++;
	return a;
}

Action SearchAgent::randomizeAction(Action a) {
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

	int error_percent = action_error_rate * 100;

	if (x_axis <= 0) {
		if (error_percent > (rand() % 100)) {
			x_axis += 1;
		}
	} else if (x_axis >= 0) {
		if (error_percent > (rand() % 100)) {
			x_axis -= 1;
		}
	}
	if (y_axis <= 0) {
		if (error_percent > (rand() % 100)) {
			y_axis += 1;
		}
	} else if (y_axis >= 0) {
		if (error_percent > (rand() % 100)) {
			y_axis -= 1;
		}
	}

	if (error_percent > (rand() % 100)) {
		fire = !fire;
	}

	if (x_axis == 0 && y_axis == 0) {
		return (Action) fire;
	} else {
		int bias = 2;
		if (fire) {
			bias = 10;
		}
		if (x_axis == 0 && y_axis == 1) {
			return (Action) bias;
		} else if (x_axis == 1 && y_axis == 0) {
			return (Action) (bias + 1);
		} else if (x_axis == -1 && y_axis == 0) {
			return (Action) (bias + 2);
		} else if (x_axis == 0 && y_axis == -1) {
			return (Action) (bias + 3);
		} else if (x_axis == 1 && y_axis == 1) {
			return (Action) (bias + 4);
		} else if (x_axis == -1 && y_axis == 1) {
			return (Action) (bias + 5);
		} else if (x_axis == 1 && y_axis == -1) {
			return (Action) (bias + 6);
		} else if (x_axis == -1 && y_axis == -1) {
			return (Action) (bias + 7);
		} else {
			assert(false && "randomizeAction error");
			return (Action) 0;
		}
	}
}
