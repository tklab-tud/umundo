#include "umundo/cache/RandomCache.h"
#include <stdlib.h>
#include <time.h>
#include <cstdlib>

namespace umundo {

RandomCacheItem::RandomCacheItem(string name) {
	_name = name;
}
RandomCacheItem::~RandomCacheItem() {}

RandomCachePtr::RandomCachePtr() {
}

RandomCachePtr::~RandomCachePtr() {
}

RandomCacheItem* RandomCachePtr::getItem() {
	return (RandomCacheItem*)_item;
}

const string RandomCachePtr::getItemName() {
	return ((RandomCacheItem*)_item)->_name;
}

RandomCache::RandomCache(uint64_t size) : SCache(size) {
	srand ( time(NULL) );
	_model = new NGramModel();
}

shared_ptr<RandomCachePtr> RandomCache::getPointer(const string& name) {
	ScopeLock lock(_mutex);
	shared_ptr<RandomCachePtr> ptr = shared_ptr<RandomCachePtr>(new RandomCachePtr());
	if (_items.find(name) != _items.end())
		ptr->_item = _items[name];
	return ptr;
}

void RandomCache::insert(RandomCacheItem* item) {
	_items[item->_name] = item;
	SCache::insert(item);
}

void RandomCache::remove(RandomCacheItem* item) {
	if (_items.find(item->_name) == _items.end()) {
		_items.erase(item->_name);
		SCache::remove(item);
	}
}

void RandomCache::connectItems(float propability) {
	map<string, RandomCacheItem*>::iterator itemIter = _items.begin();
	while(itemIter != _items.end()) {
		map<string, RandomCacheItem*>::iterator innerItemIter = _items.begin();
		while(innerItemIter != _items.end()) {
			float random = (float)rand() / (float)RAND_MAX;
			if (random <= propability) {
				itemIter->second->_nexts.insert(innerItemIter->second);
			}
			innerItemIter++;
		}
		if(itemIter->second->_nexts.size() == 0) {
			// make sure there is at least one leaving edge
			int random = ((float)rand() / (float)RAND_MAX) * (_items.size() - 1);
			map<string, RandomCacheItem*>::iterator forceConnIter = _items.begin();
			while(random--) {
				forceConnIter++;
				if (forceConnIter->first.compare(itemIter->first))
					forceConnIter++;
			}
			itemIter->second->_nexts.insert(forceConnIter->second);
		}
		itemIter++;
	}
}
}
