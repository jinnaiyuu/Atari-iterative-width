/****************************************************************************************
 ** Implementation of Basic Features, described in details in the paper below.
 **       "The Arcade Learning Environment: An Evaluation Platform for General Agents.
 **        Marc G. Bellemare, Yavar Naddaf, Joel Veness, and Michael Bowling.
 **        Journal of Artificial Intelligence Research, 47:253–279, 2013."
 **
 ** The idea is to divide the screen in tiles and to answer, for each tile, if one of the
 ** n colors defined are present in that tile.
 **
 ** Author: Marlos C. Machado
 ***************************************************************************************/

#ifndef BASIC_FEATURES_H
#define BASIC_FEATURES_H
#include "BasicFeatures.hpp"
#endif

BasicFeatures::BasicFeatures(RomSettings *rom_settings, Settings &settings,
		ActionVect &actions, StellaEnvironment* _env) :
		Features(_env) {
	m_env = _env;
	screen_f_n_columns = settings.getInt("tile_columns", false);
	screen_f_n_rows = settings.getInt("tile_rows", false);
	screen_f_n_colors = settings.getInt("tile_colors", false);
	n_features = screen_f_n_columns * screen_f_n_rows * screen_f_n_colors;
	getSubstractBackground = settings.getBool("get_background", false);
	if (getSubstractBackground) {
		this->background = new Background(rom_settings, settings, actions,
				_env);
	}
}

BasicFeatures::~BasicFeatures() {
	if (getSubstractBackground) {
		delete this->background;
	}
}

void BasicFeatures::getFeatures(const ALEScreen &screen, const ALERAM &ram,
		vector<bool>& features) {
	vector<vector<int> > img;

	features.resize(n_features);
	features.assign(n_features, false);

	for (unsigned int i = 0; i < screen.height(); i++) {
		img.push_back(vector<int>(screen.width(), 0));
		for (unsigned int j = 0; j < screen.width(); j++) {
			img[i][j] = screen.get(i, j);
		}
	}

//	assert(features.size() == 0); //If the vector is not empty this can be a mess

	int screenHeight = img.size();
	int screenWidth = img[0].size();
	//The width and height of the screen are expanded to avoid mistakes due to boundaries:
	int expandedHeight =
			screenHeight % screen_f_n_rows ?
					screen_f_n_rows * (screenHeight / screen_f_n_rows + 1) :
					screenHeight;
	int expandedWidth =
			screenWidth % screen_f_n_columns ?
					screen_f_n_columns
							* (screenWidth / screen_f_n_columns + 1) :
					screenWidth;
	//Get number of pixels that define a tile, horizontally and vertically:
	int numColumnPixelsInTile = expandedHeight / screen_f_n_rows;
	int numRowPixelsInTile = expandedWidth / screen_f_n_columns;

	//Before generating features we must check whether we can subtract the background:
	if (getSubstractBackground) {
		unsigned int sizeBackground = this->background->getWidth()
				* this->background->getHeight();
		assert(sizeBackground == screen.width() * screen.height());
	}

	int blockIndex = 0;
	//Iterate over the tiles:
	for (int r = 0; r < screen_f_n_rows; r++) {
		int firstPositionRow = r * numColumnPixelsInTile;
		int lastPositionRow = (r + 1) * numColumnPixelsInTile;
		for (int c = 0; c < screen_f_n_columns; c++) {
			vector<bool> hasColor(screen_f_n_colors, false); //It is a bool vector because these features are binaries
			int firstPositionCol = c * numRowPixelsInTile;
			int lastPositionCol = (c + 1) * numRowPixelsInTile;
			//Now that we know the limits for the tile we iterate over all
			//pixels to verify whether each of the colors are present:
			for (int x = firstPositionCol; x < lastPositionCol; x++) {
				for (int y = firstPositionRow; y < lastPositionRow; y++) {
					if (x < screenWidth && y < screenHeight) {
						unsigned char pixel = img[y][x];
						if (screen_f_n_colors <= 9) { //SECAM, considering only 8 colors
							if (!getSubstractBackground
									|| (this->background->getPixel(y, x) >> 4)
											!= (int) (pixel >> 4)) {
								hasColor[(pixel) >> 4] = true;
							}
						} else { //NTSC, considering 128 colors
							if (!getSubstractBackground
									|| this->background->getPixel(y, x) >> 1
											!= (int) (pixel >> 1)) {
								hasColor[(int) (pixel) >> 1] = true;

							}
						}
					}
				}
			}
			//Putting the numColors bits in the feature vector, one for each color for the current time:
			//TODO: This may be optimized by a Hashing function in place of a vector, may be done in the future.
			for (int color = 0; color < screen_f_n_colors; color++) {
				if (hasColor[color]) {
					features[color + blockIndex] = true;
//                    features.push_back(color + blockIndex);
				}
			}
			blockIndex += screen_f_n_colors;
		}
	}

//	printf("features: ");
//	for (int i = 0; i < n_features; ++i) {
//		if (features[i]) {
//			printf("o");
//		} else {
//			printf("x");
//		}
//	}
//	printf("\n");
	//Bias
	// TODO: Not sure what it is.
//	features.push_back(
//			screen_f_n_columns * screen_f_n_rows * screen_f_n_colors);
}

