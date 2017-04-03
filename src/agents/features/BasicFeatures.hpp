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

#ifndef FEATURES_H
#define FEATURES_H
#include "Features.hpp"
#endif
#ifndef BACKGROUND_H
#define BACKGROUND_H
#include "Background.hpp"
#endif

class BasicFeatures: public Features {
private:
//		Parameters *param;
	StellaEnvironment* m_env;
	Background *background;
	int screen_f_n_rows;
	int screen_f_n_columns;
	bool getSubstractBackground;
//		int numberOfFeatures;
public:
	/**
	 * Destructor, used to delete the background, which is allocated dynamically.
	 */
	~BasicFeatures();
	/**
	 * Constructor. Since every operation in this class has to be done knowing the number of
	 * columns, rows and colors to generate the feature vector, this information is given in
	 * param, as any other relevant information such as background.
	 *
	 * @param Parameters *param, which gives access to the number of columns, number of rows,
	 *                   number of colors and the background information
	 * @return nothing, it is a constructor.
	 */
	BasicFeatures(RomSettings *rom_settings, Settings &settings,
			ActionVect &actions, StellaEnvironment* _env);
	/**
	 * This method is the instantiation of the virtual method in the class Features (also check
	 * its documentation). It iterates over all tiles defined by the columns and rows and checks
	 * if each of the colors to be evaluated are present in the tile.
	 *
	 * REMARKS: - It is necessary to provide both the screen and the ram because of the superclass,
	 * despite the RAM being useless here. In fact a null pointer works just fine.
	 *          - To avoid return huge vectors, this method is void and the appropriate
	 * vector is returned trough a parameter passed by reference.
	 *          - This method was adapted from Sriram Srinivasan's code
	 *
	 * @param ALEScreen &screen is the current game screen that one may use to extract features.
	 * @param ALERAM &ram is the current game RAM that one may use to extract features.
	 * @param vector<int>& features an empy vector that will be filled with the requested information,
	 *        therefore it must be passed by reference. It contain the active indices.
	 * @return nothing as one will receive the requested data by the last parameter, by reference.
	 */
	void getActiveFeaturesIndices(const ALEScreen &screen, const ALERAM &ram,
			vector<int>& features);
	void getFeatures(const ALEScreen &screen, const ALERAM &ram,
			vector<bool>& features);
};
