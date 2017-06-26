/* *****************************************************************************
 * A.L.E (Arcade Learning Environment)
 * Copyright (c) 2009-2012 by Yavar Naddaf, Joel Veness, Marc G. Bellemare
 * Released under GNU General Public License www.gnu.org/licenses/gpl-3.0.txt
 *
 * Based on: Stella  --  "An Atari 2600 VCS Emulator"
 * Copyright (c) 1995-2007 by Bradford W. Mott and the Stella team
 *
 * *****************************************************************************
 *  UCTSearchTree.hpp
 *
 *  A subclass of SearchTree that implements UCT. 
 **************************************************************************** */

#ifndef __BRUTE_HPP__
#define __BRUTE_HPP__

#include "SearchTree.hpp"
#include "RomSettings.hpp"
#include "BruteTreeNode.hpp"
#include "Settings.hxx"
#include "Constants.h"

class Brute: public SearchTree {
public:
	/* *********************************************************************
	 Constructor
	 ******************************************************************* */
	Brute(RomSettings *, Settings &settings, ActionVect &actions,
			StellaEnvironment* _env);

	class BruteComparator {
	public:
		bool operator()(TreeNode* a, TreeNode* b) const {
			return ((BruteTreeNode *) a)->max_return
					< ((BruteTreeNode *) b)->max_return;
		}
	};

	/* *********************************************************************
	 Destructor
	 ******************************************************************* */
	virtual ~Brute();

	/* *********************************************************************
	 Builds a new tree
	 ******************************************************************* */
	virtual void build(ALEState & state);

	/* *********************************************************************
	 Re-Expands the tree until i_max_sim_steps_per_tree is reached
	 ******************************************************************* */
	virtual void update_tree(void);

	/* *********************************************************************
	 Returns the best action based on the expanded search tree
	 ******************************************************************* */
	virtual Action get_best_action(void);
	virtual void move_to_branch(Action a, int duration);

protected:

	/* *********************************************************************
	 Performs a single UCT iteration, starting from the root. Returns
	 how many simulation steps were used.
	 ******************************************************************* */
	virtual int single_brute_iteration(void);

	/* *********************************************************************
	 Returns the index of the first child with zero count
	 Returns -1 if no such child is found
	 ******************************************************************* */
	int get_child_with_count_zero(const TreeNode* node) const;

	int choose_child(const BruteTreeNode* node) const;

	/* *********************************************************************
	 Returns the sub-branch with the highest value if add_uct_bias is true
	 we will add the UCT bonus to each branch value and then take the max
	 ******************************************************************* */
	int get_best_branch(BruteTreeNode* node, bool add_uct_bias);

	/** Most visited branch; used as an alternate action selection method */
	int get_most_visited_branch(BruteTreeNode * node);

	/* *********************************************************************
	 Expands the given node, by generating all its children. The children
	 are not simulated.
	 ******************************************************************* */
	void expand_node(TreeNode* node);

	/* *********************************************************************
	 Update the node values and counters from the current node, all the
	 way up to the root
	 ******************************************************************* */
	void update_values(BruteTreeNode* node, return_t mc_return);

	/** For debugging purposes */
	void print_path(TreeNode * node, int c);

	/** Print the structure of the tree */
	void print_tree();
	void print_node(BruteTreeNode* node);

	std::vector<Action> getEffectiveActionsVector(const TreeNode* node) const;

	float epsilon;
};

#endif // __UCT_SEARCH_TREE_HPP__
