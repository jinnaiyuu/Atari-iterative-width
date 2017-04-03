/*
 * Screen.hpp
 *
 *  Created on: Apr 3, 2017
 *      Author: yuu
 */

#ifndef SRC_AGENTS_FEATURES_SCREENPIXELS_HPP_
#define SRC_AGENTS_FEATURES_SCREENPIXELS_HPP_

#include "Features.hpp"

class ScreenPixels: public Features {
public:
	ScreenPixels(StellaEnvironment* _env);
	virtual ~ScreenPixels();
	void getFeatures(const ALEScreen &screen, const ALERAM &ram,
			vector<bool>& features);
};

#endif /* SRC_AGENTS_FEATURES_SCREENPIXELS_HPP_ */
