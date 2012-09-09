#include "umundo/core.h"
#include "umundo/cache/StructuredCache.h"
#include "umundo/cache/NBandCache.h"
#include "umundo/cache/RandomCache.h"
#include "umundo/cache/CacheProfiler.h"
#include <fstream>
#include <iostream>
#include <iomanip>
#include <stdio.h>
#include <algorithm>
#include <math.h>

using namespace umundo;

int itemsToLeft(NBandCacheItem* item);
int itemsToRight(NBandCacheItem* item);
int bandsOnTop(NBandCacheItem* item);
int bandsBelow(NBandCacheItem* item);

class NBandCacheProfiler : public CacheProfiler {
public:
	NBandCacheProfiler(SCache* cache, const string& filename) : CacheProfiler(cache, filename) {}

	virtual void finishedPressureBalancing(float currentPressure) {
		if (_cache->getAllItems().empty())
			return;

		NBandCacheItem* topLeft = (NBandCacheItem*)*(_cache->getAllItems().begin());
		// move to first band
		while(topLeft->_up->_otherBand != NULL) {
			topLeft = topLeft->_up->_otherBand->_currentItem;
		}
		// move to first item in band
		while(topLeft->_left != NULL) {
			topLeft = topLeft->_left;
		}

		int rightMost = 0;
		int leftMost = 0;
		// find the width when we would align current items in bands
		NBandCacheItem* currItem = topLeft->_up->_currentItem;
		while(true) {
			rightMost = std::max(rightMost, itemsToRight(currItem));
			leftMost = std::max(leftMost, itemsToLeft(currItem));

			if (currItem->_down->_otherBand == NULL)
				break;
			currItem = currItem->_down->_otherBand->_currentItem;
		}
		// width of relevance matrix if we aligned current items
		int width = leftMost + 1 + rightMost;

		std::ofstream outFile;
		std::stringstream outFileName;
		outFileName << _filename << "." << std::setw(2) << std::setfill('0') << _filesWritten << ".relevance.dat";
		outFile.open(outFileName.str().c_str());

		int currWidth = 0;
		currItem = topLeft->_up->_currentItem;
		for(;;) {
			currWidth = 0;
			for (int i = 0; i < leftMost - itemsToLeft(currItem); i++) {
//        outFile << currWidth << " " << bandsOnTop(currItem) << " " << "NaN       " << std::endl;
				currWidth++;
			}

			while(currItem->_left != NULL)
				currItem = currItem->_left;

			NBandCacheItem* iterItem = currItem;
			while(iterItem != NULL) {
				outFile << currWidth << " " << std::setw(2) << std::setfill('0') << bandsOnTop(iterItem) << " " << iterItem->getDistance() << std::endl;
				iterItem = iterItem->_right;
				currWidth++;
			}

			while(currWidth < width) {
//        outFile << currWidth << " " << bandsOnTop(currItem) << " " << "NaN       " << std::endl;
				currWidth++;
			}

			outFile << std::endl;

			if (currItem->_down->_otherBand == NULL)
				break;
			currItem = currItem->_down->_otherBand->_currentItem;
		}

		currItem = topLeft->_up->_currentItem;
		int i = 0;
		for(;;) {
			outFile << leftMost - i << " " << std::setw(2) << std::setfill('0') << bandsOnTop(currItem) << " " << currItem->getDistance() << std::endl;
			if(currItem->_down->_otherBand == NULL)
				break;
			currItem = currItem->_down->_otherBand->_currentItem;
		}
		outFile << std::endl;

		outFile << "NaN NaN NaN" << std::endl;
		CacheProfiler::finishedPressureBalancing(currentPressure);
	}
};

class TestCacheItem : public NBandCacheItem {
public:
	TestCacheItem(uint64_t size, string name, string band) : NBandCacheItem(name, band), _fullSize(size) {
		_previewSize = _fullSize * 0.2;
		_hasAllLoaded = false;
		_hasPreviewLoaded = false;
	}
	uint64_t getSizeAtPressure(float pressure) {
    float relevance = pow((float)0.8, (float)_distance);

//    std::cout << "\t" << _name << ":" << _band << " assuming pressure: " << pressure << " at " << getRelevance() << " -> ";
		if (relevance > pressure) {
			_currSize = _fullSize;
		} else if (1.2*(relevance*(1/(relevance + 0.2))) > pressure) {
//    } else if (1.2 * getRelevance() > pressure) {
			_currSize = _previewSize;
		} else {
			_currSize = 0;
		}

//    std::cout << _currSize << std::endl;
		return _currSize;
	}

