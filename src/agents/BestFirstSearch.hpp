#ifndef __BEST_FIRST_SEARCH_HPP__
#define __BEST_FIRST_SEARCH_HPP__

#include "IW1Search.hpp"

class BestFirstSearch : public IW1Search {
public:
    BestFirstSearch(RomSettings *, Settings &settings, ActionVect &actions, StellaEnvironment* _env);

	virtual ~BestFirstSearch();


    class TreeNodeComparerExploration
    {
    public:
	
	bool operator()( TreeNode* a, TreeNode* b ) const 
	{
		if ( b->novelty < a->novelty ) return true;
		else if( b->novelty == a->novelty && b->fn < a->fn ) return true;
		return false;
	}
    };


    class TreeNodeComparerExploitation
    {
    public:
	
	bool operator()( TreeNode* a, TreeNode* b ) const 
	{
	    if ( b->fn < a->fn ) return true;
	    else if( b->fn == a->fn && b->novelty < a->novelty ) return true;
	    return false;
	}
    };

    virtual int  expand_node( TreeNode* n ); 

    void clear_queues(){
	    delete q_exploration;
	    delete q_exploitation;
	    q_exploration = new std::priority_queue<TreeNode*, std::vector<TreeNode*>, TreeNodeComparerExploration >;
	    q_exploitation = new std::priority_queue<TreeNode*, std::vector<TreeNode*>, TreeNodeComparerExploitation >;
    }

protected:	

    void reset_branch(TreeNode* node);
    int  reuse_branch(TreeNode* node);
    unsigned size_branch(TreeNode* node);

    virtual void expand_tree(TreeNode* start);

    std::priority_queue<TreeNode*, std::vector<TreeNode*>, TreeNodeComparerExploration >* q_exploration;
    std::priority_queue<TreeNode*, std::vector<TreeNode*>, TreeNodeComparerExploitation >* q_exploitation;

    reward_t		m_max_reward;
    unsigned m_gen_count_novelty2;
    unsigned m_gen_count_novelty1;
    unsigned m_exp_count_novelty2;
    unsigned m_exp_count_novelty1;
};



#endif // __IW_DIJKSTRA_SEARCH_HPP__
