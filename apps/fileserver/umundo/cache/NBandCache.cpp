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

#include "umundo/cache/NBandCache.h"

using namespace umundo;

NBandProxyCacheItem::NBandProxyCacheItem() {
	_otherBand = NULL;
	_currentItem = NULL;
}

set<SCacheItem*> NBandProxyCacheItem::getNext() {
	set<SCacheItem*> next;
	if (_otherBand != NULL)
		next.insert((SCacheItem*)_otherBand->_currentItem);
	next.insert((SCacheItem*)_currentItem);
	return next;
}

NBandCacheItem::NBandCacheItem(string name, string band) : _name(name), _band(band), _isPrepared(false) {
	_left = NULL;
	_right = NULL;
	_up = NULL;
	_down = NULL;
}

set<SCacheItem*> NBandCacheItem::getNext() {
	set<SCacheItem*> next;
	if (_left != NULL)
		next.insert(_left);
	if (_right != NULL)
		next.insert(_right);
	if (_up->_otherBand != NULL)
		next.insert(_up->_otherBand->_currentItem);
	if (_down->_otherBand != NULL)
		next.insert(_down->_otherBand->_currentItem);
	return next;
}

NBandCachePtr::NBandCachePtr() {
	_item = NULL;
}

NBandCachePtr::~NBandCachePtr() {}

SCacheItem* NBandCachePtr::getItem() {
	return _item;
}

const string NBandCachePtr::getBandName() {
	if (_item != NULL)
		return ((NBandCacheItem*)_item)->_band;
	return "";
}

const string NBandCachePtr::getItemName() {
	if (_item != NULL)
		return ((NBandCacheItem*)_item)->_name;
	return "";
}

const int NBandCachePtr::getElementId() {
	int elemId = 0;
	NBandCacheItem* curr = ((NBandCacheItem*)_item);
	while(curr->_left != NULL) {
		curr = curr->_left;
		elemId++;
	}
	return elemId;
}

NBandCacheItem* NBandCachePtr::left(bool moveBand) {
	ScopeLock lock(_cache->_mutex);
	((NBandCache*)_cache)->_model->remember("left", true);
	if (((NBandCacheItem*)_item)->_left == NULL) {
		return NULL;
	}
	if (moveBand) {
		((NBandCacheItem*)_item)->_up->_currentItem = ((NBandCacheItem*)_item)->_left;
		((NBandCacheItem*)_item)->_down->_currentItem = ((NBandCacheItem*)_item)->_left;
	}
	_item = ((NBandCacheItem*)_item)->_left;
	assert(_item != NULL);
	_cache->dirty();
	return ((NBandCacheItem*)_item);
}

NBandCacheItem* NBandCachePtr::right(bool moveBand) {
	ScopeLock lock(_cache->_mutex);
	((NBandCache*)_cache)->_model->remember("right", true);
	if (((NBandCacheItem*)_item)->_right == NULL) {
		return NULL;
	}
	if (moveBand) {
		((NBandCacheItem*)_item)->_up->_currentItem = ((NBandCacheItem*)_item)->_right;
		((NBandCacheItem*)_item)->_down->_currentItem = ((NBandCacheItem*)_item)->_right;
	}
	_item = ((NBandCacheItem*)_item)->_right;
	assert(_item != NULL);
	_cache->dirty();
	return ((NBandCacheItem*)_item);
}

NBandCacheItem* NBandCachePtr::up() {
	ScopeLock lock(_cache->_mutex);
	((NBandCache*)_cache)->_model->remember("up", true);
	if (((NBandCacheItem*)_item)->_up->_otherBand == NULL) {
		return NULL;
	}
	_item = ((NBandCacheItem*)_item)->_up->_otherBand->_currentItem;
	assert(_item != NULL);
	_cache->dirty();
	return ((NBandCacheItem*)_item);
}

NBandCacheItem* NBandCachePtr::down() {
	ScopeLock lock(_cache->_mutex);
	((NBandCache*)_cache)->_model->remember("down", true);
	if (((NBandCacheItem*)_item)->_down->_otherBand == NULL) {
		return NULL;
	}
	_item = ((NBandCacheItem*)_item)->_down->_otherBand->_currentItem;
	assert(_item != NULL);
	_cache->dirty();
	return ((NBandCacheItem*)_item);
}

NBandCache::NBandCache(uint64_t size) : SCache(size) {
	_model = new NGramModel(5);
}

shared_ptr<NBandCachePtr> NBandCache::getPointer() {
	ScopeLock lock(_mutex);
	if (_bands.size() > 0) {
		return getPointer(_bands.begin()->first);
	} else {
		return getPointer("");
	}
}

shared_ptr<NBandCachePtr> NBandCache::getPointer(const string& band, int elemId) {
	ScopeLock lock(_mutex);
	// is there such a band?
	BandIter bandIter = _bands.find(band);
	if (bandIter == _bands.end())
		return shared_ptr<NBandCachePtr>();

	// get the current item of the band
	shared_ptr<NBandCachePtr> ptr = shared_ptr<NBandCachePtr>(new NBandCachePtr());
	NBandCacheItem* item;
	if (elemId == -1) {
		item = bandIter->second.begin()->second->_up->_currentItem;
		ptr->_item = item;
	} else {
		item = bandIter->second.begin()->second;
		while(elemId-- > 0) {
			item = item->_right;
		}
		ptr->_item = item;
	}
	assert(ptr->_item != NULL);
	ptr->_cache = this;
	_cachePointers.insert(ptr);
	dirty();
	return ptr;
}

