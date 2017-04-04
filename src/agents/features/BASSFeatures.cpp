/****************************************************************************************
** Implementation of BASS Features, described in details in the paper below. 
**       "The Arcade Learning Environment: An Evaluation Platform for General Agents.
**        Marc G. Bellemare, Yavar Naddaf, Joel Veness, and Michael Bowling.
**        Journal of Artificial Intelligence Research, 47:253–279, 2013."
**
** The idea is to divide the screen in tiles and to answer, for each tile, if one of the
** n colors defined are present in that tile. Additionally we then evaluate whether there
** are pairwise combinations of these features.
**
** Author: Marlos C. Machado
***************************************************************************************/


#include "BASSFeatures.hpp"
#include "BasicFeatures.hpp"

using namespace std;

BASSFeatures::BASSFeatures(Parameters *param){
    this->param = param;
    numPureFeatures = this->param->getNumColumns() * this->param->getNumRows() * this->param->getNumColors();
    numPairwiseFeatures = numPureFeatures * (numPureFeatures -1) / 2;
}

BASSFeatures::~BASSFeatures(){}

void BASSFeatures::addPairwiseFeatures(vector<int>& features, int size){
    assert(size > 0);
    int offset;
    
    /*The equation to locate the coordinate of the interaction between row and column is:
    coordinate = size + row*size + column - (row +1)(1 + row + 1)/2
    The intuition is: index a matrix as a vector and subtract the number of terms
    you skip, which is the sum of terms of an arithmetic progression.*/
    for(unsigned int i = 0; i < features.size(); i++){
        for(unsigned int j = i + 1; j < features.size(); j++){
            //First indexing as a vectorization of a matrix:
            offset = size * features[i] + features[j];
            //But only has elements in the diagonal, it makes 
            //no sense to consider the elements below the diagonal:
            offset = offset - ((features[i]+1)*(features[i]+2))/2;
            features.push_back(size + offset);
        }
    }
}

void BASSFeatures::getActiveFeaturesIndices(const ALEScreen &screen, const ALERAM &ram, vector<int>& features){
    //First get the Basic Features for 8 colors:
    BasicFeatures basic(this->param);
    basic.getActiveFeaturesIndices(screen, ram, features);
    //Remove bias to be added at the end:
    features.pop_back();
    //Now obtain its pairwise combinations: 
    addPairwiseFeatures(features, basic.getNumberOfFeatures());
    //Bias:
    features.push_back(numPureFeatures + numPairwiseFeatures);

}

int BASSFeatures::getNumberOfFeatures(){
    return numPureFeatures + numPairwiseFeatures + 1;
}