	void applyPressure(float pressure) {
//    std::cout << _name << ":" << _band << " applying pressure: " << getRelevance() << ":" << pressure << std::endl;
    float relevance = pow((float)0.8, (float)_distance);
		if (relevance > pressure) {
			if (!_hasAllLoaded) {
				std::cout << _name << ":" << _band << " is loading full" << std::endl;
				_hasAllLoaded = true;
				_hasPreviewLoaded = false;
			}
		} else if (1.2*(relevance*(1/(relevance + 0.2))) > pressure) {
			if (!_hasPreviewLoaded) {
				std::cout << _name << ":" << _band << " is loading preview" << std::endl;
				_hasAllLoaded = false;
				_hasPreviewLoaded = true;
			}
		} else {
			if (_hasPreviewLoaded || _hasAllLoaded) {
				std::cout << _name << ":" << _band << " is unloading" << std::endl;
				_hasAllLoaded = false;
				_hasPreviewLoaded = false;
			}
		}
	}

  bool addPath(std::list<SCacheItem*> path) {
    if (path.size() > 5)
      return false;
    
    _paths.push_back(path);
    
    std::list<SCacheItem*>::iterator pathIter = path.begin();
    while(pathIter != path.end()) {
      TestCacheItem* tItem = dynamic_cast<TestCacheItem*>(*pathIter);
      if (tItem) {
        std::cout << tItem->_band << ":" << tItem->_name << " -> ";
      } else {
        std::cout << "proxy -> ";        
      }
      pathIter++;
    }
    std::cout << _band << ":" << _name << std::endl;
      
    return true;
  }
  
	bool _hasPreviewLoaded;
	bool _hasAllLoaded;
	uint64_t _currSize;
	uint64_t _fullSize;
	uint64_t _previewSize;
};

class TestCacheItemLabeler : public CacheItemLabeler {
public:
	TestCacheItemLabeler() {}
	TestCacheItemLabeler(const string& additionalCaption) {
		_additionalCaption = additionalCaption;
	}
	const string caption(SCache* cache) const {
		std::stringstream ss;
		ss << "" << (cache->getCurrSize() / 1024) << "kB of " << (cache->getMaxSize() / 1024) << "kB\\n";
		ss << "Pressure: " << cache->getPressure();
		return ss.str();
	}
	const string label(SCacheItem* item) const {
		TestCacheItem* tItem = dynamic_cast<TestCacheItem*>(item);
		if (tItem) {
			std::stringstream ss;
			ss << tItem->_band << ":" << tItem->_name << "\\n";
			ss << (tItem->_currSize / 1024) << "kB at " << std::setprecision(3) << tItem->getDistance() << "\\n";
			return ss.str();
		}
		return "";
	}
	const string label(SCacheItem* item1, SCacheItem* item2) const {
		TestCacheItem* tItem1 = dynamic_cast<TestCacheItem*>(item1);
		if (tItem1) {
			if (tItem1->_up == item2)    return "up";
			if (tItem1->_down == item2)  return "down";
			if (tItem1->_left == item2)  return "left";
			if (tItem1->_right == item2) return "right";
			return "";
		}
		return "";
	}
	const string shapeColor(SCacheItem* item) const {
		TestCacheItem* tItem = dynamic_cast<TestCacheItem*>(item);
		if (tItem) {
			if (tItem->_hasAllLoaded) return "red";
			if (tItem->_hasPreviewLoaded) return "orange";
			return "yellow";
		}
		return "grey";
	}

	string _additionalCaption;
};

bool testPaths() {
  NBandCache* nbCache = new NBandCache(1024.0 * 1024.0 * 2.3);
  nbCache->stop();
  
  for (int band = 0; band < 3; band++) {
    for (int item = 0; item < 3; item++) {
      nbCache->insert(new TestCacheItem(0, toStr(item), toStr(band)));
    }
  }
  shared_ptr<NBandCachePtr> ptr = nbCache->getPointer("0", "0");
  nbCache->update();
  return true;
}

