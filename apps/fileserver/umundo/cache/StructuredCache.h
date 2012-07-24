/**
 *  Copyright (C) 2012  Stefan Radomski (stefan.radomski@cs.tu-darmstadt.de)
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the FreeBSD license as published by the FreeBSD
 *  project.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 *  You should have received a copy of the FreeBSD license along with this
 *  program. If not, see <http://www.opensource.org/licenses/bsd-license>.
 */

#ifndef SCACHE_H_LAY4Q1LN
#define SCACHE_H_LAY4Q1LN

#include <umundo/core.h>
#include <list>

namespace umundo {

using std::list;
using std::set;
class SCachePointer;
class SCache;
class SCacheItem;
class SCacheHistory;
class CacheProfiler;

/**
 * A SCachePointer is the sole handle for applications to access items in the structured cache.
 *
 * It is important to exclusively use SCachePointers or subclasses thereof retrieved from the cache or
 * subclasses thereof to access SCacheItems in the cache as this will allow the cache to page out items
 * that are "further away" form the pointers.
 */
class DLLEXPORT SCachePointer {
public:
	SCachePointer() {}
	virtual ~SCachePointer() {}

	virtual SCacheItem* getItem() { return _item; };

	SCacheItem* _item;
	SCache* _cache;
	friend class SCache;
};

/**
 * A node in the structure of the cache with the actual data.
 *
 * Subclasses of this class are expected to alleviate the pressure put on them by
 * paging out their contents. If the pressure drops or their relevance increases, 
 * they are to reload their content.
 */
class DLLEXPORT SCacheItem {
public:
	virtual ~SCacheItem();

  /**
   * Assign relevance. 
   */
  virtual set<SCacheItem*> relevanceFromParent(float parentRelevance);
  float getRelevance() { return _relevance; }

	virtual set<SCacheItem*> getNext() = 0;

  virtual uint64_t assumePressure(float pressure) = 0;
  virtual void applyPressure(float pressure) = 0;

protected:
	float _relevance;
	SCache* _cache;

	friend class SCache;
	friend class SCachePointer;
};

/**
 * A SCache orders its items in a graph and puts pressure on those far away from items pointed to.
 */
class DLLEXPORT SCache : public Thread {
public:
	SCache(uint64_t size);
	virtual ~SCache();

	void insert(SCacheItem*);
	void remove(SCacheItem*);
	virtual bool isEmpty();

	void run();
  void dirty() { _monitor.signal(); }
	void update();

	void setMaxSize(uint64_t size);
  uint64_t getMaxSize()                                { return _maxSize; }
  uint64_t getCurrSize()                               { return _currSize; }
  float getPressure()                                  { return _pressure; }
  void setRelevanceThreshold(float relevanceThreshold) { _relevanceThreshold = relevanceThreshold; }
  float getRelevanceThreshold()                        { return  _relevanceThreshold; }
  void setProfiler(CacheProfiler* profiler)            { _profiler = profiler; }
  set<SCacheItem*> getAllItems()                       { return _cacheItems; }
  set<weak_ptr<SCachePointer> > getAllPointers()       { return _cachePointers; }
  
  Mutex _mutex;

protected:
  void resetRelevance();
  uint64_t assumePressure(float, bool breakEarly = false);
  
	virtual shared_ptr<SCachePointer> getPointer(SCacheItem*);

  uint64_t _maxSize; ///< the maximum size this cache is allowed to grow to
  uint64_t _currSize; ///< the current size the items in th cache have
	float _pressure; ///< the current pressure
  float _relevanceThreshold; ///< the threshold to stop propagating relevance
  float _convergenceThreshold; ///< the maximum bound distance when searching for optimal pressure
  
	Monitor _monitor;
	CacheProfiler* _profiler;
  set<SCacheItem*> _cacheItems; ///< All the items in the cache
	set<weak_ptr<SCachePointer> > _cachePointers; ///< The items pointed to with relevance = 1

	friend class SCachePointer;
	friend class SCacheItem;
  friend class CacheToDot;
  friend class CacheProfiler;
};

}
#endif /* end of include guard: SCACHE_H_LAY4Q1LN */
