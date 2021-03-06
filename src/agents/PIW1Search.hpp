#ifndef __PIW_SEARCH_HPP__
#define __PIW_SEARCH_HPP__

#include "SearchTree.hpp"
#include "bit_matrix.hxx"
#include "../environment/ale_ram.hpp"
#include "features/Features.hpp"

#include <queue> // TODO: Implement priority queue

class PIW1Search: public SearchTree {
public:
	PIW1Search(RomSettings *, Settings &settings, ActionVect &actions,
			StellaEnvironment* _env);

	virtual ~PIW1Search();

	class TreeNodeComparerReward {
	public:
		// TODO: IMPORTANT: Note that std::priority_queue puts the LEAST
		// significant node to the top. Therefore, THE SMALLER THE BETTER, thus accumulated reward should be flipped.
		bool operator()(TreeNode* a, TreeNode* b) const {
			if (b->accumulated_reward > a->accumulated_reward)
				return true;
			else if (b->accumulated_reward == a->accumulated_reward
					&& b->novelty < a->novelty)
				return true;
			return false;
		}
	};

	class TreeNodeComparerAdditiveNovelty {
	public:
		// TODO: IMPORTANT: Note that std::priority_queue puts the LEAST
		// significant node to the top. Therefore, THE SMALLER THE BETTER, thus accumulated reward should be flipped.
		bool operator()(TreeNode* a, TreeNode* b) const {
			if (b->additive_novelty > a->additive_novelty)
				return true;
			else if (b->fn == a->fn
					&& b->accumulated_reward > a->accumulated_reward)
				return true;
			return false;
		}
	};

	// Fn is defined in PIW1Search.cpp. as a function of reward and (additive)novelty.
	class TreeNodeComparerFunction {
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
	int jasd_pruned() const { return m_jasd_pruned_nodes; }

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

	void update_novelty_table(ALEState &machine_state,
			reward_t accumulated_reward);
	bool check_novelty_1(ALEState& machine_state,
			reward_t accumulated_reward);

	int check_novelty(ALEState& machine_state, reward_t accumulated_reward);
	// "check_novelty" returns the number of novel features whereas "check_novelty_1" returns if there is a novel feature or not.
	int calc_fn(ALEState& machine_state, reward_t accumulated_reward);

	virtual void clear();
	virtual void move_to_best_sub_branch();
	virtual void move_to_branch(Action a, int duration);
	bool test_duplicate_reward(TreeNode * node);

	const ALEScreen get_screen(ALEState &machine_state);


	std::priority_queue<TreeNode*, std::vector<TreeNode*>,
			TreeNodeComparerReward> m_q_reward;
	std::priority_queue<TreeNode*, std::vector<TreeNode*>,
			TreeNodeComparerAdditiveNovelty> m_q_novelty;

	ALERAM m_ram;
//	aptk::Bit_Matrix* m_ram_novelty_table; // TODO: replace with reward table.
//	aptk::Bit_Matrix* m_ram_novelty_table_true;
//	aptk::Bit_Matrix* m_ram_novelty_table_false;
	Features* m_novelty_feature;
	std::vector<int> m_novelty_table;
	std::string m_feature;

//	std::vector<int> m_ram_reward_table_true;
//	std::vector<int> m_ram_reward_table_false;
//	std::vector<int> m_ram_reward_table_byte;
//	std::vector<int> m_image_reward_table;
	unsigned m_pruned_nodes;
	bool m_stop_on_first_reward;
	unsigned m_reward_horizon;
	bool m_novelty_boolean_representation;

	bool m_expand_all_emulated_nodes; // True if we do not prune already emulated nodes because these nodes do not require additional computational overhead.

	int m_redundant_ram;


	string m_priority_queue;
};

#endif // __IW_SEARCH_HPP__
