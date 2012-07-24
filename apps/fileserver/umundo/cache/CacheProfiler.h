#ifndef CACHETODOT_H_NARTHU6E
#define CACHETODOT_H_NARTHU6E

#include "umundo/cache/StructuredCache.h"

namespace umundo {

class CacheItemLabeler {
public:
  virtual const string caption(SCache* cache) const { return ""; }
  virtual const string label(SCacheItem* item) const = 0;
  virtual const string label(SCacheItem* item1, SCacheItem* item2) const = 0;
  virtual const string shapeColor(SCacheItem* item) const { return "black"; }
  virtual int penWidth(SCacheItem* item) const { return 1; };
};
  
class CacheProfiler {
public:
  
  struct PressureBound {
    float lowerBound;
    float upperBound;
    float pressure;
    uint64_t propsedSize;
    uint64_t maxSize;
  };
  
	CacheProfiler(SCache* cache, const string& filename);
	~CacheProfiler();
	
	virtual void startedPressureBalancing(float currentPressure);
	virtual void finishedPressureBalancing(float currentPressure);
	virtual void newBounds(float lowerBound, float upperBound, float currPressure, uint64_t propsedSize);
	virtual void writeToPlot();

	static void layoutToDot(SCache* cache, const string& filename, const CacheItemLabeler& labeler);

protected:
	SCache* _cache;
	string _filename;
	int _filesWritten;

  vector<PressureBound> _currPressureBounds; // the pressure bounds during one iteration
  vector<vector<PressureBound> > _allPressureBounds; // all the pressure bounds we have seen
};

}

#endif /* end of include guard: CACHETODOT_H_NARTHU6E */
