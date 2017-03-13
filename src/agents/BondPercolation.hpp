#ifndef __BOND_PERCOLATION_HPP__
#define __BOND_PERCOLATION_HPP__

#include "SearchTree.hpp"
#include "bit_matrix.hxx"
#include "../environment/ale_ram.hpp"

#include <queue> // TODO: Implement priority queue
#include <tuple>

class BondPercolation: public SearchTree {
public:
	BondPercolation(RomSettings *, Settings &settings, ActionVect &actions,
			StellaEnvironment* _env);

	virtual ~BondPercolation();

	struct TreeNodeExp {
		TreeNode* node;
		int priority;
		int action;
		TreeNodeExp(TreeNode* n, int p, int a) {
			node = n;
			priority = p;
			action = a;
		}
	};

	class Comparator {
	public:
		virtual bool operator()(TreeNodeExp* a, TreeNodeExp* b) const = 0;
	};

	class ListComparator: public Comparator {
	public:
		std::vector<Comparator*> comps;
		bool operator()(TreeNodeExp* a, TreeNodeExp* b) const {
			for (int i = 0; i < comps.size(); ++i) {
				// !comps && !comps -> a and b are tied!
				if (!comps[i]->operator ()(a, b)
						&& !comps[i]->operator ()(b, a)) {
					continue;
				} else {
					return comps[i]->operator ()(a, b);
				}
			}
			return true;
		}
	};

	// node->fn is defined in calc_fn: it returns random number.
	class BondIPPrioirty: public Comparator {
	public:
		bool operator()(TreeNodeExp* a, TreeNodeExp* b) const {
			if (a->priority < b->priority)
				return true;
			else
				return false;
		}
	};

	class DepthPriority: public Comparator {
	public:
		// TODO: Refactor using func call
		bool operator()(TreeNodeExp* a, TreeNodeExp* b) const {
			if (a->node->depth() > b->node->depth()) {
				return true;
			} else {
				return false;
			}
		}
	};

	class NoveltyPriority: public Comparator {
	public:
		// TODO: Refactor using func call
		bool operator()(TreeNodeExp* a, TreeNodeExp* b) const {
			if (a->node->novelty < b->node->novelty) {
				return true;
			} else {
				return false;
			}
		}
	};
	virtual void build(ALEState & state);

	virtual void update_tree();
//	virtual int expand_node(TreeNode* n, queue<TreeNode*>& q);
	int expand_node(TreeNode* n, int action);
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

	virtual int expand_node(TreeNode* curr_node);
	void update_branch_return(TreeNode* node);

	void set_terminal_root(TreeNode* node);

	virtual void clear();
	virtual void move_to_best_sub_branch();

	void update_novelty_table(ALEState &machine_state);
	bool check_novelty_1(ALEState &machine_state);
	const ALEScreen get_screen(ALEState &machine_state);

	std::priority_queue<TreeNodeExp*, std::vector<TreeNodeExp*>, ListComparator>* m_q_percolation;

	ALERAM m_ram;
//	aptk::Bit_Matrix* m_ram_novelty_table; // TODO: replace with reward table.
//	aptk::Bit_Matrix* m_ram_novelty_table_true;
//	aptk::Bit_Matrix* m_ram_novelty_table_false;

	unsigned m_pruned_nodes;
	bool m_stop_on_first_reward;
	unsigned m_reward_horizon;
	bool m_novelty_boolean_representation;

	bool m_expand_all_emulated_nodes; // True if we do not prune already emulated nodes because these nodes do not require additional computational overhead.

	std::vector<std::string> ties;

	// Novelty
	aptk::Bit_Matrix* m_ram_novelty_table;
	aptk::Bit_Matrix* m_ram_novelty_table_true;
	aptk::Bit_Matrix* m_ram_novelty_table_false;
	aptk::Bit_Matrix* m_image_novelty_table;
};

#endif // __IW_SEARCH_HPP__
