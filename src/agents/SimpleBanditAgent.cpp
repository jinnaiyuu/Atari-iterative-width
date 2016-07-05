#include "SimpleBanditAgent.hpp"
#include "random_tools.h"
#include <cstdlib>
#include <cassert>

double	rand_double( ) {

	return (double)rand()/(double)RAND_MAX;

}

SimpleBanditAgent::SimpleBanditAgent( OSystem* _osystem, RomSettings* _settings )
	: PlayerAgent( _osystem, _settings ), m_lookahead_env( _osystem, _settings) {
	m_lookahead_env.reset();
	m_value_table[0] = QValue( available_actions.size() );
	m_log.open( "bandit.log" );
}

SimpleBanditAgent::~SimpleBanditAgent() {
	assert(false);
}

Action	SimpleBanditAgent::act() {
	// initialize lookahead environment
    m_lookahead_env.restoreState(  current_state() );
    std::cout <<  m_lookahead_env.equals_state( current_state() ) ;

	int t = episode_frame_number;

	m_log << "t=" << t << std::endl;

	// Update
	if ( t > 0 ) {
		m_log << "Update:" << std::endl;
		m_log << "Was" << std::endl;
		m_value_table[t-1].write( m_log );
		m_log << std::endl;
		m_value_table[t-1].update( m_last_action, m_last_reward );
		m_log << "And now is" << std::endl;
		m_value_table[t-1].write( m_log );
		m_log << std::endl;
	}

	// Select
	Action selected;
	if ( m_value_table.find(t) == m_value_table.end() )
		m_value_table[t] = QValue( available_actions.size() );

	// epsilon greedy
	double epsilon = 0.25;
	
	if ( rand_double() < epsilon ) {
		selected = choice( &available_actions );
		m_log << "Random action: " << action_to_string(selected) << std::endl;
	}
	else {

		// greedy action selection
		const std::vector<double>& Qa = m_value_table[t].m_Q;
		double bestQ = Qa[0];
		selected = available_actions[0];
		for ( unsigned i = 1; i < Qa.size(); i++ )
			if ( Qa[i] > bestQ ) {
				bestQ = Qa[i];
				selected = available_actions[i];
			}
		m_log << "Greedy action: " << action_to_string(selected) << std::endl;
	} 

	m_last_reward = m_lookahead_env.act( selected , PLAYER_B_NOOP );
	
	m_log << ( m_last_reward > 0 ? "Positive" : ( m_last_reward < 0 ? "Negative" : " Null" ) ) << " Reward: " << m_last_reward << std::endl;

	m_last_action = selected;
	
	return selected;
}
