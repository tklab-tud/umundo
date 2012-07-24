#include "umundo/prediction/SPA.h"
#include <stdlib.h> // strtol
#include <errno.h>

namespace umundo {

UITracer::UITracer(const string& filename) {
  _file.open(filename.c_str());
  if (_file) {
    string line;
    uint64_t delay = 0;
    string event = line;
    // read whole trace
    while (getline(_file, line)) {
      if (line.find_first_of(":") == 0) {
        const char* start = line.substr(1).c_str();
        char* end;
        delay = strtol(start, &end, 0);
        if (errno != 0)
          delay = 0;
        event = line.substr(line.find_first_of(" "));
      }
      _trace.push_back(std::make_pair(delay, event));
    }
  }
  _lastInputTime = Thread::getTimeStampMs();
}

UITracer::~UITracer() {
  _file.close();
}

void UITracer::trace(const string& event) {
  uint64_t now = Thread::getTimeStampMs();
  _trace.push_back(std::make_pair(now - _lastInputTime, event));
  if (_file)
    _file << ":" << now - _lastInputTime << " " << event << std::endl;
  _lastInputTime = now;
}

void UITracer::replay(UITracer::Traceable* traceable) {
  list<std::pair<uint64_t, string> >::iterator traceIter = _trace.begin();
  while(traceIter != _trace.end()) {
    Thread::sleepMs((uint32_t)traceIter->first);
    traceable->replayTrace(traceIter->second);
    traceIter++;
  }
}

  
vector<std::pair<string, float> > SPA::getSortedPredictions(list<string> history) {
  map<string, float> predictions = getPredictions(history);
  vector<std::pair<string, float> > sortedResults(predictions.begin(), predictions.end());
  std::sort(sortedResults.begin(), sortedResults.end(), mostProbableFirst);
  return sortedResults;
}
	
NGramModel::NGramModel(int size) {
  _windowSize = size;
  _applicablity.reserve(_windowSize + 1);
  _correctPredictions.reserve(_windowSize + 1);
  for (int i = 0; i < _windowSize + 1; i++) {
    _applicablity.push_back(0);
    _correctPredictions.push_back(0);
  }
}

bool NGramModel::mostCommonFirst (Occurences first, Occurences second) {
  return first.occurences > second.occurences;
}

string NGramModel::remember(const string& item, bool learn) {
  
  if (learn) {
//    std::cout << "inserting element " << item << std::endl;
    // calculate applicability
    list<list<NGramModel::Occurences> > freqs = getPriorOccurences();
    assert(freqs.size() == _windowSize + 1);
    list<list<NGramModel::Occurences> >::reverse_iterator freqIter = freqs.rbegin();
    int i = _windowSize;
    // use items with longest prefix first
    while (freqIter != freqs.rend()) {
      if (freqIter->size() > 0) {
        assert(i >= 0);
        assert(freqIter->begin()->prefixLength == i);
//        std::cout << "\tIs applicable with prefix " << i << "/" << freqIter->begin()->prefixLength << ": " << freqIter->begin()->prediction << std::endl;
        _applicablity[i] = _applicablity[i] + 1;
        freqIter->sort(mostCommonFirst);
        if (item.compare(freqIter->begin()->prediction) == 0) {
//          std::cout << "\tIs correct for prefix " << i << ": " << freqIter->begin()->prediction << std::endl;
          _correctPredictions[i] = _correctPredictions[i] + 1;
        } else {
//          std::cout << "\tBut not correct " << i << ": " << freqIter->begin()->prediction << " != " << item << std::endl;        
        }
        break;
      }
      i--; freqIter++;
    }    

#if 0
    i = 0;
    while (i < _windowSize) {
      std::cout << "\t" << i << " quality: " << _correctPredictions[i] << "/" << _applicablity[i] << std::endl;
      i++;
    }
#endif
    
    // remember item for the various prefixes
    list<string> histKeys = historyToKey();
    list<string>::iterator keyIter = histKeys.begin();
    while(keyIter != histKeys.end()) {
      map<string, int> predictions = _historyOccurences[*keyIter];
      if (predictions.find(item) == predictions.end()) {
        _historyOccurences[*keyIter][item] = 0;
      }
//      std::cout << "Inserting " << item << " at " << *keyIter << std::endl;
      _historyOccurences[*keyIter][item] = _historyOccurences[*keyIter][item] + 1;
      keyIter++;
    }
  }
  
  string forgotten;
  if (_historyWindow.size() >= _windowSize) {
    forgotten = _historyWindow.back();
    _historyWindow.pop_back();
  }
  _historyWindow.push_front(item);
  return forgotten;
}

/**
 * Returns a list of Occurences items per matching prefix length.
 *
 * result[0] = <all occurences where suffix equals whole history> / best matching occurences
 * result[n .. _windowSize - 1] = <all items matching in the last n prefix items>
 * result[_windowSize] = <unigrams with history is ignored> / least matching occurences
 */
list<list<NGramModel::Occurences> > NGramModel::getPriorOccurences(const list<string>& history) {
  list<list<Occurences> > result;
  
  list<string> histKeys = historyToKey(history);
  list<string>::iterator keyIter = histKeys.begin();
  int keyLength = 0;
  while(_windowSize - keyLength >= 0) {
    list<Occurences> lengthFreqs;
    if (keyIter != histKeys.end()) {
//      std::cout << "Looking for " << *keyIter << std::endl;
      if (_historyOccurences.find(*keyIter) != _historyOccurences.end()) {
        map<string, int> occurences = _historyOccurences[*keyIter];
        map<string, int>::iterator occurIter = occurences.begin();
        while(occurIter != occurences.end()) {
          Occurences freq;
          freq.prediction = occurIter->first;
          freq.occurences = occurIter->second;
          freq.prefixLength = keyLength;
          freq.prefix = *keyIter;
          lengthFreqs.push_back(freq);
          occurIter++;
        }
      }
      keyIter++;
    }
    result.push_back(lengthFreqs);
    keyLength++;
  }
  return result;
}

list<list<NGramModel::Occurences> > NGramModel::getPriorOccurences(int count, ... ) {
  
  list<string> vecHistory;
  va_list ap;
  
  va_start(ap, count);
  char* foo;
  for(int i = 0; i < count; i++) {
    foo = va_arg(ap, char*);
    vecHistory.push_back(string(foo));
  }
  va_end(ap);
  
  assert(vecHistory.size() == count);
  
  return getPriorOccurences(vecHistory);
}

list<string> NGramModel::historyToKey(int count, ... ) {

  list<string> vecHistory;
	va_list ap;

	va_start(ap, count);
  char* foo;
  while(count--) {
    foo = va_arg(ap, char*);
    vecHistory.push_back(foo);
  }
  va_end(ap);
  
  return historyToKey(vecHistory);
}

list<string> NGramModel::historyToKey(const list<string>& history) {
  list<string> keys;

  // empty history is a valid path
  keys.push_back("");
  
  string currKey = "";
  list<string>::const_iterator histIter = history.begin();
  while(histIter != history.end()) {
    currKey = *histIter + "," + currKey;
    keys.push_back(currKey);
//    std::cout << "currKey: " << currKey << std::endl;
    histIter++;
  }
  
//  list<string>::iterator keyIter = keys.begin();
//  while (keyIter != keys.end()) {
//    std::cout << "SDFG for " << *keyIter << std::endl;
//    keyIter++;
//  }

  return keys;
}

	
}