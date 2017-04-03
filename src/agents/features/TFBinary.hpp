/*
 * TFBinary.hpp
 *
 *  Created on: Apr 3, 2017
 *      Author: yuu
 */

#ifndef SRC_AGENTS_FEATURES_TFBINARY_HPP_
#define SRC_AGENTS_FEATURES_TFBINARY_HPP_

#include "Features.hpp"

class TFBinary: public Features {
public:
	TFBinary(StellaEnvironment* _env);
	virtual ~TFBinary();

	void getFeatures(const ALEScreen &screen, const ALERAM &ram,
			vector<bool>& features);


};

#endif /* SRC_AGENTS_FEATURES_TFBINARY_HPP_ */
