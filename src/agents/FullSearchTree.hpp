/* *****************************************************************************
 * A.L.E (Arcade Learning Environment)
 * Copyright (c) 2009-2012 by Yavar Naddaf, Joel Veness, Marc G. Bellemare
 * Released under GNU General Public License www.gnu.org/licenses/gpl-3.0.txt
 *
 * Based on: Stella  --  "An Atari 2600 VCS Emulator"
 * Copyright (c) 1995-2007 by Bradford W. Mott and the Stella team
 *
 * *****************************************************************************
 *  FullSearchTree.hpp
 *
 *  Implementation of the FullSearchTree class, which represents search a
 *  Full-Search-Tree (i.e. a search-tree that expands all its nodes until
 *  the given depth), used by the Search Agent
 **************************************************************************** */

#ifndef __FULL_SEARCH_TREE_HPP__
#define __FULL_SEARCH_TREE_HPP__

#include "SearchTree.hpp"
#include "bit_matrix.hxx"
#include "../environment/ale_ram.hpp"


class FullSearchTree : public SearchTree {
    /* *************************************************************************
        Represents search a Full-Search-Tree (i.e. a search-tree that expands 
		all its nodes until the given depth), used by the Search Agent
    ************************************************************************* */
	
    public:
		/* *********************************************************************
            Constructor
			Generates a whole search tree from the current state, until 
			max_frame_num is reached.
         ******************************************************************* */
    FullSearchTree(RomSettings *, Settings &settings, ActionVect &actions, StellaEnvironment* _env);

		/* *********************************************************************
           Destructor 
         ******************************************************************* */
		virtual ~FullSearchTree();

		/* *********************************************************************
            Builds a new tree
         ******************************************************************* */
		virtual void build(ALEState & state);
		
		/* *********************************************************************
			Re-expands the tree until i_max_sim_steps_per_tree is reached
         ******************************************************************* */
		virtual void update_tree();


	int expanded() const { return m_expanded_nodes; }
	int generated() const { return m_generated_nodes; }
	int pruned() const { return m_pruned_nodes; }

	virtual	void print_frame_data( int frame_number, float elapsed, Action curr_action, std::ostream& output );
	protected:	

    /* For debugging purposes */
    void print_path(TreeNode *start, int a);

		/* *********************************************************************
			Expands the tree from the given node until i_max_sim_steps_per_tree
         ******************************************************************* */
		virtual void expand_tree(TreeNode* start);


		/* *********************************************************************
			Updates the branch reward for the given node
			which equals to: node_reward + max(children.branch_reward)
         ******************************************************************* */
		void update_branch_return(TreeNode* node);

    void set_terminal_root(TreeNode* node); 

	void	update_novelty_table( const ALERAM &machine_state );
	bool	check_novelty_1( const ALERAM& machine_state );

	void	update_novelty_table( const ALEScreen& screen );
	bool	check_novelty_1( const ALEScreen& screen );

	virtual void	clear();
	virtual void	move_to_best_sub_branch();
	
	ALERAM 			m_ram;
	aptk::Bit_Matrix*	m_ram_novelty_table;
	aptk::Bit_Matrix*	m_screen_novelty_table;
	unsigned		m_pruned_nodes;


};



#endif // __FULL_SEARCH_TREE_HPP__
