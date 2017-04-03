/*
 * TFBinary.cpp
 *
 *  Created on: Apr 3, 2017
 *      Author: yuu
 */

#include "TFBinary.hpp"

TFBinary::TFBinary(StellaEnvironment* _env) :
		Features(_env) {
	// TODO Auto-generated constructor stub
	n_features = _env->getRAM().size() * 8 * 2;
}

TFBinary::~TFBinary() {
	// TODO Auto-generated destructor stub
}

void TFBinary::getFeatures(const ALEScreen &screen, const ALERAM &ram,
		vector<bool>& features) {
	features.resize(n_features); // TODO: this should be able to optimize out later on.
	features.assign(n_features, false);
	for (size_t i = 0; i < ram.size(); i++) {
		unsigned char mask = 1;
		byte_t byte = ram.get(i);
		for (int j = 0; j < 8; j++) {
			bool bit_is_set = (byte & (mask << j)) != 0;
			if (bit_is_set) {
				assert(i * 8 + j < features.size());
				features[i * 8 + j] = true;
			} else {
				assert(i * 8 + j + ram.size() * 8 < features.size());
				features[i * 8 + j + ram.size() * 8] = true;
			}
		}
	}
}

