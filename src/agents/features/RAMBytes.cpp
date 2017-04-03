/*
 * RAMBytes.cpp
 *
 *  Created on: Apr 3, 2017
 *      Author: yuu
 */

#include "RAMBytes.hpp"

RAMBytes::RAMBytes(StellaEnvironment* _env) :
		Features(_env), redundant_ram(0) {
	n_features = _env->getRAM().size() * 256;
}

RAMBytes::RAMBytes(StellaEnvironment* _env, int redundant_ram) :
		Features(_env), redundant_ram(redundant_ram) {
	n_features = _env->getRAM().size() * 256 * (1 + redundant_ram);
}

RAMBytes::~RAMBytes() {
	// TODO Auto-generated destructor stub
}

void RAMBytes::getFeatures(const ALEScreen &screen, const ALERAM &ram,
		vector<bool>& features) {
	features.resize(n_features); // TODO: this should be able to optimize out later on.
	features.assign(n_features, false);
	for (size_t i = 0; i < ram.size(); i++) {
		byte_t byte = ram.get(i);
		features[i * 256 + byte] = true;
	}

	for (int j = 0; j < redundant_ram; ++j) {
		for (size_t i = 0; i < ram.size(); i++) {
			byte_t byte = ram.get(i) ^ ram.get((i + j) % ram.size());
			assert((j * ram.size() + i) * 256 + byte < features.size());
			features[(j * ram.size() + i) * 256 + byte] = true;
		}
	}
}