bool testNodeLoading() {
  // test pressure and relevance
  NBandCache* nbCache = new NBandCache(1024.0 * 1024.0 - 1);
  
  // insert a single item
  TestCacheItem* tItem1 = new TestCacheItem(1024 * 1024, "00", "0");
  nbCache->insert(tItem1);
  Thread::sleepMs(50);
  CacheProfiler::layoutToDot(nbCache, "foo1.dot", TestCacheItemLabeler("Single item without cache pointer"));
  
  // we have no pointer, therefore no relevance
  assert(!tItem1->_hasAllLoaded);
  assert(!tItem1->_hasPreviewLoaded);
  
  // get a pointer
  shared_ptr<NBandCachePtr> nbPtr = nbCache->getPointer("0");
  Thread::sleepMs(50);
  
  // item is too big, preview only
  assert(tItem1->_hasPreviewLoaded);
  CacheProfiler::layoutToDot(nbCache, "foo2.dot", TestCacheItemLabeler());
  
  // increase cache size for item to be fully loaded
  nbCache->setMaxSize(1024 * 1024);
  Thread::sleepMs(50);
  assert(tItem1->_hasAllLoaded);
  CacheProfiler::layoutToDot(nbCache, "foo3.dot", TestCacheItemLabeler());
  
  // add another item to the right of the existing one
  TestCacheItem* tItem2 = new TestCacheItem(1024 * 1024, "01", "0");
  nbCache->insert(tItem2);
  Thread::sleepMs(100);
  
  // both have their previews loaded
  CacheProfiler::layoutToDot(nbCache, "foo4.dot", TestCacheItemLabeler());
  
  // increase cache size to hold one full and a preview
  nbCache->setMaxSize(1024.0 * 1024.0 + (1024.0 * 1024.0 * 0.3));
  Thread::sleepMs(50);
  assert(tItem1->_hasAllLoaded);
  assert(tItem2->_hasPreviewLoaded);
  CacheProfiler::layoutToDot(nbCache, "foo5.dot", TestCacheItemLabeler());
  
  delete nbCache;
  return true;
}

