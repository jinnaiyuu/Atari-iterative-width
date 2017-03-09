/* *****************************************************************************
 * A.L.E (Arcade Learning Environment)
 * Copyright (c) 2009-2013 by Yavar Naddaf, Joel Veness, Marc G. Bellemare and 
 *   the Reinforcement Learning and Artificial Intelligence Laboratory
 * Released under the GNU General Public License; see License.txt for details. 
 *
 * Based on: Stella  --  "An Atari 2600 VCS Emulator"
 * Copyright (c) 1995-2007 by Bradford W. Mott and the Stella team
 *
 * *****************************************************************************
 *  PlayerAgent.cpp
 *
 * The implementation of the PlayerAgent abstract class
 **************************************************************************** */
#include <sstream>
#include "PlayerAgent.hpp"
#include "SearchAgent.hpp"
#include "RandomAgent.hpp"
#include "SingleActionAgent.hpp"
#include "SDLKeyboardAgent.hpp"
#include "SimpleBanditAgent.hpp"

/* **********************************************************************
 Constructor
 ********************************************************************* */
PlayerAgent::PlayerAgent(OSystem* _osystem, RomSettings* _settings) :
		p_osystem(_osystem), p_rom_settings(_settings), frame_number(0), episode_frame_number(
				0), episode_number(0), available_actions(
				_settings->getMinimalActionSet()), m_has_terminated(false), curr_state(
				NULL) {
	Settings& settings = p_osystem->settings();

	i_max_num_episodes = settings.getInt("max_num_episodes", true);
	i_max_num_frames = settings.getInt("max_num_frames", true);
	i_max_num_frames_per_episode = settings.getInt("max_num_frames_per_episode",
			true);

	// By default this is false
	record_trajectory = settings.getBool("record_trajectory", false);

	// Default: false (not currently implemented)
	bool use_restricted_action_set = settings.getBool("restricted_action_set",
			false);

	m_alg_name = settings.getString("search_method", false);

	string rom_file = settings.getString("rom_file", false);
	size_t pos = 0;
	std::string delimiter = "/";
	while ((pos = rom_file.find(delimiter)) != std::string::npos) {
		m_rom_name = rom_file.substr(0, pos);
		rom_file.erase(0, pos + delimiter.length());
	}
	delimiter = ".";
	pos = rom_file.find(delimiter);
	m_rom_name = rom_file.substr(0, pos);

	if (!use_restricted_action_set)
		available_actions = _settings->getAllActions();

	extended_action_set = settings.getInt("extended_action_set", false);

	if (extended_action_set <= 0) {
		extended_action_set = 0;
	} else if (extended_action_set == 1) {
		// PLAYSTATION CONTROLLER = 9x9x2^8 = 9x2^7
		// 2 8-way directional button
		// 4 buttons (o, x,...)
		// 4 LR buttons
		vector<Action> playstation_actions = available_actions;
		for (int i = 0; i < 3; ++i) {
			playstation_actions.insert(playstation_actions.end(), available_actions.begin(), available_actions.end());
		}
		available_actions = playstation_actions;
	} else if (extended_action_set == 2) {
		vector<Action> simple_set(available_actions.begin(), available_actions.begin() + 6);
		available_actions = simple_set;
	} else {

	}


	m_player_B = false;
}

/* **********************************************************************
 Destructor
 ********************************************************************* */
PlayerAgent::~PlayerAgent() {
}

void PlayerAgent::update_state(ALEState *s) {
	if (curr_state)
		delete curr_state;
	curr_state = s;
}

/** This methods handles the basic agent functionality: bookeeping
 *  the number of episodes, frames, etc... It calls the method
 *  act(), which will provide it with an action. act() which should
 *  be overriden by subclasses.
 */
