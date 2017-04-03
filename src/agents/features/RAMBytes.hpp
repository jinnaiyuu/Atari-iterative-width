/*
 * RAMBytes.hpp
 *
 *  Created on: Apr 3, 2017
 *      Author: yuu
 */

#ifndef SRC_AGENTS_FEATURES_RAMBYTES_HPP_
#define SRC_AGENTS_FEATURES_RAMBYTES_HPP_

#include "Features.hpp"

class RAMBytes: public Features {
public:
	RAMBytes(StellaEnvironment* _env);
	RAMBytes(StellaEnvironment* _env, int redundant_ram);
	virtual ~RAMBytes();

	void getFeatures(const ALEScreen &screen, const ALERAM &ram,
			vector<bool>& features);


private:
	int redundant_ram;
};

#endif /* SRC_AGENTS_FEATURES_RAMBYTES_HPP_ */