bool testNavigation() {
  // test navigation in NBand cache
  NBandCache* nbCache = new NBandCache(1024.0 * 1024.0 * 2.3);
  nbCache->stop();
  nbCache->insert(new TestCacheItem(1024 * 1024, "00", "0"));
  nbCache->insert(new TestCacheItem(1024 * 1024, "01", "0"));
  nbCache->insert(new TestCacheItem(1024 * 1024, "02", "0"));
  nbCache->insert(new TestCacheItem(1024 * 1024, "00", "1"));
  nbCache->insert(new TestCacheItem(1024 * 1024, "01", "1"));
  nbCache->insert(new TestCacheItem(1024 * 1024, "02", "1"));
  nbCache->insert(new TestCacheItem(1024 * 1024, "00", "2"));
  
  shared_ptr<NBandCachePtr> nbPtr = nbCache->getPointer("2");
  assert(nbPtr.get() != NULL);
  assert(((NBandCacheItem*)nbPtr->getItem())->getName().compare("00") == 0);
  assert(((NBandCacheItem*)nbPtr->getItem())->getBand().compare("2") == 0);
  assert(bandsOnTop((NBandCacheItem*)nbPtr->getItem()) == 2);
  assert(bandsBelow((NBandCacheItem*)nbPtr->getItem()) == 0);
  assert(itemsToRight((NBandCacheItem*)nbPtr->getItem()) == 0);
  assert(itemsToLeft((NBandCacheItem*)nbPtr->getItem()) == 0);
  nbCache->update();
  CacheProfiler::layoutToDot(nbCache, "bar01.dot", TestCacheItemLabeler());
  
  assert(nbPtr->right() == NULL);
  nbCache->update();
  //    CacheProfiler::layoutToDot(nbCache, "bar02.dot", TestCacheItemLabeler());
  
  assert(nbPtr->down() == NULL);
  nbCache->update();
  //    CacheProfiler::layoutToDot(nbCache, "bar03.dot", TestCacheItemLabeler());
  
  nbPtr->up();
  assert(((NBandCacheItem*)nbPtr->getItem())->getName().compare("00") == 0);
  assert(((NBandCacheItem*)nbPtr->getItem())->getBand().compare("1") == 0);
  assert(bandsOnTop((NBandCacheItem*)nbPtr->getItem()) == 1);
  assert(bandsBelow((NBandCacheItem*)nbPtr->getItem()) == 1);
  assert(itemsToRight((NBandCacheItem*)nbPtr->getItem()) == 2);
  assert(itemsToLeft((NBandCacheItem*)nbPtr->getItem()) == 0);
  
  nbCache->update();
  CacheProfiler::layoutToDot(nbCache, "bar04.dot", TestCacheItemLabeler());
  
  assert(nbPtr->right(true) != NULL);
  assert(((NBandCacheItem*)nbPtr->getItem())->getName().compare("01") == 0);
  assert(((NBandCacheItem*)nbPtr->getItem())->getBand().compare("1") == 0);
  assert(itemsToRight((NBandCacheItem*)nbPtr->getItem()) == 1);
  assert(itemsToLeft((NBandCacheItem*)nbPtr->getItem()) == 1);
  nbCache->update();
  CacheProfiler::layoutToDot(nbCache, "bar05.dot", TestCacheItemLabeler());
  
  assert(nbPtr->right(true) != NULL);
  assert(((NBandCacheItem*)nbPtr->getItem())->getName().compare("02") == 0);
  assert(((NBandCacheItem*)nbPtr->getItem())->getBand().compare("1") == 0);
  assert(itemsToRight((NBandCacheItem*)nbPtr->getItem()) == 0);
  assert(itemsToLeft((NBandCacheItem*)nbPtr->getItem()) == 2);
  nbCache->update();
  CacheProfiler::layoutToDot(nbCache, "bar06.dot", TestCacheItemLabeler());
  
  assert(nbPtr->right() == NULL);
  nbCache->update();
  //    CacheProfiler::layoutToDot(nbCache, "bar07.dot", TestCacheItemLabeler());
  
  assert(nbPtr->up() != NULL);
  assert(((NBandCacheItem*)nbPtr->getItem())->getName().compare("00") == 0);
  assert(((NBandCacheItem*)nbPtr->getItem())->getBand().compare("0") == 0);
  assert(bandsOnTop((NBandCacheItem*)nbPtr->getItem()) == 0);
  assert(bandsBelow((NBandCacheItem*)nbPtr->getItem()) == 2);
  assert(itemsToRight((NBandCacheItem*)nbPtr->getItem()) == 2);
  assert(itemsToLeft((NBandCacheItem*)nbPtr->getItem()) == 0);
  nbCache->update();
  CacheProfiler::layoutToDot(nbCache, "bar08.dot", TestCacheItemLabeler());
  
  assert(nbPtr->down() != NULL);
  assert(((NBandCacheItem*)nbPtr->getItem())->getName().compare("02") == 0);
  assert(((NBandCacheItem*)nbPtr->getItem())->getBand().compare("1") == 0);
  assert(itemsToRight((NBandCacheItem*)nbPtr->getItem()) == 0);
  assert(itemsToLeft((NBandCacheItem*)nbPtr->getItem()) == 2);
  nbCache->update();
  CacheProfiler::layoutToDot(nbCache, "bar09.dot", TestCacheItemLabeler());
  
  assert(nbPtr->up() != NULL);
  assert(((NBandCacheItem*)nbPtr->getItem())->getName().compare("00") == 0);
  assert(((NBandCacheItem*)nbPtr->getItem())->getBand().compare("0") == 0);
  assert(bandsOnTop((NBandCacheItem*)nbPtr->getItem()) == 0);
  assert(bandsBelow((NBandCacheItem*)nbPtr->getItem()) == 2);
  assert(itemsToRight((NBandCacheItem*)nbPtr->getItem()) == 2);
  assert(itemsToLeft((NBandCacheItem*)nbPtr->getItem()) == 0);
  
  nbCache->update();
  CacheProfiler::layoutToDot(nbCache, "bar11.dot", TestCacheItemLabeler());
  
  assert(nbPtr->down() != NULL);
  assert(nbPtr->down() != NULL);
  assert(((NBandCacheItem*)nbPtr->getItem())->getName().compare("00") == 0);
  assert(((NBandCacheItem*)nbPtr->getItem())->getBand().compare("2") == 0);
  assert(bandsOnTop((NBandCacheItem*)nbPtr->getItem()) == 2);
  assert(bandsBelow((NBandCacheItem*)nbPtr->getItem()) == 0);
  assert(itemsToRight((NBandCacheItem*)nbPtr->getItem()) == 0);
  assert(itemsToLeft((NBandCacheItem*)nbPtr->getItem()) == 0);
  
  nbCache->update();
  CacheProfiler::layoutToDot(nbCache, "bar12.dot", TestCacheItemLabeler());
  
  // start deleting entries
  nbPtr->up();
  assert(((NBandCacheItem*)nbPtr->getItem())->getName().compare("02") == 0);
  assert(((NBandCacheItem*)nbPtr->getItem())->getBand().compare("1") == 0);
  nbCache->remove(((NBandCacheItem*)nbPtr->getItem())->_left); // removed 1.01
  assert(itemsToLeft((NBandCacheItem*)nbPtr->getItem()) == 1);
  
  nbCache->remove(((NBandCacheItem*)nbPtr->getItem())->_down->_otherBand->_currentItem); // removed 2.00
  assert(bandsBelow((NBandCacheItem*)nbPtr->getItem()) == 0);
  
  nbCache->remove((NBandCacheItem*)nbPtr->getItem()); // removed 1.02
  assert((NBandCacheItem*)nbPtr->getItem() == NULL);
  
  delete nbCache;
  return true;
}

