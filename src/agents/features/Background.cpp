/****************************************************************************************
 ** This class is used to store the background, which may be removed from features.
 **
 ** Author: Marlos C. Machado
 ***************************************************************************************/

#ifndef BACKGROUND_H
#define BACKGROUND_H
#include "Background.hpp"
#endif
#include <fstream>
#include <stdlib.h>

Background::Background() {
}

Background::Background(RomSettings *rom_settings, Settings &settings,
		ActionVect &actions, StellaEnvironment* _env) {
	std::string line;
	std::string token;
	std::string delimiter = ",";
	int row = 0;
	int col = 0;
	//TODO: Make this code more robust
	int ratio = 2;

	std::string bgpath = settings.getString("bgpath", true) + rom_settings->rom() + ".bg";
//	bgpath = bgpath + rom_settings->rom();
	printf("bgpath = %s\n", bgpath.c_str());
	//Open background file
	std::ifstream backgroundFile(bgpath.c_str());

	if (backgroundFile.is_open()) {
		//First read the matrix dimensions and allocate background
		getline(backgroundFile, line);

		size_t pos = 0;
		//I assume the first line is the height x width

		pos = line.find(delimiter);
		this->width = atoi(line.substr(0, pos).c_str());
		line.erase(0, pos + delimiter.length());
		this->height = atoi(line.c_str());

		//Dynamically allocate the background matrix
		for (int i = 0; i < height; i++) {
			background.push_back(std::vector<int>(width, 0));
		}

		//Read file line by line, adding parsed elements to matrix:
		while (getline(backgroundFile, line)) {
			if (line.length() > 0) {
				col = 0;
				while ((pos = line.find(delimiter)) != std::string::npos) {
					this->background[row][col] = atoi(
							line.substr(0, pos).c_str());
					line.erase(0, pos + 1);
					col++;
				}
				this->background[row][col] = atoi(line.c_str());
				row++;
			}
		}
		backgroundFile.close();
	} else {
		printf("backgroundFile not open\n");
	}
}

int Background::getPixel(int x, int y) {
	return this->background[x][y];
}

int Background::getWidth() {
	return this->width;
}

int Background::getHeight() {
	return this->height;
}

Background::~Background() {
}
