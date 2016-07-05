 /* *****************************************************************************
 *  RandomAgent.hpp
 *
 * The implementation of the RandomAgent class.
 **************************************************************************** */

#ifndef __SIMPLE_BANDIT_AGENT__
#define __SIMPLE_BANDIT_AGENT__

#include "../common/Constants.h"
#include "PlayerAgent.hpp"
#include "../emucore/OSystem.hxx"
#include "../environment/ale_state.hpp"
#include "../environment/stella_environment.hpp"
#include <vector>
#include <map>
#include <fstream>

class SimpleBanditAgent : public PlayerAgent {
public:
	SimpleBanditAgent( OSystem* _osystem, RomSettings* _settings );
	~SimpleBanditAgent();	
	class QValue {
	public:
		QValue() {}

		QValue( unsigned num_act )  {
			m_Q.resize( num_act );
			m_k.resize( num_act );
			for ( unsigned i = 0; i < m_Q.size(); i++ )
				m_Q[i] = m_k[i] = 0.0;
		}

		QValue( const QValue& other ) {
			m_Q = other.m_Q;
			m_k = other.m_k;
		}

		QValue( QValue&& other ) {
			m_Q = std::move( other.m_Q );
			m_k = std::move( other.m_k );
		}

		QValue&	operator=( const QValue& other ) {
			m_Q = other.m_Q;
			m_k = other.m_k;	
			return *this;
		}

	
		void update( Action a, reward_t r ) {
			m_Q[a] += (1.0/(m_k[a]+1.0))*((double)r - m_Q[a]);
			m_k[a]+=1.0;
		}
	
		void	write( std::ostream& os ) const {
			for ( unsigned k = 0; k < m_Q.size(); k++ ) {
				os << "(a=" << k << ",k=" << m_k[k] << ",Q_a=" << m_Q[k] << ")";
				if ( k < m_Q.size() -1 )
					os << ", ";
			}
		}

		std::vector< double >		m_Q;
		std::vector< double >		m_k;
	};

protected:

	virtual Action act();

protected:

	StellaEnvironment		m_lookahead_env;
	std::map< int, QValue >		m_value_table;
	Action				m_last_action;
	reward_t			m_last_reward;
	std::ofstream			m_log;
};

#endif // SimpleBanditAgent.hpp

