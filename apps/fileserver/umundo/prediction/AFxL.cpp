#include "umundo/prediction/AFxL.h"

namespace umundo {

map<string, float> AFxL::getPredictions(list<string> history) {
	map<string, float> results;
	vector<float> factors(_model->_windowSize + 1);

	list<list<NGramModel::Occurences> > freqs = _model->getPriorOccurences(history);
	list<list<NGramModel::Occurences> >::iterator freqIter = freqs.begin();
	assert(freqs.size() == _model->_windowSize + 1);

	// we can skip the absolute frequencies as their prefix length is 0 in the product below
	freqIter++;
	int i = 1;

	float multiplier = 1;
	while(freqIter != freqs.end()) {
		if (_model->_applicablity[i] > 0) {
			factors[i] = (float)_model->_correctPredictions[i] / (float)_model->_applicablity[i];
		} else {
			factors[i] = 1;
		}
		//std::cout << multiplier << " * Factor[" << i << "]: " << factors[i] << " = " << _correctPredictions[i] << "/" << _applicablity[i] << std::endl;
		if (freqIter->size() > 0) {
			list<NGramModel::Occurences>::iterator itemIter = freqIter->begin();
			while(itemIter != freqIter->end()) {
				if (results.find(itemIter->prediction) == results.end())
					results[itemIter->prediction] = 0.0;

				results[itemIter->prediction] += multiplier * i * factors[i] * itemIter->occurences;
				//std::cout << "Prediction " << i << ":" << itemIter->prefix << " .. " << itemIter->prediction << " with " << itemIter->occurences << " at " << results[itemIter->prediction] << std::endl;
				itemIter++;
			}
		}
//    std::cout << "New multiplier: 1.0-" << (float)factors[i] << " * " << multiplier << " = ";
		multiplier *= (1-factors[i]);
//    std::cout << multiplier << std::endl;
		i++;
		freqIter++;
	}

	// normalize the score
	float overallScore = 0;
	map<string, float>::iterator resultIter = results.begin();
	while(resultIter != results.end()) {
		overallScore += resultIter->second;
		resultIter++;
	}
	resultIter = results.begin();
	while(resultIter != results.end()) {
		results[resultIter->first] = results[resultIter->first] / overallScore;
		resultIter++;
	}
	_factors = factors;
	return results;
}


}