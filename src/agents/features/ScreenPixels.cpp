/*
 * Screen.cpp
 *
 *  Created on: Apr 3, 2017
 *      Author: yuu
 */

#include "ScreenPixels.hpp"

ScreenPixels::ScreenPixels(StellaEnvironment* _env) :
		Features(_env) {
	n_features = _env->getScreen().width() * _env->getScreen().height() * 256;
}

ScreenPixels::~ScreenPixels() {
	// TODO Auto-generated destructor stub
}

void ScreenPixels::getFeatures(const ALEScreen &screen, const ALERAM &ram,
		vector<bool>& features) {
	features.resize(n_features); // TODO: this should be able to optimize out later on.
	features.assign(n_features, false);
	int m_column = screen.width();
	for (size_t i = 0; i < screen.arraySize(); i++) {
		byte_t byte = (byte_t) screen.get(i / m_column, i % m_column);
		features[i * 256 + byte] = true;
	}
}
