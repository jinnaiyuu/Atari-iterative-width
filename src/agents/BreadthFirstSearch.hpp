// BreadthFirstSearch.hpp
//
// Implementation of BreadthFirstSearch

#ifndef __BREADTH_FIRST_SEARCH_HPP__
#define __BREADTH_FIRST_SEARCH_HPP__

#include "SearchTree.hpp"

class BreadthFirstSearch : public SearchTree {
	
public:
    	BreadthFirstSearch(RomSettings *, Settings &settings, ActionVect &actions, StellaEnvironment* _env);

	virtual ~BreadthFirstSearch();

	virtual void build(ALEState & state);
		
	virtual void update_tree();


	int expanded() const { return m_expanded_nodes; }
	int generated() const { return m_generated_nodes; }
	int pruned() const { return m_pruned_nodes; }

	virtual	void print_frame_data( int frame_number, float elapsed, Action curr_action, std::ostream& output );
protected:	

	void print_path(TreeNode *start, int a);

	virtual void expand_tree(TreeNode* start);

	void update_branch_return(TreeNode* node);

    	void set_terminal_root(TreeNode* node); 

	virtual void	clear();
	virtual void	move_to_best_sub_branch();
	
	ALERAM 			m_ram;
	unsigned		m_pruned_nodes;
};

#endif