Action PlayerAgent::agent_step() {
	// Terminate if we have a maximum number of frames
	if (i_max_num_frames > 0 && frame_number >= i_max_num_frames) {
		std::cout << "End of game called " << frame_number << ">="
				<< i_max_num_frames << std::endl;
		p_rom_settings->print(std::cout);
		std::cout << std::endl;

		end_game();
	}
	// Terminate this episode if we have reached the max. number of frames
	if (i_max_num_frames_per_episode > 0
			&& episode_frame_number >= i_max_num_frames_per_episode) {
		std::cout << "RESET " << episode_frame_number << ">="
				<< i_max_num_frames_per_episode << std::endl;
		p_rom_settings->print(std::cout);
		std::cout << std::endl;
		return RESET;
	}

	// Only take an action if manual control is disabled
	Action a;
	a = act();

	if (record_trajectory)
		record_action(a);

	frame_number++;
	episode_frame_number++;

	return a;
}

/* *********************************************************************
 This method is called when the game ends. The superclass
 implementation takes care of counting number of episodes, and
 saving the reward history. Any other update should be taken care of
 in the derived classes
 ******************************************************************** */
void PlayerAgent::episode_end(void) {
	episode_number++;

	if (i_max_num_episodes > 0 && episode_number >= i_max_num_episodes)
		end_game();
}

Action PlayerAgent::episode_start(void) {
	episode_frame_number = 0;

	Action a = act();
	if (record_trajectory)
		record_action(a);

	frame_number++;
	episode_frame_number++;

	return a;
}

void PlayerAgent::record_action(Action a) {
	if (episode_number == 0) {
		action_trajectory.push_back(a);
		state_trajectory.push_back(
				new ALEState(*curr_state, curr_state->serialized()));
	}
}

void PlayerAgent::end_game() {
	// Post-processing
	if (record_trajectory && episode_number == 1) {

		std::stringstream filename;
		filename << "state_trajectory_" << m_alg_name << "_" << m_rom_name
				<< "_episode." << episode_number;

		std::ofstream output(filename.str().c_str());

		for (size_t i = 0; i < state_trajectory.size(); i++) {
			output << state_trajectory[i]->serialized() << "<endstate>";
			delete state_trajectory[i];
			state_trajectory[i] = NULL;
		}
		output.close();

		cout << "\n";
		cout << "Action Trajectory ";
		for (size_t i = 0; i < action_trajectory.size(); i++) {
			cout << action_trajectory[i] << " ";
		}
		cout << "\n";
	}

	state_trajectory.clear();
	action_trajectory.clear();
	m_has_terminated = true;
}

bool PlayerAgent::has_terminated() {
	return m_has_terminated;
}

/* *********************************************************************
 Generates an instance of one of the PlayerAgent subclasses, based on
 "player_agent" value in the settings.
 Returns a NULL pointer if the value of player_agent is invalid.
 Note 1: If you add a new player_agent subclass, you need to also
 add it here
 Note 2: The caller is responsible for deleting the returned pointer
 ******************************************************************** */
PlayerAgent* PlayerAgent::generate_agent_instance(OSystem* _osystem,
		RomSettings * _settings, StellaEnvironment* _env, bool player_B) {
	string player_agent = _osystem->settings().getString("player_agent");
	PlayerAgent* new_agent = NULL;

	if (player_agent == "search_agent")
		new_agent = new SearchAgent(_osystem, _settings, _env, player_B);
	else if (player_agent == "random_agent")
		new_agent = new RandomAgent(_osystem, _settings);
	else if (player_agent == "single_action_agent")
		new_agent = new SingleActionAgent(_osystem, _settings);
	else if (player_agent == "keyboard_agent")
		new_agent = new SDLKeyboardAgent(_osystem, _settings);
	else if (player_agent == "simple_bandit")
		new_agent = new SimpleBanditAgent(_osystem, _settings);
	else {
		std::cerr << "Invalid agent type requested: " << player_agent
				<< ". Terminating." << std::endl;
		// We can't play without any agent, so exit now.
		exit(-1);
	}

	if (player_B) {
		new_agent->available_actions = _settings->getAllActions_B();
		new_agent->m_player_B = true;
	}

	return new_agent;
}

