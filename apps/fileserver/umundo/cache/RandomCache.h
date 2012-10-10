#ifndef RANDOMCACHE_H_LAY4Q1LN
#define RANDOMCACHE_H_LAY4Q1LN

#include "umundo/cache/StructuredCache.h"
#include "umundo/prediction/SPA.h"

namespace umundo {

class DLLEXPORT RandomCacheItem : public SCacheItem {
public:
	RandomCacheItem(string name);
	~RandomCacheItem();
	set<SCacheItem*> getNext() {
		return _nexts;
	}
	const string getName() {
		return _name;
	}
	set<SCacheItem*> _nexts;
	string _name;
};

class DLLEXPORT RandomCachePtr : public SCachePointer {
public:
	RandomCachePtr();

	virtual ~RandomCachePtr();
	RandomCacheItem* getItem();
	const string getItemName();
};

class DLLEXPORT RandomCache : public SCache {
public:
	RandomCache(uint64_t size);

	using SCache::getPointer;
	virtual shared_ptr<RandomCachePtr> getPointer(const string& name);
	virtual void insert(RandomCacheItem* item);
	virtual void remove(RandomCacheItem* item);

	void connectItems(float propability);
	map<string, RandomCacheItem*> _items;

protected:
	NGramModel* _model;
	friend class RandomCachePtr;
};

}


#endif