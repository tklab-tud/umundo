#ifndef SPA_H_63PKNW7K
#define SPA_H_63PKNW7K

#include <list>
#include <fstream>
#include <umundo/core.h>
//#include "umundo/prediction/SPA.h"

namespace umundo {

using std::list;

class DLLEXPORT UITracer {
public:
  class DLLEXPORT Traceable {
  public:
    virtual void replayTrace(const string& event) = 0;
  };
  
  UITracer(const string& filename);
  ~UITracer();
  void trace(const string& event);
  void replay(UITracer::Traceable* traceable);
  
protected:
  Mutex _mutex;
  list<std::pair<uint64_t, string> > _trace;
  int64_t _lastInputTime;
  std::fstream _file;
};

class DLLEXPORT NGramModel {
public:
  struct Occurences {
    int prefixLength;
    int occurences;
    string prediction;
    string prefix;
  };

  NGramModel(int size = 5);
  string remember(const string& item, bool learn = false);
  void resetWindow() { _historyWindow.clear(); }
  
  list<list<Occurences> > getPriorOccurences(int count, ... );
  list<list<Occurences> > getPriorOccurences(const list<string>& history);
  list<list<Occurences> > getPriorOccurences() { return getPriorOccurences(_historyWindow); };
  list<string> historyToKey(int count, ... );
  list<string> historyToKey(const list<string>& history);
  list<string> historyToKey() { return historyToKey(_historyWindow); }
  
  vector<int> _applicablity;
  vector<int> _correctPredictions;

  list<string> _historyWindow;
  map<string, map<string, int > > _historyOccurences; // history to predictions with seen count
  int _windowSize;

protected:
  static bool mostCommonFirst(Occurences, Occurences);
};

class DLLEXPORT SPA {
public:
  SPA(NGramModel* model) : _model(model) {}
  virtual ~SPA() {}
  
  virtual map<string, float> getPredictions(list<string> history) = 0;
  virtual map<string, float> getPredictions() {return getPredictions(_model->_historyWindow); };
  vector<std::pair<string, float> > getSortedPredictions(list<string> history);
  vector<std::pair<string, float> > getSortedPredictions() { return getSortedPredictions(_model->_historyWindow); }

protected:
  static bool mostProbableFirst(const std::pair<string, float> &lhs, const std::pair<string, float> &rhs) {
    return rhs.second < lhs.second;
  }
  
  NGramModel* _model;
};


}

#endif /* end of include guard: SPA_H_63PKNW7K */