/* This method was adapted from Sriram Srinivasan's code */
//void BasicFeatures::getActiveFeaturesIndices(const ALEScreen &screen,
//		const ALERAM &ram, vector<int>& features) {
//	vector<vector<int> > img;
//
//	for (unsigned int i = 0; i < screen.height(); i++) {
//		img.push_back(vector<int>(screen.width(), 0));
//		for (unsigned int j = 0; j < screen.width(); j++) {
//			img[i][j] = screen.get(i, j);
//		}
//	}
//
//	assert(features.size() == 0); //If the vector is not empty this can be a mess
//
//	int screenHeight = img.size();
//	int screenWidth = img[0].size();
//	//The width and height of the screen are expanded to avoid mistakes due to boundaries:
//	int expandedHeight =
//			screenHeight % screen_f_n_rows ?
//					screen_f_n_rows * (screenHeight / screen_f_n_rows + 1) :
//					screenHeight;
//	int expandedWidth =
//			screenWidth % screen_f_n_columns ?
//					screen_f_n_columns
//							* (screenWidth / screen_f_n_columns + 1) :
//					screenWidth;
//	//Get number of pixels that define a tile, horizontally and vertically:
//	int numColumnPixelsInTile = expandedHeight / screen_f_n_rows;
//	int numRowPixelsInTile = expandedWidth / screen_f_n_columns;
//
//	//Before generating features we must check whether we can subtract the background:
//	if (getSubstractBackground) {
//		unsigned int sizeBackground = this->background->getWidth()
//				* this->background->getHeight();
//		assert(sizeBackground == screen.width() * screen.height());
//	}
//
//	int blockIndex = 0;
//	//Iterate over the tiles:
//	for (int r = 0; r < screen_f_n_rows; r++) {
//		int firstPositionRow = r * numColumnPixelsInTile;
//		int lastPositionRow = (r + 1) * numColumnPixelsInTile;
//		for (int c = 0; c < screen_f_n_columns; c++) {
//			vector<bool> hasColor(screen_f_n_colors, false); //It is a bool vector because these features are binaries
//			int firstPositionCol = c * numRowPixelsInTile;
//			int lastPositionCol = (c + 1) * numRowPixelsInTile;
//			//Now that we know the limits for the tile we iterate over all
//			//pixels to verify whether each of the colors are present:
//			for (int x = firstPositionCol; x < lastPositionCol; x++) {
//				for (int y = firstPositionRow; y < lastPositionRow; y++) {
//					if (x < screenWidth && y < screenHeight) {
//						unsigned char pixel = img[y][x];
//						if (screen_f_n_colors <= 9) { //SECAM, considering only 8 colors
//							if (!getSubstractBackground
//									|| (this->background->getPixel(y, x) >> 4)
//											!= (int) (pixel >> 4)) {
//								hasColor[(pixel) >> 4] = true;
//							}
//						} else { //NTSC, considering 128 colors
//							if (!getSubstractBackground
//									|| this->background->getPixel(y, x) >> 1
//											!= (int) (pixel >> 1)) {
//								hasColor[(int) (pixel) >> 1] = true;
//
//							}
//						}
//					}
//				}
//			}
//			//Putting the numColors bits in the feature vector, one for each color for the current time:
//			//TODO: This may be optimized by a Hashing function in place of a vector, may be done in the future.
//			for (int color = 0; color < screen_f_n_colors; color++) {
//				if (hasColor[color]) {
//					features.push_back(color + blockIndex);
//				}
//			}
//			blockIndex += screen_f_n_colors;
//		}
//	}
//	//Bias
//	features.push_back(
//			screen_f_n_columns * screen_f_n_rows * screen_f_n_colors);
//}
//
