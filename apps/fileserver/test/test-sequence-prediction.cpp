#include "umundo/core.h"
#include "umundo/cache/StructuredCache.h"
#include "umundo/cache/NBandCache.h"
#include "umundo/cache/CacheProfiler.h"
#include "umundo/prediction/SPA.h"
#include "umundo/prediction/AFxL.h"
#include <fstream>
#include <iostream>
#include <iomanip>
#include <stdio.h>

using namespace umundo;

int main(int argc, char** argv, char** envp) {
	{
		int windowSize = 2;
		NGramModel* history = new NGramModel(windowSize);
		history->remember("a", true);
		assert(history->_applicablity[0] == 0);
		history->remember("b", true);
		assert(history->_applicablity[0] == 1);
		history->remember("c", true);
		assert(history->_applicablity[0] == 2);
		history->remember("d", true);
		assert(history->_applicablity[0] == 3);
		history->remember("e", true);
		assert(history->_applicablity[0] == 4);

		for (int i = 0; i < windowSize; i++) {
			assert(history->_correctPredictions[i] == 0);
		}

		history->remember("a", true);
		assert(history->_applicablity[0] == 5);
		assert(history->_correctPredictions[0] == 1);

		list<list<NGramModel::Occurences> > freqs = history->getPriorOccurences();
		list<list<NGramModel::Occurences> >::iterator freqIter = freqs.begin();
		assert(freqs.size() == windowSize + 1); // 0 - windowsSizeGrams
		assert(freqIter->size() == 5); // every entry is a 1-Gram
		assert((++freqIter)->size() == 1); // there is one entry with length 1 and prefix "a"
		assert((++freqIter)->size() == 0); // there is no entry with prefix "e,a"


		history->remember("b", true);
		assert(history->_applicablity[1] == 1);
		assert(history->_correctPredictions[1] == 1);
		history->remember("c", true);
		assert(history->_applicablity[2] == 1);
		assert(history->_correctPredictions[2] == 1);

	}

	{

		NGramModel* history = new NGramModel(10);

//    const char* testFilename = "";
		const char* testFilename = "/Users/sradomski/Desktop/PredictionExperiments/dataSets/word/test_M000.txt";
//    const char* testFilename = "/Users/sradomski/Desktop/PredictionExperiments/dataSets/word/all_tests.txt";
//    const char* testFilename = "/Users/sradomski/Desktop/PredictionExperiments/dataSets/small/test_1.txt";
//    const char* testFilename = "/Users/sradomski/Desktop/PredictionExperiments/dataSets/test/test_test1.txt";
		std::ifstream testFile(testFilename);
		if (testFile) {
			string line;
			while (getline(testFile, line)) {
				line.erase( remove( line.begin(), line.end(), '\r'), line.end() );
				if (line.find_first_of("#") == 0) {
					history->resetWindow();
				}
				//std::cout << "--- currLine: " << line << std::endl;
				history->remember(line, true);
			}
		} else {
			int i = 4;
			while(i--) {
				history->remember("a", true);
				history->remember("b", true);
				history->remember("c", true);
				history->remember("d", true);
				history->remember("e", true);
				if (i > 0) {
					history->resetWindow();
					history->remember("######", true);
				}
			}
		}
		history->resetWindow();
//    history->remember("#####");
//    history->remember("b");
//    history->remember("c");
//    history->remember("d");
		history->remember("FileOpen");
		history->remember("FileNew");
		history->remember("EditPaste");
		history->remember("FileSaveAs");
//    history->remember("FormatBold");
//    history->remember("FormatBold");
//    history->remember("EditClear");

//    std::cout << "--- Occurences (" << history->_historyOccurences.size() << "):" << std::endl;
//    map<string, map<string, int > >::iterator occurIter = history->_historyOccurences.begin();
//    while(occurIter != history->_historyOccurences.end()) {
//      std::cout << "\tprefix: " << "'" << occurIter->first << "'" << std::endl;
//      map<string, int >::const_iterator predIter = occurIter->second.begin();
//      while(predIter != occurIter->second.end()) {
//        std::cout << "\t\t" << predIter->second << "x " << predIter->first << std::endl;
//        predIter++;
//      }
//      occurIter++;
//    }

		std::cout << "--- Quality (" << history->_applicablity.size() << "):" << std::endl;
		for (int i = 0; i < history->_applicablity.size(); i++) {
			std::cout << "\t" << i << ": " << history->_correctPredictions[i] << "/" << history->_applicablity[i] << " = " << (float)history->_correctPredictions[i] / (float)history->_applicablity[i] << std::endl;
		}

		std::cout << "--- Keys per History:" << std::endl;
		list<string> keys = history->historyToKey();
		list<string>::iterator keyIter = keys.begin();
		while(keyIter != keys.end()) {
			std::cout << "'" << *keyIter << "'" << std::endl;
			keyIter++;
		}

		std::cout << "--- Freq:" << std::endl;
		list<list<NGramModel::Occurences> > frequencies = history->getPriorOccurences(4, "FileSaveAs", "EditPaste", "FileNew", "FileOpen");
		list<list<NGramModel::Occurences> >::iterator lengthIter = frequencies.begin();
		while (lengthIter != frequencies.end()) {
			list<NGramModel::Occurences>::iterator freqIter = lengthIter->begin();
			while(freqIter != lengthIter->end()) {
				if (freqIter->prefixLength > 0) {
					std::cout << "Pred: " << freqIter->occurences << "x " << freqIter->prefix << " .. " << freqIter->prediction << std::endl;
				}
				freqIter++;
			}
			lengthIter++;
		}

		AFxL afxl(history);

		std::cout << "--- Predictions:" << std::endl;
		vector<std::pair<string, float> > preds = afxl.getSortedPredictions();
		vector<std::pair<string, float> >::iterator predIter = preds.begin();
		while(predIter != preds.end()) {
			std::cout << predIter->first << ": " << predIter->second << "" << std::endl;
			predIter++;
		}

		std::cout << "--- Factors:" << std::endl;
		int j = 0;
		vector<float>::iterator facIter = afxl._factors.begin();
		while(facIter != afxl._factors.end()) {
			std::cout << j << ": " << *facIter << std::endl;
			facIter++;
			j++;
		}

	}

}