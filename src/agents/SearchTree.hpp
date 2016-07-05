/* *****************************************************************************
 * A.L.E (Arcade Learning Environment)
 * Copyright (c) 2009-2012 by Yavar Naddaf, Joel Veness, Marc G. Bellemare
 * Released under GNU General Public License www.gnu.org/licenses/gpl-3.0.txt
 *
 * Based on: Stella  --  "An Atari 2600 VCS Emulator"
 * Copyright (c) 1995-2007 by Bradford W. Mott and the Stella team
 *
 * *****************************************************************************
 *  SearchTree.hpp
 *
 *  Implementation of the SearchTree class, which represents search tree for the
 *  Search-Agent
 **************************************************************************** */

#ifndef __SEARCH_TREE_HPP__
#define __SEARCH_TREE_HPP__

#include <queue>
#include "Constants.h"
#include "TreeNode.hpp"
#include "RomSettings.hpp"
#include "../environment/ale_state.hpp"
#include "../environment/stella_environment.hpp"
#include "Settings.hxx"
#include <fstream>
#include <limits>

class SearchAgent;


class SearchTree {
    /* *************************************************************************
        Represents a search tree for rollout-based agents
    ************************************************************************* */
	
    public:
		/* *********************************************************************
            Constructor
         ******************************************************************* */
               SearchTree(RomSettings *, Settings &, ActionVect &, StellaEnvironment* _env);

		/* *********************************************************************
            Destructor   
         ******************************************************************* */
		virtual ~SearchTree();

		/* *********************************************************************
            Builds a new tree
         ******************************************************************* */
		virtual void build(ALEState &state) = 0;

		/* *********************************************************************
			Updates (re-expands) the tree. 
		 ******************************************************************* */
		virtual void update_tree() = 0;
		
		/* *********************************************************************
            Deletes the search-tree
         ******************************************************************* */
		virtual void clear();
		
		/* *********************************************************************
            Returns the best action based on the expanded search tree
         ******************************************************************* */
		virtual Action get_best_action(void);	

		/* *********************************************************************
			Moves the best sub-branch of the root to be the new root of the tree
         ******************************************************************* */
		virtual void move_to_best_sub_branch(void);

		
		/* *********************************************************************
			Returns the the best branch-value for root
         ******************************************************************* */
		float get_root_value(void) {
			return p_root->v_children[p_root->best_branch]->branch_return;
		}

    bool has_root() { return p_root != NULL; }

    /** Simulates forward for a number of steps, collecting reward & terminal
      *  information along the way. Provides very primitive rollout policy
      *  capabilities (random rollout or fixed action). Note that the state
      *  is not loaded prior to simulation; a call to state.load() might be
      *  in order before simulate_game(state, ...).
      *
      *  random_action_length specifies how frequently to pick a new random
      *   action during simulation.
      *  if discount_return == true, then every 'sim_steps_per_node' we
      *   discount the reward by 'discount_factor'
      */
    int simulate_game(ALEState & state, Action act, int num_steps, 
		      return_t &traj_return, bool &game_ended, bool discount_return = false, bool save_state = true);

    /** Normalizes a reward using the first non-zero reward's magnitude */ 
    return_t normalize(reward_t reward);
    virtual unsigned max_depth(){ return m_max_depth; }
    unsigned expanded_nodes() const { return m_expanded_nodes; }
    unsigned generated_nodes() const { return m_generated_nodes; }
    int num_nodes();

    void set_novelty_pruning(){ m_novelty_pruning = true;}

    bool is_built;			// True whe the tree is built
	
    /** Debugging methods */

    void print_best_path();
    int get_root_frame_number();
    TreeNode *get_root() {return p_root; }

    bool is_player_B() {return m_player_B;}
    void set_player_B( bool b ) { m_player_B = b; }
    void set_available_actions( ActionVect acts ){ available_actions = acts;}
    /** Returns the number of simulation steps used since the last call to 
      *  this function. */
    long num_simulation_steps();

	virtual	void print_frame_data( int frame_number, float elapsed, Action curr_action, std::ostream& output );

	protected:	


		/* *********************************************************************
			Deletes a node and all its children, all the way down the branch
         ******************************************************************* */
		void delete_branch(TreeNode* node);

    /** Returns true if this node has a sibling with the same resulting state;
      *  also sets the node's duplicate flag to true in that case. */
    bool test_duplicate(TreeNode * node);

  protected:

    // Root of the SearchTree
		TreeNode* p_root;		
    // Number of steps we will run the simulation in each search-tree node
		int sim_steps_per_node;	
    // Maximum number of simulation steps that will be carried to build 
    //   the tree on one frame
		int max_sim_steps_per_frame;
    // The number of UCT simulations (trajectories) we require per step
    //  when the tree is used fewer simulations need to be performed 
    int num_simulations_per_frame;
    // Discount factor to force the tree prefer closer goals
		float discount_factor; 
    // Whether to normalize the simulated rewards using the magnitude of the 
    //  first non-zero reward we see
    bool normalize_rewards;
    // Whether to ignore duplicate nodes when searching
    bool ignore_duplicates;
    return_t reward_magnitude;

    RomSettings * m_rom_settings;

    ActionVect  available_actions;

    // Counting how many simulation steps we have used since the last call
    //   to get_num_simulation_steps() 
    long total_simulation_steps;
    
    StellaEnvironment* m_env;

    unsigned m_expanded_nodes;
    unsigned m_generated_nodes;
    unsigned m_max_depth;

    unsigned m_reused_nodes;

    bool   m_novelty_pruning;
    bool             m_player_B;    
    bool             m_randomize_successor;
};

#endif // __SEARCH_TREE_HPP__
