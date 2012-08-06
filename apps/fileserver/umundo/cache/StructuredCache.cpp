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

#include "umundo/cache/StructuredCache.h"
#include "umundo/cache/CacheProfiler.h"

#include <limits>
#include <algorithm>

#ifdef WIN32
// max gets defined in windows.h and overrides numeric_limits<>::max
#undef max
#endif

namespace umundo {

SCache::SCache(uint64_t size) : _maxSize(size) {
	_distanceThreshold = 10;
	_convergenceThreshold = (float)0.01;
	_profiler = NULL;
	start();
}

SCache::~SCache() {
	stop();
	_monitor.signal();
	join();
}

/**
 * The maximum size for the cache in bytes.
 */
void SCache::setMaxSize(uint64_t maxSize) {
	ScopeLock lock(&_mutex);
	_maxSize = maxSize;
	// have the cache thread remanage its items
	_monitor.signal();
}

/**
 * Get the size this cache would have with the given pressure.
 */
uint64_t SCache::getSizeAtPressure(float pressure, bool breakEarly) {
	uint64_t proposedSize = 0;
	set<SCacheItem*>::iterator itemIter = _cacheItems.begin();
	while(itemIter != _cacheItems.end()) {
		proposedSize += (*itemIter)->getSizeAtPressure(pressure);
		if (proposedSize > _maxSize && breakEarly)
			break;
		itemIter++;
	}
	return proposedSize;
}

void SCache::update() {
	ScopeLock lock(&_mutex);

	resetDistance();

	float newPressure = _pressure;

	// cache size if we would apply the new pressure
	uint64_t pSize = getSizeAtPressure(newPressure, _profiler == NULL);
	if (_profiler != NULL)
		_profiler->startedPressureBalancing(newPressure);

	bool growing = pSize < _maxSize && newPressure > 0;

	// if we are growing, the current pressure is the upper bound
	float upperPressure = (float)(growing ? std::max(_pressure + 0.1, 1.0) : 1);
	// if we are shrinking, the current pressure is the lower bound
	float lowerPressure = (float)(!growing ? std::max(_pressure - 0.1, 0.0) : 0);

	if (_profiler != NULL)
		_profiler->newBounds(lowerPressure, upperPressure, newPressure, pSize);

	if (growing) {
		std::cout << "Growing ";
	} else {
		std::cout << "Shrinking ";
	}
	std::cout << "cache at pressure " << _pressure << std::endl;
	// use binary search to arrive at optimal pressure
	while(pSize > _maxSize || growing) {
		newPressure = (float)((upperPressure + lowerPressure) / 2.0);
		pSize = getSizeAtPressure(newPressure, _profiler == NULL);
		growing = pSize < _maxSize;
		std::cout << "[" << lowerPressure << "," << upperPressure << "]: " << newPressure << " => " << pSize << " of " << _maxSize << " ";

		if (_profiler != NULL)
			_profiler->newBounds(lowerPressure, upperPressure, newPressure, pSize);

		// did we convert?
		if (pSize == _maxSize || (upperPressure - lowerPressure < _convergenceThreshold && pSize < _maxSize)) {
			std::cout << "breaking" << std::endl;
			break;
		}

		// set new bounds
		if (growing) {
			std::cout << "new upper bound" << std::endl;
			upperPressure = newPressure; // (upperPressure - lowerPressure) / 2.0;
			if (upperPressure < (_convergenceThreshold / 4) && lowerPressure == 0)
				upperPressure = 0;
		} else {
			std::cout << "new lower bound" << std::endl;
			lowerPressure = newPressure; //(upperPressure - lowerPressure) / 2.0;
			if (1 - lowerPressure < (_convergenceThreshold / 4) && upperPressure == 1)
				lowerPressure = 1;
		}
		assert(upperPressure >= lowerPressure);
	}

	_pressure = newPressure;
	_currSize = pSize;

	if (_currSize > _maxSize)
		LOG_WARN("Could not get cache to %d bytes, will be %d bytes", _maxSize, _currSize);

	set<SCacheItem*>::iterator itemIter = _cacheItems.begin();
	while(itemIter != _cacheItems.end()) {
		(*itemIter)->applyPressure(_pressure);
		itemIter++;
	}

	if (_profiler != NULL)
		_profiler->finishedPressureBalancing(newPressure);

}

/**
 * Caclulate relevance and apply pressure.
 */
void SCache::run() {
	while(_monitor.wait() && isStarted()) {
		ScopeLock lock(&_mutex);
		update();
	}
}

void SCache::resetDistance() {
	ScopeLock lock(&_mutex);

	// set distance to infinity
	set<SCacheItem*>::iterator itemIter = _cacheItems.begin();
	while(itemIter != _cacheItems.end()) {
		(*itemIter)->_distance = std::numeric_limits<int>::max();
		itemIter++;
	}

	// calculate relevance from all pointers
	set<weak_ptr<SCachePointer> >::iterator ptrIter = _cachePointers.begin();
	list<std::pair<float, SCacheItem*> > itemQueue;
	while(ptrIter != _cachePointers.end()) {
		shared_ptr<SCachePointer> ptr = ptrIter->lock();
		if (ptr.get() != NULL) {
			itemQueue.push_back(std::make_pair(0, ptr->_item));
		} else {
			// weak pointer points to deleted object
			_cachePointers.erase(ptrIter);
		}
		ptrIter++;
	}

	// breadth first propagation of relevance
	while(!itemQueue.empty()) {
		int distance = itemQueue.front().first;
		SCacheItem* item = itemQueue.front().second;
    item->_distance = distance;

		set<SCacheItem*> next = item->getNext();
		set<SCacheItem*>::iterator nextIter = next.begin();
		while(nextIter != next.end()) {
      if ((*nextIter)->_distance > distance + 1) {
        itemQueue.push_back(std::make_pair(distance + 1, (*nextIter)));
      }
			nextIter++;
		}
		itemQueue.pop_front();
	}

}

shared_ptr<SCachePointer> SCache::getPointer(SCacheItem* item) {
	shared_ptr<SCachePointer> ptr = shared_ptr<SCachePointer>(new SCachePointer());

	if (_cacheItems.find(item) != _cacheItems.end())
		ptr->_item = item;

	_cachePointers.insert(ptr);
	dirty();
	return ptr;
}

void SCache::insert(SCacheItem* item) {
	item->_cache = this;
	_cacheItems.insert(item);
	dirty();
}

void SCache::remove(SCacheItem* item) {
	if(_cacheItems.find(item) != _cacheItems.end()) {
		item->_cache = NULL;
		_cacheItems.erase(item);
	}
	dirty();
}

bool SCache::isEmpty() {
	return (_cacheItems.size() == 0);
}

SCacheItem::~SCacheItem() {
}

}