shared_ptr<NBandCachePtr> NBandCache::getPointer(const string& band, const string& name) {
	ScopeLock lock(_mutex);
	// is there such a band?
	BandIter bandIter = _bands.find(band);
	if (bandIter == _bands.end())
		return shared_ptr<NBandCachePtr>();

	map<string, NBandCacheItem*>::iterator itemIter = bandIter->second.find(name);
	if (itemIter == bandIter->second.end())
		return shared_ptr<NBandCachePtr>();

	shared_ptr<NBandCachePtr> ptr = shared_ptr<NBandCachePtr>(new NBandCachePtr());
	ptr->_item = itemIter->second;

	assert(ptr->_item != NULL);
	ptr->_cache = this;
	_cachePointers.insert(ptr);
	dirty();
	return ptr;
}

void NBandCache::insert(NBandCacheItem* item) {
	ScopeLock lock(_mutex);
	Proxies currProxies;
	// if there is no such band yet establish its proxy
	if (_proxies.find(item->_band) == _proxies.end()) {
		currProxies = std::make_pair(new NBandProxyCacheItem(), new NBandProxyCacheItem());
		SCache::insert((SCacheItem*)currProxies.first);
		SCache::insert((SCacheItem*)currProxies.second);
		_proxies[item->_band] = currProxies;
		currProxies.first->_currentItem = item;
		currProxies.second->_currentItem = item;
		// insert the band and connect with neighboring proxies
		ProxyIter proxyIter = _proxies.find(item->_band);

		if (proxyIter != _proxies.begin()) {
			// there is a proxy for the band above
			proxyIter--;
			proxyIter->second.second->_otherBand = currProxies.first;
			currProxies.first->_otherBand = proxyIter->second.second;
			proxyIter++;
		}

		if (++proxyIter != _proxies.end()) {
			// there is a proxy for the band below
			proxyIter->second.first->_otherBand = currProxies.second;
			currProxies.second->_otherBand = proxyIter->second.first;
		}
	} else {
		currProxies = _proxies[item->_band];
	}

	item->_up = currProxies.first;
	item->_down = currProxies.second;

	// insert the new item within the band
	if (_bands[item->_band].find(item->_name) == _bands[item->_band].end()) {
		_bands[item->_band][item->_name] = item;
		map<string, NBandCacheItem*>::iterator itemIter = _bands[item->_band].find(item->_name);
		if (itemIter != _bands[item->_band].begin()) {
			// there is an item before us in the band
			itemIter--;
			item->_left = itemIter->second;
			itemIter->second->_right = item;
			itemIter++;
		}
		if (++itemIter != _bands[item->_band].end()) {
			// there is an item after us in the band
			item->_right = itemIter->second;
			itemIter->second->_left = item;
		}
	}
	SCache::insert((SCacheItem*)item);

}

void NBandCache::remove(NBandCacheItem* item) {
	ScopeLock lock(_mutex);

	// have we been inserted before?
	if (_bands[item->_band].find(item->_name) == _bands[item->_band].end())
		return;

	// is someone pointing to the item?
	set<weak_ptr<SCachePointer> >::iterator ptrIter = _cachePointers.begin();
	while(ptrIter != _cachePointers.end()) {
		shared_ptr<SCachePointer> ptr = ptrIter->lock();
		if (ptr && ptr->_item == item) {
			ptr->_item = NULL;
		}
		ptrIter++;
	}

	// have we been the last item in the band?
	if (item->_left == NULL && item->_right == NULL) {
		// remove whole band
		if (item->_up->_otherBand != NULL)
			item->_up->_otherBand->_otherBand = item->_down->_otherBand;
		if (item->_down->_otherBand != NULL)
			item->_down->_otherBand->_otherBand = item->_up->_otherBand;
		SCache::remove((SCacheItem*)item->_down);
		SCache::remove((SCacheItem*)item->_up);

		// remove item and band
		_bands[item->_band].erase(item->_name);
		_bands.erase(item->_band);
	} else {
		// adapt neighbors neightbors
		map<string, NBandCacheItem*>::iterator itemIter = _bands[item->_band].find(item->_name);
		if (itemIter != _bands[item->_band].begin()) {
			// there is someone before us in the band
			itemIter--;
			NBandCacheItem* leftNeighbor = itemIter->second;
			leftNeighbor->_right = item->_right; // its right neighbor is our former right member
			itemIter++; // reset iterator
		}
		if (++itemIter != _bands[item->_band].end()) {
			// there is an item after us in the band
			NBandCacheItem* rightNeighbor = itemIter->second;
			rightNeighbor->_left = item->_left;
		}

		// have we been the current item?
		if (item->_up->_currentItem == item) {
			// prefer left neighbor
			NBandCacheItem* newCurrent = (item->_left != NULL ? item->_left : item->_right);
			item->_up->_currentItem = newCurrent;
			item->_down->_currentItem = newCurrent;
		}

		// remove item from band
		_bands[item->_band].erase(item->_name);
	}


	SCache::remove((SCacheItem*)item);
}
