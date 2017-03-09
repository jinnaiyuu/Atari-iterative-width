#ifndef __SITE_PERCOLATION_HPP__
#define __SITE_PERCOLATION_HPP__

#include "SearchTree.hpp"
#include "bit_matrix.hxx"
#include "../environment/ale_ram.hpp"

#include <queue> // TODO: Implement priority queue

class SitePercolation: public SearchTree {
public:
	SitePercolation(RomSettings *, Settings &settings, ActionVect &actions,
			StellaEnvironment* _env);

	virtual ~SitePercolation();

	// node->fn is defined in calc_fn: it returns random number.
	class SiteIPPriority {
	public:
		bool operator()(TreeNode* a, TreeNode* b) const {
			if (b->fn < a->fn)
				return true;
			else
				return false;
		}
	};

	virtual void build(ALEState & state);

	virtual void update_tree();
//	virtual int expand_node(TreeNode* n, queue<TreeNode*>& q);
	virtual int expand_node(TreeNode* n);
//	virtual int expand_node(TreeNode* n,
//			std::priority_queue<TreeNode*, std::vector<TreeNode*>,
//					TreeNodeComparerReward>& q);

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

	virtual unsigned max_depth() {
		for (size_t c = 0; c < p_root->v_children.size(); c++)
			if (m_max_depth < p_root->v_children[c]->branch_depth)
				m_max_depth = p_root->v_children[c]->branch_depth;

		return m_max_depth;
	}
	virtual void print_frame_data(int frame_number, float elapsed,
			Action curr_action, std::ostream& output);

protected:

	void print_path(TreeNode *start, int a);

	virtual void expand_tree(TreeNode* start);

	void update_branch_return(TreeNode* node);

	void set_terminal_root(TreeNode* node);

	int calc_fn(const ALERAM& machine_state, reward_t accumulated_reward);

	virtual void clear();
	virtual void move_to_best_sub_branch();

	std::priority_queue<TreeNode*, std::vector<TreeNode*>, SiteIPPriority> m_q_percolation;

	ALERAM m_ram;
//	aptk::Bit_Matrix* m_ram_novelty_table; // TODO: replace with reward table.
//	aptk::Bit_Matrix* m_ram_novelty_table_true;
//	aptk::Bit_Matrix* m_ram_novelty_table_false;

	unsigned m_pruned_nodes;
	bool m_stop_on_first_reward;
	unsigned m_reward_horizon;
	bool m_novelty_boolean_representation;

	bool m_expand_all_emulated_nodes; // True if we do not prune already emulated nodes because these nodes do not require additional computational overhead.
//	bool test_duplicate_reward(TreeNode * node);
};

#endif // __IW_SEARCH_HPP__
