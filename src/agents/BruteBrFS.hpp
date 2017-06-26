// BreadthFirstSearch.hpp
//
// Implementation of BreadthFirstSearch

#ifndef __BRUTE_BRFS_HPP__
#define __BRUTE_BRFS_HPP__

#include "SearchTree.hpp"

class BruteBrFS: public SearchTree {

public:

	class BruteComparator {
	public:
		bool operator()(BruteTreeNode* a, BruteTreeNode* b) const {
			return a->max_return < b->max_return;
		}
	};
	BruteBrFS(RomSettings *, Settings &settings, ActionVect &actions,
			StellaEnvironment* _env);

	virtual ~BruteBrFS();

	virtual void build(ALEState & state);

	virtual void update_tree();

	int expanded() const {
		return m_expanded_nodes;
	}
	int generated() const {
		return m_generated_nodes;
	}
	int pruned() const {
		return m_pruned_nodes;
	}
	int jasd_pruned() const {
		return m_jasd_pruned_nodes;
	}

	virtual void print_frame_data(int frame_number, float elapsed,
			Action curr_action, std::ostream& output);
protected:

	void print_path(TreeNode *start, int a);

	virtual void expand_tree(TreeNode* start);

	void update_branch_return(TreeNode* node);

	void set_terminal_root(TreeNode* node);

	virtual void clear();
	virtual void move_to_best_sub_branch();
	virtual void move_to_branch(Action a, int duration);

	TreeNode* selectNode(
			priority_queue<BruteTreeNode*, vector<BruteTreeNode*>,
					BruteComparator>& queue);
//	void update_values(BruteTreeNode* node, return_t mc_return);

	ALERAM m_ram;
	unsigned m_pruned_nodes;
};

#endif