bool testProfiling() {
  NBandCache* nbCache = new NBandCache(1024.0 * 1024.0 * 4.8);
  NBandCacheProfiler* profiler = new NBandCacheProfiler(nbCache, "test");
  nbCache->stop();
  nbCache->setProfiler(profiler);
  srand ( time(NULL) );
  int nrBands = 10 + rand() % 10;
  for (int band = 0; band < nrBands; band++) {
    std::stringstream ssBand;
    ssBand << std::setw(2) << std::setfill('0') << band;
    int nrItem = 10 + rand() % 20;
    for (int entity = 0; entity < nrItem; entity++) {
      std::stringstream ssEntity;
      ssEntity << entity;
      
      nbCache->insert(new TestCacheItem(1024 * 1024, ssEntity.str(), ssBand.str()));
    }
  }
  
  shared_ptr<NBandCachePtr> nbPtr1 = nbCache->getPointer("00", 0);
  for (int i = 0; i < nrBands; i++) {
    int random = rand() % itemsToRight((NBandCacheItem*)nbPtr1->getItem());
    for (int j = 0; j < random; j++) {
      nbPtr1->right(true);
    }
    nbPtr1->down();
  }
  nbPtr1 = nbCache->getPointer("10");
  
  //    shared_ptr<NBandCachePtr> nbPtr2 = nbCache->getPointer("10", 10);
  
  nbCache->update();
  
  CacheProfiler::layoutToDot(nbCache, "baz.dot", TestCacheItemLabeler());
  profiler->writeToPlot();
  return true;
}

class TestRandomCacheItem : public RandomCacheItem {
public:
  TestRandomCacheItem(const string& name) : RandomCacheItem(name) {}
  
  uint64_t getSizeAtPressure(float pressure) {
    return 0;
  }
  
  void applyPressure(float pressure) {
  }
};

class RandomCacheItemLabeler : public CacheItemLabeler {
  const string label(SCacheItem* item) const {
    return ((RandomCacheItem*)item)->_name;
  }
  
	const string label(SCacheItem* item1, SCacheItem* item2) const {
    return "";
  }

};

bool testRandomCache() {
  RandomCache* rCache = new RandomCache(1024 * 1024);
  rCache->stop();
  for (int i = 0; i < 20; i++) {
    rCache->insert(new TestRandomCacheItem(toStr(i)));
  }
  rCache->connectItems(.2);
  CacheProfiler::layoutToDot(rCache, "rCache.dot", RandomCacheItemLabeler());
    
  return true;
}

int main(int argc, char** argv, char** envp) {

//  if (!testPaths())
//    return EXIT_FAILURE;
  if (!testRandomCache())
    return EXIT_FAILURE;
//  if (!testNodeLoading())
//    return EXIT_FAILURE;
//  if (!testNavigation())
//    return EXIT_FAILURE;
//  if (!testProfiling())
//    return EXIT_FAILURE;
  return EXIT_SUCCESS;
}

int itemsToLeft(NBandCacheItem* item) {
	int i = 0;
	while((item = item->_left)) i++;
	return i;
}

int itemsToRight(NBandCacheItem* item) {
	int i = 0;
	while((item = item->_right)) i++;
	return i;
}

int bandsOnTop(NBandCacheItem* item) {
	int i = 0;
	while(item->_up->_otherBand != NULL) {
		item = item->_up->_otherBand->_currentItem;
		i++;
	}
	return i;
}

int bandsBelow(NBandCacheItem* item) {
	int i = 0;
	while(item->_down->_otherBand != NULL) {
		item = item->_down->_otherBand->_currentItem;
		i++;
	}
	return i;
}




